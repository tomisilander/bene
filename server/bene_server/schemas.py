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
    timeout_seconds: float | None = Field(
        default=None,
        description="Client max wait for this learn (seconds); capped by server max_learn_seconds",
    )

    @field_validator("variables")
    @classmethod
    def unique_ordered(cls, v: list[int]) -> list[int]:
        seen = set()
        for x in v:
            if x in seen:
                raise ValueError("variables must be unique")
            seen.add(x)
        return v

    @field_validator("timeout_seconds")
    @classmethod
    def timeout_positive(cls, v: float | None) -> float | None:
        if v is None:
            return None
        if v <= 0:
            raise ValueError("timeout_seconds must be positive when set")
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


class LocalScoreEntry(BaseModel):
    """Per-node family score for the learned network (same ordering as bene ``net`` / work dir)."""

    node_local: int = Field(..., description="Index into the request ``variables`` list")
    node_global: int = Field(..., description="Global column index")
    parent_set: int = Field(
        ...,
        description=(
            "Parent set as an unsigned integer bitmask in **learn-local** space only: "
            "bit j (0 ≤ j < k) is 1 iff the variable at local index j is a parent of "
            "``node_local``. ``k`` is ``len(variables)`` from the learn request. "
            "This is **not** a bitmask over global column indices."
        ),
    )
    parents_local: list[int] = Field(..., description="Parent indices in local numbering")
    parents_global: list[int] = Field(..., description="Parent column indices (global)")
    score: float = Field(..., description="Local family score (decomposable contribution)")


class LearnResponse(BaseModel):
    """Learned structure and decomposable score."""

    applied_timeout_seconds: float = Field(
        ...,
        description="Deadline used for this run (min of client timeout and server max)",
    )
    score: float = Field(..., description="Total network score from score_net")
    local_scores: list[LocalScoreEntry] = Field(
        ...,
        description="Per-node local scores for the returned DAG (sum equals ``score`` up to float noise)",
    )
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


class FamilyQuery(BaseModel):
    """One family: child variable and its parent set (global column indices)."""

    child: int = Field(..., description="Global column index of the child variable")
    parents: list[int] = Field(
        default_factory=list,
        description="Global column indices of parents (subset of ``variables``, must not include ``child``)",
    )


class ScoreFamiliesRequest(BaseModel):
    """Score specific (child, parents) families (no structure search). Each family uses a minimal variable set."""

    dataset_id: str | None = None
    vdfile: str | None = None
    datafile: str | None = None
    score: str = Field(..., description="Decomposable score name, e.g. BIC")
    families: list[FamilyQuery] = Field(..., min_length=1)
    timeout_seconds: float | None = Field(
        default=None,
        description="Optional client cap; combined with server max_learn_seconds",
    )

    @field_validator("timeout_seconds")
    @classmethod
    def timeout_positive_sf(cls, v: float | None) -> float | None:
        if v is None:
            return None
        if v <= 0:
            raise ValueError("timeout_seconds must be positive when set")
        return v

    @model_validator(mode="after")
    def dataset_or_paths_sf(self):
        has_id = self.dataset_id is not None and str(self.dataset_id).strip() != ""
        has_paths = (self.vdfile is not None and str(self.vdfile).strip() != "") and (
            self.datafile is not None and str(self.datafile).strip() != ""
        )
        if has_id and has_paths:
            raise ValueError("Provide either dataset_id or vdfile/datafile, not both")
        if not has_id and not has_paths:
            raise ValueError("Provide dataset_id or both vdfile and datafile")
        return self

    @model_validator(mode="after")
    def families_disjoint_union(self):
        for fq in self.families:
            if fq.child in fq.parents:
                raise ValueError("parents must not include child")
            dup = set()
            for p in fq.parents:
                if p in dup:
                    raise ValueError("duplicate parent in family")
                dup.add(p)
        return self


class FamilyScoreResult(BaseModel):
    """Score for one requested family."""

    child_local: int = Field(
        ...,
        description="Child index in **minimal-union** local order: ``sorted({child} ∪ parents)`` by global index.",
    )
    child_global: int
    parents_local: list[int] = Field(
        ...,
        description="Parent local indices in the same minimal union (not necessarily learn-local indices).",
    )
    parents_global: list[int]
    score: float


class ScoreFamiliesResponse(BaseModel):
    """Scores for each requested family (order matches ``families`` in the request)."""

    applied_timeout_seconds: float = Field(
        ...,
        description=(
            "Single wall-clock budget (seconds) for scoring **all** families in this request: "
            "min(server cap, optional ``timeout_seconds``). The server shares this deadline "
            "across subprocesses; families with the same variable-set union are batched into "
            "one ``score_families`` process (one data load per union)."
        ),
    )
    scores: list[FamilyScoreResult]
