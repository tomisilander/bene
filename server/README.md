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
| `BENE_ALLOWED_DATA_ROOTS` | **Required** for `/v1/learn`: comma-separated directory roots; `vdfile` and `datafile` must resolve under one of them. |
| `BENE_BIN_DIR` | Directory containing bene executables (default: `<repo>/bin`). |
| `BENE_LOGREG_FILE` | Path to `logreg256x2000.bin` (default: `<bin>/logreg256x2000.bin`). |
| `BENE_MAX_SUBGRAPH_VARS` | Maximum number of selected variables per request (default 32, max 64). |
| `BENE_MAX_CONCURRENT_JOBS` | Concurrent pipeline runs (default 2). |
| `BENE_DEBUG_WORKDIR` | Set to `1` to keep temp work directories (path returned in response). |
| `BENE_HOST` / `BENE_PORT` | Bind address for `python -m bene_server.main` (default `127.0.0.1:8765`). |

## Run

```sh
export BENE_ALLOWED_DATA_ROOTS="/path/to/bene/example,/path/to/bene/tests/data"
uvicorn bene_server.main:app --host 127.0.0.1 --port 8765
```

Or: `bene-server` (same defaults as `main`).

## API

- `GET /health` — liveness.
- `GET /v1/info` — binary path and limits.
- `POST /v1/learn` — JSON body: `vdfile`, `datafile`, `variables` (ordered global column indices), `score`, optional `required_arcs`, `forbidden_arcs`, `zeta`, `max_parents`.

Semantics match bene’s `-s` selfile and `-c` constraints: learning is over the **induced subgraph** on the selected columns only (see top-level plan).
