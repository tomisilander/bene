"""Server configuration from environment variables."""

import tempfile
from pathlib import Path

from pydantic import Field
from pydantic_settings import BaseSettings, SettingsConfigDict


def _default_bin_dir() -> Path:
    """Resolve ``.../bene/bin`` from this package location."""
    return Path(__file__).resolve().parents[2] / "bin"


def _default_dataset_staging_dir() -> Path:
    return Path(tempfile.gettempdir()) / "bene-server-datasets"


class Settings(BaseSettings):
    model_config = SettingsConfigDict(env_prefix="BENE_", env_file=".env", extra="ignore")

    bin_dir: Path = Field(default_factory=_default_bin_dir, description="Directory with bene binaries")
    logreg_file: Path | None = Field(
        default=None,
        description="Path to logreg256x2000.bin (defaults to bin_dir/logreg256x2000.bin)",
    )
    allowed_data_roots: str = Field(
        default="",
        description="Comma-separated list of filesystem roots; vd/data paths must resolve under one of them",
    )
    dataset_staging_dir: Path = Field(
        default_factory=_default_dataset_staging_dir,
        description="Directory for POST /v1/datasets uploads (UUID subdirs)",
    )
    max_upload_bytes_total: int = Field(
        default=50_000_000,
        ge=1024,
        description="Max combined size (bytes) of vd + data in one multipart upload",
    )
    dataset_ttl_seconds: float = Field(
        default=86_400.0,
        ge=60.0,
        description="Remove staged datasets older than this (mtime-based)",
    )
    cleanup_interval_seconds: float = Field(
        default=3_600.0,
        ge=60.0,
        description="How often to run TTL cleanup in the background",
    )
    max_subgraph_vars: int = Field(default=32, ge=1, le=64, description="Maximum |S| per request")
    max_concurrent_jobs: int = Field(default=2, ge=1, description="Concurrent pipeline runs")
    max_learn_seconds: float = Field(
        default=3600.0,
        ge=1.0,
        description="Server-wide cap on POST /v1/learn wall time (child process terminated if exceeded)",
    )

    def resolved_logreg(self) -> Path:
        path = self.logreg_file or (self.bin_dir / "logreg256x2000.bin")
        return path.resolve()

    def allowed_roots(self) -> list[Path]:
        raw = [p.strip() for p in self.allowed_data_roots.split(",") if p.strip()]
        if not raw:
            return []
        return [Path(p).resolve() for p in raw]


settings = Settings()
