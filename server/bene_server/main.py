"""FastAPI application entry point."""

from __future__ import annotations

import asyncio
import logging
import os
import uuid
from contextlib import asynccontextmanager
from pathlib import Path

import uvicorn
from fastapi import FastAPI, File, HTTPException, UploadFile
from pydantic import BaseModel

from bene_server.config import settings
from bene_server.datasets import (
    UploadTooLargeError,
    cleanup_expired,
    delete_dataset,
    ensure_staged_files_exist,
    save_multipart_pair,
)
from bene_server.paths import PathPolicyError, resolve_allowed_file
from bene_server.pipeline import (
    PipelineTimeoutError,
    globalize_arcs,
    run_subgraph_pipeline_deadline,
)
from bene_server.schemas import Arc, DatasetUploadResponse, LearnRequest, LearnResponse

logger = logging.getLogger(__name__)


@asynccontextmanager
async def lifespan(app: FastAPI):
    settings.dataset_staging_dir.mkdir(parents=True, exist_ok=True)
    try:
        n = await asyncio.to_thread(
            cleanup_expired,
            settings.dataset_staging_dir,
            settings.dataset_ttl_seconds,
        )
        if n:
            logger.info("Startup TTL cleanup removed %d staged dataset(s)", n)
    except OSError as e:
        logger.warning("Startup TTL cleanup failed: %s", e)

    async def cleanup_loop() -> None:
        while True:
            await asyncio.sleep(settings.cleanup_interval_seconds)
            try:
                removed = await asyncio.to_thread(
                    cleanup_expired,
                    settings.dataset_staging_dir,
                    settings.dataset_ttl_seconds,
                )
                if removed:
                    logger.info("TTL cleanup removed %d staged dataset(s)", removed)
            except OSError as e:
                logger.warning("TTL cleanup failed: %s", e)

    task = asyncio.create_task(cleanup_loop())
    try:
        yield
    finally:
        task.cancel()
        try:
            await task
        except asyncio.CancelledError:
            pass


app = FastAPI(
    title="bene server",
    description="Exact constrained Bayesian network structure learning via the bene pipeline",
    version="0.1.0",
    lifespan=lifespan,
)

_learn_semaphore = asyncio.Semaphore(settings.max_concurrent_jobs)


class HealthResponse(BaseModel):
    status: str


class InfoResponse(BaseModel):
    bin_dir: str
    max_subgraph_vars: int
    max_concurrent_jobs: int
    max_learn_seconds: float
    allowed_data_roots_configured: bool
    dataset_staging_dir: str
    max_upload_bytes_total: int
    dataset_ttl_seconds: float
    cleanup_interval_seconds: float


@app.get("/health", response_model=HealthResponse)
async def health() -> HealthResponse:
    return HealthResponse(status="ok")


@app.get("/v1/info", response_model=InfoResponse)
async def info() -> InfoResponse:
    roots = settings.allowed_roots()
    return InfoResponse(
        bin_dir=str(settings.bin_dir.resolve()),
        max_subgraph_vars=settings.max_subgraph_vars,
        max_concurrent_jobs=settings.max_concurrent_jobs,
        max_learn_seconds=settings.max_learn_seconds,
        allowed_data_roots_configured=len(roots) > 0,
        dataset_staging_dir=str(settings.dataset_staging_dir.resolve()),
        max_upload_bytes_total=settings.max_upload_bytes_total,
        dataset_ttl_seconds=settings.dataset_ttl_seconds,
        cleanup_interval_seconds=settings.cleanup_interval_seconds,
    )


def _validate_arc_lists(
    required: list[tuple[int, int]],
    forbidden: list[tuple[int, int]],
) -> None:
    req_set = set(required)
    for edge in forbidden:
        if edge in req_set:
            raise HTTPException(
                status_code=400,
                detail=f"Arc {edge} appears in both required and forbidden",
            )
    for s, d in required + forbidden:
        if s == d:
            raise HTTPException(status_code=400, detail=f"Self-loop not allowed: ({s}, {d})")


def _resolve_vd_data_paths(body: LearnRequest) -> tuple[Path, Path]:
    if body.dataset_id is not None and str(body.dataset_id).strip():
        ds = str(body.dataset_id).strip()
        try:
            return ensure_staged_files_exist(settings.dataset_staging_dir, ds)
        except ValueError:
            raise HTTPException(status_code=400, detail="invalid dataset_id") from None
        except FileNotFoundError as e:
            raise HTTPException(status_code=404, detail=str(e)) from e

    assert body.vdfile is not None and body.datafile is not None
    allowed_roots = settings.allowed_roots()
    try:
        vd_path = resolve_allowed_file(body.vdfile, allowed_roots)
        data_path = resolve_allowed_file(body.datafile, allowed_roots)
    except PathPolicyError as e:
        raise HTTPException(status_code=400, detail=str(e)) from e
    return vd_path, data_path


@app.post("/v1/datasets", response_model=DatasetUploadResponse)
async def upload_dataset(
    vd: UploadFile = File(..., description="Variable description (.vd)"),
    data: UploadFile = File(..., description="Discrete data file"),
) -> DatasetUploadResponse:
    """Upload vd + data; receive a ``dataset_id`` for use in ``POST /v1/learn``."""
    dataset_id = str(uuid.uuid4())
    settings.dataset_staging_dir.mkdir(parents=True, exist_ok=True)
    try:
        vd_bytes, data_bytes = await save_multipart_pair(
            staging_root=settings.dataset_staging_dir,
            dataset_id=dataset_id,
            vd_stream=vd,
            data_stream=data,
            max_total_bytes=settings.max_upload_bytes_total,
        )
    except UploadTooLargeError:
        raise HTTPException(
            status_code=413,
            detail=f"Combined upload exceeds max_upload_bytes_total={settings.max_upload_bytes_total}",
        ) from None

    return DatasetUploadResponse(
        dataset_id=dataset_id,
        vd_bytes=vd_bytes,
        data_bytes=data_bytes,
        ttl_seconds=settings.dataset_ttl_seconds,
    )


@app.delete("/v1/datasets/{dataset_id}", status_code=204)
async def remove_dataset(dataset_id: str) -> None:
    """Delete a staged dataset immediately (before TTL)."""
    try:
        existed = await asyncio.to_thread(delete_dataset, settings.dataset_staging_dir, dataset_id)
    except ValueError:
        raise HTTPException(status_code=400, detail="invalid dataset_id") from None
    if not existed:
        raise HTTPException(status_code=404, detail="dataset not found")


@app.post("/v1/learn", response_model=LearnResponse)
async def learn(body: LearnRequest) -> LearnResponse:
    if len(body.variables) > settings.max_subgraph_vars:
        raise HTTPException(
            status_code=400,
            detail=f"|variables|={len(body.variables)} exceeds max_subgraph_vars={settings.max_subgraph_vars}",
        )

    _validate_arc_lists(body.required_arcs, body.forbidden_arcs)

    vd_path, data_path = _resolve_vd_data_paths(body)

    logreg = settings.resolved_logreg()
    if not logreg.is_file():
        raise HTTPException(
            status_code=500,
            detail=f"logreg file not found: {logreg} (set BENE_LOGREG_FILE or build bene)",
        )

    keep_work = os.environ.get("BENE_DEBUG_WORKDIR", "") == "1"

    effective_timeout = float(settings.max_learn_seconds)
    if body.timeout_seconds is not None:
        effective_timeout = min(effective_timeout, float(body.timeout_seconds))

    pl_kwargs = dict(
        vd_path=vd_path,
        data_path=data_path,
        global_vars=body.variables,
        score=body.score,
        required_arcs=body.required_arcs,
        forbidden_arcs=body.forbidden_arcs,
        bin_dir=settings.bin_dir,
        logreg_path=logreg,
        zeta=body.zeta,
        max_parents=body.max_parents,
        keep_workdir=keep_work,
    )

    async with _learn_semaphore:
        try:

            def _run_learn():
                return run_subgraph_pipeline_deadline(effective_timeout, **pl_kwargs)

            result = await asyncio.to_thread(_run_learn)
        except PipelineTimeoutError as e:
            raise HTTPException(
                status_code=504,
                detail=(
                    f"Learn exceeded deadline {e.deadline_seconds}s "
                    f"(server max_learn_seconds={settings.max_learn_seconds}; "
                    "client may set a lower timeout_seconds)"
                ),
            ) from e
        except ValueError as e:
            raise HTTPException(status_code=400, detail=str(e)) from e
        except FileNotFoundError as e:
            raise HTTPException(status_code=500, detail=str(e)) from e
        except RuntimeError as e:
            raise HTTPException(status_code=500, detail=str(e)) from e

    arcs_g = globalize_arcs(result.arcs_local, body.variables)
    work_dir_str = str(result.work_dir) if result.work_dir is not None else None

    return LearnResponse(
        applied_timeout_seconds=effective_timeout,
        score=result.score,
        arcs_global=[Arc(src=a, dst=b) for a, b in arcs_g],
        arcs_local=[Arc(src=a, dst=b) for a, b in result.arcs_local],
        work_dir=work_dir_str,
    )


def run() -> None:
    """Console script entry: ``bene-server``."""
    uvicorn.run(
        "bene_server.main:app",
        host=os.environ.get("BENE_HOST", "127.0.0.1"),
        port=int(os.environ.get("BENE_PORT", "8765")),
        reload=False,
    )


if __name__ == "__main__":
    run()
