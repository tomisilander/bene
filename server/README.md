# bene HTTP server

Phase A orchestration: runs the existing `bin/` pipeline on a variable subset with optional arc constraints.

## Setup

Build bene binaries first (`build/build.sh` from the repo root). Install the server:

```sh
cd server
python3 -m venv .venv && . .venv/bin/activate
pip install -e .
```

## Configuration

| Environment variable | Meaning |
|---------------------|---------|
| `BENE_ALLOWED_DATA_ROOTS` | **Required** for `/v1/learn` when using `vdfile`/`datafile` paths: comma-separated directory roots; both paths must resolve under one of them. **Not** required when using `dataset_id` from `POST /v1/datasets`. |
| `BENE_DATASET_STAGING_DIR` | Where multipart uploads are stored (default: `<TMP>/bene-server-datasets`). |
| `BENE_MAX_UPLOAD_BYTES_TOTAL` | Max combined size of vd + data in one upload (default 50_000_000). |
| `BENE_DATASET_TTL_SECONDS` | Staged datasets older than this (by directory mtime) are deleted by a background task (default 86400). |
| `BENE_CLEANUP_INTERVAL_SECONDS` | How often to run TTL cleanup (default 3600). |
| `BENE_BIN_DIR` | Directory containing bene executables (default: `<repo>/bin`). |
| `BENE_LOGREG_FILE` | Path to `logreg256x2000.bin` (default: `<bin>/logreg256x2000.bin`). |
| `BENE_MAX_SUBGRAPH_VARS` | Maximum number of selected variables per request (default 32, max 64). |
| `BENE_MAX_CONCURRENT_JOBS` | Concurrent pipeline runs (default 2). |
| `BENE_DEBUG_WORKDIR` | Set to `1` to keep temp work directories (path returned in response). |
| `BENE_HOST` / `BENE_PORT` | Bind address for `python -m bene_server.main` (default `127.0.0.1:8765`). |
| `TMPDIR` | Standard temp directory for each job’s working directory (`tempfile`); point to fast local storage in production. |

## Run

```sh
export BENE_ALLOWED_DATA_ROOTS="/path/to/bene/example,/path/to/bene/tests/data"
uvicorn bene_server.main:app --host 127.0.0.1 --port 8765
```

Or: `bene-server` (same defaults as `main`).

## API

- `GET /health` — liveness.
- `GET /v1/info` — binary path, upload limits, TTL, staging directory.
- `POST /v1/datasets` — multipart form: fields `vd` and `data` (files). Returns `dataset_id` and byte counts; **413** if combined size exceeds `BENE_MAX_UPLOAD_BYTES_TOTAL`.
- `DELETE /v1/datasets/{dataset_id}` — remove a staged dataset before TTL (204 or 404).
- `POST /v1/learn` — JSON body: either **`dataset_id`** (from upload) **or** **`vdfile` + `datafile`** (paths); plus `variables` (ordered global column indices), `score`, optional `required_arcs`, `forbidden_arcs`, `zeta`, `max_parents`.

Semantics match bene’s `-s` selfile and `-c` constraints: learning is over the **induced subgraph** on the selected columns only (see top-level plan).

**Cleaning:** staged uploads are removed when their directory is older than `BENE_DATASET_TTL_SECONDS` (mtime), on a schedule (`BENE_CLEANUP_INTERVAL_SECONDS`), and once at startup. Clients can also `DELETE` explicitly.

## Concurrent jobs and storage

**Disk fragmentation** (files split across non-contiguous disk blocks) is rarely the bottleneck on modern SSDs with flash translation layers; it is not the main concern when many bene processes run in parallel.

What actually limits throughput:

- **Filesystem and I/O contention**: each run creates a temp directory with many small files (`res`, per-variable score files, `sinks`, `ord`, `net`, …). Many processes on the same host compete for **IOPS**, **inode allocation**, and metadata updates on the same volume.
- **`tmpfs` / RAM**: if `/tmp` is memory-backed, many large jobs can exhaust RAM or swap; if `/tmp` is disk-backed, you hit disk throughput instead.
- **Shared network filesystems** (NFS, Lustre): high metadata load from many concurrent temp dirs can hurt latency; prefer **local NVMe scratch** per worker node.
- **Per-process limit**: `BENE_MAX_CONCURRENT_JOBS` caps how many pipelines run at once inside one server process; scale out with many server instances behind a queue instead of unbounded parallelism on one disk.

**Operational guidance:** set `TMPDIR` (or mount a dedicated scratch path) to a **local**, fast, **dedicated** directory on each machine; size the queue so aggregate disk throughput stays below what the disk can sustain; avoid putting temp work on a shared home directory over NFS for heavy workloads.

## Tests

```sh
cd server
pip install -e ".[dev]"
pytest tests/                        # needs `pip install -e ".[dev]"` (httpx)
```
