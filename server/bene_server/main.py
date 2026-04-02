"""FastAPI application entry point."""

from __future__ import annotations

import asyncio
import os

import uvicorn
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel

from bene_server.config import settings
from bene_server.paths import PathPolicyError, resolve_allowed_file
from bene_server.pipeline import globalize_arcs, run_subgraph_pipeline
from bene_server.schemas import Arc, LearnRequest, LearnResponse

app = FastAPI(
    title="bene server",
    description="Exact constrained Bayesian network structure learning via the bene pipeline",
    version="0.1.0",
)

_learn_semaphore = asyncio.Semaphore(settings.max_concurrent_jobs)


class HealthResponse(BaseModel):
    status: str


class InfoResponse(BaseModel):
    bin_dir: str
    max_subgraph_vars: int
    max_concurrent_jobs: int
    allowed_data_roots_configured: bool


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
        allowed_data_roots_configured=len(roots) > 0,
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


@app.post("/v1/learn", response_model=LearnResponse)
async def learn(body: LearnRequest) -> LearnResponse:
    if len(body.variables) > settings.max_subgraph_vars:
        raise HTTPException(
            status_code=400,
            detail=f"|variables|={len(body.variables)} exceeds max_subgraph_vars={settings.max_subgraph_vars}",
        )

    _validate_arc_lists(body.required_arcs, body.forbidden_arcs)

    allowed_roots = settings.allowed_roots()
    try:
        vd_path = resolve_allowed_file(body.vdfile, allowed_roots)
        data_path = resolve_allowed_file(body.datafile, allowed_roots)
    except PathPolicyError as e:
        raise HTTPException(status_code=400, detail=str(e)) from e

    logreg = settings.resolved_logreg()
    if not logreg.is_file():
        raise HTTPException(
            status_code=500,
            detail=f"logreg file not found: {logreg} (set BENE_LOGREG_FILE or build bene)",
        )

    keep_work = os.environ.get("BENE_DEBUG_WORKDIR", "") == "1"

    async with _learn_semaphore:
        try:
            result = await asyncio.to_thread(
                run_subgraph_pipeline,
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
        except ValueError as e:
            raise HTTPException(status_code=400, detail=str(e)) from e
        except FileNotFoundError as e:
            raise HTTPException(status_code=500, detail=str(e)) from e
        except RuntimeError as e:
            raise HTTPException(status_code=500, detail=str(e)) from e

    arcs_g = globalize_arcs(result.arcs_local, body.variables)
    work_dir_str = str(result.work_dir) if result.work_dir is not None else None

    return LearnResponse(
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
