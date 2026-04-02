"""Request/response models for the HTTP API."""

from pydantic import BaseModel, Field, field_validator, model_validator


class LearnRequest(BaseModel):
    """Exact structure learning on an induced subgraph (variables in ``S`` only)."""

    dataset_id: str | None = Field(
        default=None,
        description="Staged dataset id from POST /v1/datasets (omit vdfile/datafile when set)",
    )
    vdfile: str | None = Field(default=None, description="Path to variable description file (tab-separated)")
    datafile: str | None = Field(default=None, description="Path to discrete data file")
    variables: list[int] = Field(
        ...,
        description="Ordered global column indices (0 .. nof_cols-1); defines local indices 0..k-1",
        min_length=1,
    )
    score: str = Field(
        ...,
        description="Score criterion, e.g. BIC, AIC, HQC, fNML, qNML, LOO, BDe, BDq",
    )
    required_arcs: list[tuple[int, int]] = Field(
        default_factory=list,
        description="Edges that must be present (global variable indices)",
    )
    forbidden_arcs: list[tuple[int, int]] = Field(
        default_factory=list,
        description="Edges that must be absent (global variable indices)",
    )
    zeta: bool = Field(default=False, description="Apply zeta transform before DP")
    max_parents: int | None = Field(default=None, ge=1, description="Optional -m for get_local_scores")

    @field_validator("variables")
    @classmethod
    def unique_ordered(cls, v: list[int]) -> list[int]:
        seen = set()
        for x in v:
            if x in seen:
                raise ValueError("variables must be unique")
            seen.add(x)
        return v

    @model_validator(mode="after")
    def dataset_or_paths(self):
        has_id = self.dataset_id is not None and str(self.dataset_id).strip() != ""
        has_paths = (self.vdfile is not None and str(self.vdfile).strip() != "") and (
            self.datafile is not None and str(self.datafile).strip() != ""
        )
        if has_id and has_paths:
            raise ValueError("Provide either dataset_id or vdfile/datafile, not both")
        if not has_id and not has_paths:
            raise ValueError("Provide dataset_id or both vdfile and datafile")
        return self


class Arc(BaseModel):
    """Directed edge parent -> child in global variable indices."""

    src: int
    dst: int


class DatasetUploadResponse(BaseModel):
    """Result of POST /v1/datasets."""

    dataset_id: str
    vd_bytes: int
    data_bytes: int
    ttl_seconds: float
    message: str = (
        "Use dataset_id in POST /v1/learn. Data is removed after ttl_seconds or via DELETE /v1/datasets/{id}."
    )


class LearnResponse(BaseModel):
    """Learned structure and decomposable score."""

    score: float = Field(..., description="Total network score from score_net")
    arcs_global: list[Arc] = Field(
        ...,
        description="Directed edges using global column indices from the request",
    )
    arcs_local: list[Arc] = Field(
        ...,
        description="Same edges with local indices 0..k-1 in request variable order",
    )
    work_dir: str | None = Field(
        default=None,
        description="Temp directory used for this run (only if BENE_DEBUG_WORKDIR=1)",
    )
