#!/usr/bin/env python3
"""
Compare bene CLI (data2net.sh) wall time vs HTTP server for datasets under DATAIN.

Only includes datasets whose .vd line count is <= --max-vars (default 17).

Usage:
  DATAIN=~/datain BENE_HOME=/path/to/bene ./benchmark_datain.py
  ./benchmark_datain.py --limit 6    # first N datasets after filter (sorted by name)

Requires: bene built (bin/data2net.sh), server deps (httpx), uvicorn for HTTP test.
"""

from __future__ import annotations

import argparse
import asyncio
import os
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path

SCORE = "BIC"
SERVER_PORT = int(os.environ.get("BENCH_SERVER_PORT", "9876"))
HOST = "127.0.0.1"


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def datain_root() -> Path:
    p = os.environ.get("DATAIN", str(Path.home() / "datain"))
    return Path(p).expanduser().resolve()


def discover_datasets(root: Path, max_vars: int) -> list[tuple[str, Path, Path, int]]:
    """Return sorted list of (name, vd, idt, nvars) with nvars <= max_vars."""
    out: list[tuple[str, Path, Path, int]] = []
    for sub in sorted(root.iterdir()):
        if not sub.is_dir():
            continue
        name = sub.name
        vd = sub / f"{name}.vd"
        idt = sub / f"{name}.idt"
        if not vd.is_file() or not idt.is_file():
            continue
        n = sum(1 for _ in vd.open("r", encoding="utf-8", errors="replace"))
        if n <= max_vars:
            out.append((name, vd, idt, n))
    return out


def cli_one(bene_bin: Path, vd: Path, idt: Path, work: Path) -> float:
    """Run data2net.sh once; return elapsed seconds."""
    work.mkdir(parents=True, exist_ok=True)
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True)
    script = bene_bin / "data2net.sh"
    t0 = time.perf_counter()
    r = subprocess.run(
        [str(script), str(vd), str(idt), SCORE, str(work)],
        cwd=str(bene_bin.parent),
        capture_output=True,
        text=True,
    )
    elapsed = time.perf_counter() - t0
    if r.returncode != 0:
        raise RuntimeError(f"CLI failed {vd}: {r.stderr or r.stdout}")
    return elapsed


async def main_async(args: argparse.Namespace) -> int:
    root = datain_root()
    bene_home = Path(os.environ.get("BENE_HOME", str(repo_root())))
    bene_bin = bene_home / "bin"
    if not (bene_bin / "data2net.sh").is_file():
        print(f"Missing {bene_bin}/data2net.sh", file=sys.stderr)
        return 1

    max_vars = args.max_vars
    datasets = discover_datasets(root, max_vars)
    if args.only:
        by_name = {name: (name, vd, idt, n) for name, vd, idt, n in datasets}
        ordered: list[tuple[str, Path, Path, int]] = []
        for part in args.only.split(","):
            name = part.strip()
            if not name:
                continue
            if name not in by_name:
                print(f"Unknown or >max-vars dataset: {name}", file=sys.stderr)
                return 1
            ordered.append(by_name[name])
        datasets = ordered
    elif args.limit is not None:
        datasets = datasets[: args.limit]
    if not datasets:
        print(f"No datasets with vd+idt under {root} (max {max_vars} vars)", file=sys.stderr)
        return 1

    print(f"DATAIN={root}")
    print(f"max_vars={max_vars} — {len(datasets)} datasets:")
    for name, vd, _idt, n in datasets:
        print(f"  {name}: {n} vars")
    print()

    # --- CLI sequential ---
    cli_total = 0.0
    tmp_cli = Path("/tmp/bene_bench_cli")
    for i, (name, vd, idt, n) in enumerate(datasets):
        w = tmp_cli / f"run_{i}"
        t = cli_one(bene_bin, vd, idt, w)
        cli_total += t
        print(f"CLI {name} ({n} vars): {t:.3f}s")
    print(f"CLI sequential total: {cli_total:.3f}s")
    print()

    # --- HTTP server (path-based learn) ---
    server_py = repo_root() / "server"
    venv_py = server_py / ".venv" / "bin" / "python"
    py = str(venv_py) if venv_py.is_file() else sys.executable

    env = os.environ.copy()
    env["BENE_ALLOWED_DATA_ROOTS"] = str(root)
    env["BENE_MAX_CONCURRENT_JOBS"] = "2"
    env["BENE_BIN_DIR"] = str(bene_bin)
    env["BENE_HOST"] = HOST
    env["BENE_PORT"] = str(SERVER_PORT)

    proc = subprocess.Popen(
        [py, "-m", "uvicorn", "bene_server.main:app", "--host", HOST, "--port", str(SERVER_PORT)],
        cwd=str(server_py),
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
    )
    try:
        import urllib.request

        for _ in range(50):
            try:
                urllib.request.urlopen(f"http://{HOST}:{SERVER_PORT}/health", timeout=0.5)
                break
            except OSError:
                time.sleep(0.1)
        else:
            err = proc.stderr.read().decode() if proc.stderr else ""
            raise RuntimeError(f"server did not start: {err}")

        import httpx

        base = f"http://{HOST}:{SERVER_PORT}"

        def var_list(n: int) -> list[int]:
            return list(range(n))

        async def one_learn(client: httpx.AsyncClient, name: str, vd: Path, idt: Path, n: int) -> tuple[str, float]:
            t0 = time.perf_counter()
            r = await client.post(
                f"{base}/v1/learn",
                json={
                    "vdfile": str(vd),
                    "datafile": str(idt),
                    "variables": var_list(n),
                    "score": SCORE,
                },
                timeout=7200.0,
            )
            elapsed = time.perf_counter() - t0
            if r.status_code != 200:
                raise RuntimeError(f"learn {name}: {r.status_code} {r.text}")
            return name, elapsed

        async def run_all():
            async with httpx.AsyncClient() as client:
                tasks = [
                    one_learn(client, name, vd, idt, nvars)
                    for name, vd, idt, nvars in datasets
                ]
                t_wall0 = time.perf_counter()
                results = await asyncio.gather(*tasks)
                wall = time.perf_counter() - t_wall0
            return results, wall

        results, server_wall = await run_all()
        for name, elapsed in sorted(results):
            print(f"HTTP {name} (request time): {elapsed:.3f}s")
        print(f"HTTP wall (all {len(datasets)} requests, max 2 concurrent jobs): {server_wall:.3f}s")
        print()
        http_sum = sum(elapsed for _name, elapsed in results)
        print(f"Summary:")
        print(f"  CLI sequential total (sum of runs): {cli_total:.3f}s")
        print(f"  HTTP wall (all requests in flight, max 2 concurrent jobs): {server_wall:.3f}s")
        print(f"  HTTP sum of per-request times (includes queue wait): {http_sum:.3f}s")
        print(f"  Note: HTTP wall < CLI total when overlap helps; overhead is subprocess/HTTP, not the bene math.")

    finally:
        proc.send_signal(signal.SIGTERM)
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()

    return 0


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Benchmark bene CLI vs HTTP server (datasets ≤ max-vars).")
    p.add_argument(
        "--max-vars",
        type=int,
        default=17,
        help="Include only datasets whose .vd has at most this many lines (default: 17)",
    )
    p.add_argument(
        "--limit",
        type=int,
        default=None,
        metavar="N",
        help="Use only the first N datasets after sort (ignored if --only is set)",
    )
    p.add_argument(
        "--only",
        type=str,
        default="",
        metavar="NAMES",
        help="Comma-separated dataset folder names (e.g. iris,ecoli,voting). Overrides --limit.",
    )
    return p.parse_args()


def main() -> int:
    args = parse_args()
    return asyncio.run(main_async(args))


if __name__ == "__main__":
    sys.exit(main())
