"""Shared fixtures for bene-server tests."""

from pathlib import Path

import pytest


@pytest.fixture(scope="session")
def repo_root() -> Path:
    """Repository root (parent of ``server/``)."""
    return Path(__file__).resolve().parents[2]


@pytest.fixture(scope="session")
def iris_vd(repo_root: Path) -> Path:
    return repo_root / "tests" / "data" / "iris" / "iris.vd"


@pytest.fixture(scope="session")
def iris_idt(repo_root: Path) -> Path:
    return repo_root / "tests" / "data" / "iris" / "iris.idt"
