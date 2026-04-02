"""Learn deadline and /v1/info fields."""

from pathlib import Path

import pytest
from fastapi.testclient import TestClient

from bene_server.config import settings as app_settings
from bene_server.main import app


def test_info_includes_max_learn_seconds() -> None:
    c = TestClient(app)
    r = c.get("/v1/info")
    assert r.status_code == 200
    body = r.json()
    assert "max_learn_seconds" in body
    assert float(body["max_learn_seconds"]) >= 1.0


def test_learn_applied_timeout_is_min_of_server_and_client(
    repo_root: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    bin_dir = repo_root / "bin"
    if not (bin_dir / "get_local_scores").is_file():
        pytest.skip("bene binaries not built")

    monkeypatch.setattr(
        app_settings,
        "allowed_data_roots",
        str(repo_root / "tests" / "data"),
    )
    monkeypatch.setattr(app_settings, "max_learn_seconds", 500.0)

    iris_vd = repo_root / "tests" / "data" / "iris" / "iris.vd"
    iris_idt = repo_root / "tests" / "data" / "iris" / "iris.idt"

    c = TestClient(app)
    r = c.post(
        "/v1/learn",
        json={
            "vdfile": str(iris_vd),
            "datafile": str(iris_idt),
            "variables": [0, 1, 2],
            "score": "BIC",
            "timeout_seconds": 60,
        },
    )
    assert r.status_code == 200, r.text
    assert r.json()["applied_timeout_seconds"] == 60.0
