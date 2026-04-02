"""Server configuration from environment variables."""

from pathlib import Path

from pydantic import Field
from pydantic_settings import BaseSettings, SettingsConfigDict


def _default_bin_dir() -> Path:
    """Resolve ``.../bene/bin`` from this package location."""
    return Path(__file__).resolve().parents[2] / "bin"


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
    max_subgraph_vars: int = Field(default=32, ge=1, le=64, description="Maximum |S| per request")
    max_concurrent_jobs: int = Field(default=2, ge=1, description="Concurrent pipeline runs")

    def resolved_logreg(self) -> Path:
        path = self.logreg_file or (self.bin_dir / "logreg256x2000.bin")
        return path.resolve()

    def allowed_roots(self) -> list[Path]:
        raw = [p.strip() for p in self.allowed_data_roots.split(",") if p.strip()]
        if not raw:
            return []
        return [Path(p).resolve() for p in raw]


settings = Settings()
