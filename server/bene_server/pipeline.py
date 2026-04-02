"""Run the bene binary pipeline in a temporary working directory."""

from __future__ import annotations

import os
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path


@dataclass
class PipelineResult:
    score: float
    arcs_local: list[tuple[int, int]]
    work_dir: Path | None


def _run(
    args: list[str],
    *,
    cwd: Path | None = None,
    env: dict[str, str] | None = None,
) -> None:
    proc = subprocess.run(
        args,
        cwd=cwd,
        env=env,
        capture_output=True,
        text=True,
        check=False,
    )
    if proc.returncode != 0:
        err = proc.stderr.strip() or proc.stdout.strip() or f"exit {proc.returncode}"
        cmd = " ".join(args)
        raise RuntimeError(f"Command failed ({proc.returncode}): {cmd}\n{err}")


def _parse_arcs_local(net2parents: Path, parents2arcs: Path, net_path: Path) -> list[tuple[int, int]]:
    p1 = subprocess.run(
        [str(net2parents), str(net_path), "-"],
        capture_output=True,
        text=True,
        check=True,
    )
    p2 = subprocess.run(
        [str(parents2arcs), "-", "-"],
        input=p1.stdout,
        capture_output=True,
        text=True,
        check=True,
    )
    arcs: list[tuple[int, int]] = []
    for line in p2.stdout.splitlines():
        line = line.strip()
        if not line:
            continue
        parts = line.split()
        if len(parts) != 2:
            continue
        arcs.append((int(parts[0]), int(parts[1])))
    return arcs


def _write_selfile(path: Path, global_vars: list[int]) -> None:
    with path.open("w", encoding="ascii") as f:
        for v in global_vars:
            f.write(f"{v}\n")


def _write_constraints(
    path: Path,
    global_vars: list[int],
    required: list[tuple[int, int]],
    forbidden: list[tuple[int, int]],
) -> None:
    g2l = {g: i for i, g in enumerate(global_vars)}
    lines: list[str] = []

    def edge_local(a: tuple[int, int]) -> tuple[int, int]:
        s, d = a
        if s not in g2l or d not in g2l:
            raise ValueError(f"Edge ({s}, {d}) references a variable outside `variables`")
        return g2l[s], g2l[d]

    for s, d in required:
        ls, ld = edge_local((s, d))
        lines.append(f"+ {ls} {ld}\n")
    for s, d in forbidden:
        ls, ld = edge_local((s, d))
        lines.append(f"- {ls} {ld}\n")

    with path.open("w", encoding="ascii") as f:
        for line in lines:
            f.write(line)


def run_subgraph_pipeline(
    *,
    vd_path: Path,
    data_path: Path,
    global_vars: list[int],
    score: str,
    required_arcs: list[tuple[int, int]],
    forbidden_arcs: list[tuple[int, int]],
    bin_dir: Path,
    logreg_path: Path,
    zeta: bool,
    max_parents: int | None,
    keep_workdir: bool,
) -> PipelineResult:
    """
    Execute get_local_scores → … → get_best_net, then score_net and arc extraction.

    ``global_vars`` defines variable selection and order for bene's ``-s`` selfile.
    """
    bin_dir = bin_dir.resolve()
    logreg_path = logreg_path.resolve()

    for name in (
        "get_local_scores",
        "split_local_scores",
        "reverse_local_scores",
        "zeta_local_scores",
        "get_best_parents",
        "get_best_sinks",
        "get_best_order",
        "get_best_net",
        "score_net",
    ):
        exe = bin_dir / name
        if not os.access(exe, os.X_OK):
            raise FileNotFoundError(f"Missing or non-executable: {exe}")

    net2parents = bin_dir / "net2parents"
    parents2arcs = bin_dir / "parents2arcs"

    tmp_ctx: tempfile.TemporaryDirectory[str] | None = None
    work_path: Path
    if keep_workdir:
        work_path = Path(tempfile.mkdtemp(prefix="bene-server-"))
    else:
        tmp_ctx = tempfile.TemporaryDirectory(prefix="bene-server-")
        work_path = Path(tmp_ctx.name)

    try:
        resfile = work_path / "res"
        selfile = work_path / "selfile"
        cstrfile = work_path / "constraints"

        _write_selfile(selfile, global_vars)
        if required_arcs or forbidden_arcs:
            _write_constraints(cstrfile, global_vars, required_arcs, forbidden_arcs)

        nof_vars = len(global_vars)

        gls_cmd: list[str] = [
            str(bin_dir / "get_local_scores"),
            str(vd_path),
            str(data_path),
            score,
            str(resfile),
            "-l",
            str(logreg_path),
        ]
        if max_parents is not None:
            gls_cmd.extend(["-m", str(max_parents)])
        if required_arcs or forbidden_arcs:
            gls_cmd.extend(["-c", str(cstrfile)])
        gls_cmd.extend(["-s", str(selfile)])

        _run(gls_cmd)
        _run([str(bin_dir / "split_local_scores"), str(nof_vars), str(work_path)])
        _run([str(bin_dir / "reverse_local_scores"), str(nof_vars), str(work_path)])
        if zeta:
            _run([str(bin_dir / "zeta_local_scores"), str(nof_vars), str(work_path)])
        _run([str(bin_dir / "get_best_parents"), str(nof_vars), str(work_path)])
        _run(
            [
                str(bin_dir / "get_best_sinks"),
                str(nof_vars),
                str(work_path),
                str(work_path / "sinks"),
            ]
        )
        _run(
            [
                str(bin_dir / "get_best_order"),
                str(nof_vars),
                str(work_path / "sinks"),
                str(work_path / "ord"),
            ]
        )
        _run(
            [
                str(bin_dir / "get_best_net"),
                str(nof_vars),
                str(work_path),
                str(work_path / "ord"),
                str(work_path / "net"),
            ]
        )

        sn = subprocess.run(
            [str(bin_dir / "score_net"), str(work_path / "net"), str(work_path)],
            capture_output=True,
            text=True,
            check=False,
        )
        if sn.returncode != 0:
            err = sn.stderr.strip() or sn.stdout.strip()
            raise RuntimeError(f"score_net failed: {err}")
        score_line = sn.stdout.strip().splitlines()[-1] if sn.stdout.strip() else ""
        total_score = float(score_line)

        arcs_local = _parse_arcs_local(net2parents, parents2arcs, work_path / "net")

        work_dir_out: Path | None = work_path if keep_workdir else None
        return PipelineResult(
            score=total_score,
            arcs_local=arcs_local,
            work_dir=work_dir_out,
        )
    finally:
        if tmp_ctx is not None:
            tmp_ctx.cleanup()


def globalize_arcs(
    arcs_local: list[tuple[int, int]],
    global_vars: list[int],
) -> list[tuple[int, int]]:
    """Map local (parent, child) indices to global column indices."""
    out: list[tuple[int, int]] = []
    for src_l, dst_l in arcs_local:
        src_g = global_vars[src_l]
        dst_g = global_vars[dst_l]
        out.append((src_g, dst_g))
    return out
