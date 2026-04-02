# bene server — client guide

This document describes how a **client application** (script, service, or UI) should call the bene HTTP API to run exact Bayesian network structure learning on a **variable subset** with optional arc constraints.

The server exposes **OpenAPI** metadata. With the server running, open **`/docs`** (Swagger UI) or **`/openapi.json`** for an interactive schema and “try it” forms.

---

## Base URL and versioning

- **Base URL:** configure per deployment (e.g. `https://worker.example.com` or `http://127.0.0.1:8765`).
- **API prefix:** resource routes live under **`/v1/`** (health is at **`/health`**).
- **HTTPS:** recommended in production; this guide uses `BASE` as a placeholder for the origin (scheme + host + optional port).

**Authentication:** the current server does not implement auth. Deploy behind a reverse proxy or API gateway if you need authentication, rate limits, or TLS termination.

---

## Endpoints summary

| Method | Path | Purpose |
|--------|------|---------|
| `GET` | `/health` | Liveness probe |
| `GET` | `/v1/info` | Server limits and configuration (read-only) |
| `POST` | `/v1/datasets` | Upload variable description + data files (multipart) |
| `DELETE` | `/v1/datasets/{dataset_id}` | Remove a staged upload before TTL |
| `POST` | `/v1/learn` | Run structure learning (JSON) |

---

## `GET /health`

**Response:** `200` with JSON:

```json
{ "status": "ok" }
```

Use for load balancers and orchestration health checks.

---

## `GET /v1/info`

**Response:** `200` with JSON fields including:

- `bin_dir` — where bene executables are expected on the server host.
- `max_subgraph_vars` — maximum allowed length of `variables` in `/v1/learn`.
- `max_concurrent_jobs` — how many learn jobs may run concurrently server-side.
- `allowed_data_roots_configured` — whether path-based `vdfile`/`datafile` learning is allowed (see below).
- `dataset_staging_dir`, `max_upload_bytes_total`, `dataset_ttl_seconds`, `cleanup_interval_seconds` — upload staging and TTL behavior.

Clients can use this to adapt UI (e.g. cap variable count) or to log diagnostics.

---

## `POST /v1/datasets` — upload a dataset

Use this when the client **does not** share a filesystem with the server (typical for remote workers).

### Request

- **Method:** `POST`
- **URL:** `{BASE}/v1/datasets`
- **Content-Type:** `multipart/form-data`
- **Parts (two required files):**

| Part name | Type | Description |
|-----------|------|-------------|
| `vd` | file | Variable description file (bene “vd” format: one variable per line, tab-separated names and value labels). |
| `data` | file | Discrete data file: rows = observations, columns = variables; values are 0-based integers as in bene. |

The combined size of **both** parts must not exceed the server’s `max_upload_bytes_total` (see `/v1/info`). If it does, the server responds with **413**.

### Response

**200** with JSON:

| Field | Type | Meaning |
|-------|------|---------|
| `dataset_id` | string | UUID; pass to `POST /v1/learn`. |
| `vd_bytes` | integer | Bytes stored for the vd part. |
| `data_bytes` | integer | Bytes stored for the data part. |
| `ttl_seconds` | number | Hint: staged data may be removed after this age (see server env). |
| `message` | string | Human-readable reminder about using `dataset_id` and TTL. |

### Example (`curl`)

```sh
curl -sS -X POST "${BASE}/v1/datasets" \
  -F "vd=@/path/to/mydata.vd;type=text/plain" \
  -F "data=@/path/to/mydata.idt;type=text/plain"
```

### Example (JavaScript `fetch`)

```javascript
const form = new FormData();
form.append("vd", fileVd, "mydata.vd");
form.append("data", fileData, "mydata.idt");

const res = await fetch(`${baseUrl}/v1/datasets`, { method: "POST", body: form });
if (!res.ok) throw new Error(await res.text());
const { dataset_id } = await res.json();
```

---

## `DELETE /v1/datasets/{dataset_id}`

Removes the staged files for `dataset_id` immediately (before TTL).

- **204** — directory was present and removed.
- **404** — no staged dataset with that id (already deleted or expired).
- **400** — invalid `dataset_id` format (not a UUID).

---

## `POST /v1/learn` — run structure learning

### Request

- **Method:** `POST`
- **URL:** `{BASE}/v1/learn`
- **Content-Type:** `application/json`
- **Body:** JSON object (see below).

#### Data source (exactly one mode)

1. **Staged upload:** set **`dataset_id`** to the value returned by `POST /v1/datasets`.  
   - Do **not** send `vdfile` or `datafile` in the same request.

2. **Server-local paths:** set **`vdfile`** and **`datafile`** to **absolute paths** on the **server** host, under directories allowed by server configuration (`BENE_ALLOWED_DATA_ROOTS`).  
   - Do **not** send `dataset_id`.  
   - This mode is mainly for co-located or trusted deployments where the client and server agree on shared storage.

#### Learning parameters (always required or optional as noted)

| Field | Required | Type | Description |
|-------|----------|------|-------------|
| `dataset_id` | * | string | Use when data was uploaded via `/v1/datasets`. |
| `vdfile` | * | string | Absolute path to vd file on server (with path mode). |
| `datafile` | * | string | Absolute path to data file on server (with path mode). |
| `variables` | yes | array of integers | **Ordered** list of **global column indices** (0 … number of columns − 1). Must be **unique**. Order defines **local** indices `0 … k-1` used in constraints. |
| `score` | yes | string | Decomposable score name, e.g. `BIC`, `AIC`, `HQC`, `fNML`, `qNML`, `LOO`, `BDe`, `BDq` (must match what bene supports). |
| `required_arcs` | no | array of `[src, dst]` | Directed edges that **must** appear (`src` → `dst`), using **global** column indices. |
| `forbidden_arcs` | no | array of `[src, dst]` | Directed edges that **must not** appear, using **global** indices. |
| `zeta` | no | boolean | Apply zeta transform before DP (default `false`). |
| `max_parents` | no | integer | Optional cap on parent set size (bene `-m`). |

\* Provide **either** (`dataset_id`) **or** (`vdfile` + `datafile`), not both.

#### Semantics important for clients

- **Induced subgraph:** learning uses only the columns listed in `variables`. Parents are only among those variables (not “rest of full graph” unless you extend bene elsewhere).
- **Global vs local indices:** `variables[i]` is global column `i`’s identity; position `i` in the array is **local index** `i`. Arcs in the response are given both as **`arcs_global`** (global column ids) and **`arcs_local`** (positions into the `variables` array).
- **Constraints:** `required_arcs` / `forbidden_arcs` use **global** column indices. An edge must refer to variables present in `variables`; otherwise the server returns **400**.

### Response

**200** with JSON:

| Field | Type | Description |
|-------|------|-------------|
| `score` | number | Total network score (bene `score_net` output). |
| `arcs_global` | array of `{ "src": int, "dst": int }` | Edges in **global** column indices. |
| `arcs_local` | array of `{ "src": int, "dst": int }` | Same edges in **local** indices (0 … `len(variables)-1`). |
| `work_dir` | string or null | Present only if server debug flag keeps pipeline temp dirs (normally `null`). |

### Example — learn after upload

```sh
DATASET_ID="<uuid-from-upload>"

curl -sS -X POST "${BASE}/v1/learn" \
  -H "Content-Type: application/json" \
  -d "{
    \"dataset_id\": \"${DATASET_ID}\",
    \"variables\": [0, 2, 4],
    \"score\": \"BIC\",
    \"forbidden_arcs\": [[0, 4]],
    \"required_arcs\": [],
    \"zeta\": false
  }"
```

### Example — path mode (server-side files)

```sh
curl -sS -X POST "${BASE}/v1/learn" \
  -H "Content-Type: application/json" \
  -d "{
    \"vdfile\": \"/data/registry/iris.vd\",
    \"datafile\": \"/data/registry/iris.idt\",
    \"variables\": [0, 1, 2, 3],
    \"score\": \"BIC\"
  }"
```

---

## Error responses

Common HTTP status codes:

| Code | Typical cause |
|------|----------------|
| **400** | Invalid JSON, validation error (e.g. both `dataset_id` and paths, missing `variables`, duplicate variable index, bad arc constraint). |
| **404** | `dataset_id` not found on server (expired or deleted). |
| **413** | Multipart upload exceeds `max_upload_bytes_total`. |
| **422** | Request body failed schema validation (FastAPI / Pydantic). |
| **500** | Missing bene binaries, logreg file, or pipeline failure (see `detail` message). |

Error bodies are usually JSON with a **`detail`** field (string or list of validation errors).

---

## Client workflow patterns

### A. Remote client (recommended for distributed workers)

1. `POST /v1/datasets` with local vd + data files → obtain `dataset_id`.
2. `POST /v1/learn` with `dataset_id`, `variables`, `score`, optional constraints.
3. Optionally `DELETE /v1/datasets/{dataset_id}` when finished, or rely on server TTL.

### B. Shared filesystem with server

1. Ensure paths are under server `BENE_ALLOWED_DATA_ROOTS`.
2. `POST /v1/learn` with `vdfile`, `datafile`, and learning parameters (no upload step).

---

## Limits and fairness

- **`variables` length** must be ≤ `max_subgraph_vars` from `/v1/info` (bene exact DP does not scale to large sets).
- Concurrent **`/v1/learn`** calls may queue server-side (`max_concurrent_jobs`). Heavy clients should backoff on **503** if you add a gateway that signals overload; the default server returns **500** on internal pipeline errors.

---

## OpenAPI and machine-readable spec

- **`GET /docs`** — Swagger UI (human-friendly).
- **`GET /openapi.json`** — full OpenAPI 3 schema for codegen (e.g. `openapi-generator`, client SDKs).

Use these when generating typed clients or when verifying field names against a specific server version.
