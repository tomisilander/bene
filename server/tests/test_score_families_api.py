"""POST /v1/score-families and local_scores on /v1/learn."""

from pathlib import Path

import pytest
from fastapi.testclient import TestClient

from bene_server.config import settings as app_settings
from bene_server.main import app


def test_learn_includes_local_scores_that_sum_to_total(
    repo_root: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    if not (repo_root / "bin" / "net_local_scores").is_file():
        pytest.skip("net_local_scores not built")
    monkeypatch.setattr(
        app_settings,
        "allowed_data_roots",
        str(repo_root / "tests" / "data"),
    )
    iris_vd = repo_root / "tests" / "data" / "iris" / "iris.vd"
    iris_idt = repo_root / "tests" / "data" / "iris" / "iris.idt"
    c = TestClient(app)
    r = c.post(
        "/v1/learn",
        json={
            "vdfile": str(iris_vd),
            "datafile": str(iris_idt),
            "variables": [0, 1, 2, 3, 4],
            "score": "BIC",
        },
    )
    assert r.status_code == 200, r.text
    body = r.json()
    assert "local_scores" in body
    assert len(body["local_scores"]) == 5
    ssum = sum(x["score"] for x in body["local_scores"])
    assert abs(ssum - body["score"]) < 1e-4


def test_score_families_endpoint(
    repo_root: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    if not (repo_root / "bin" / "score_families").is_file():
        pytest.skip("score_families not built")
    monkeypatch.setattr(
        app_settings,
        "allowed_data_roots",
        str(repo_root / "tests" / "data"),
    )
    iris_vd = repo_root / "tests" / "data" / "iris" / "iris.vd"
    iris_idt = repo_root / "tests" / "data" / "iris" / "iris.idt"
    c = TestClient(app)
    r = c.post(
        "/v1/score-families",
        json={
            "vdfile": str(iris_vd),
            "datafile": str(iris_idt),
            "score": "BIC",
            "families": [
                {"child": 2, "parents": [0, 1]},
            ],
        },
    )
    assert r.status_code == 200, r.text
    data = r.json()
    assert len(data["scores"]) == 1
    assert data["scores"][0]["child_global"] == 2
    assert set(data["scores"][0]["parents_global"]) == {0, 1}
    assert isinstance(data["scores"][0]["score"], float)


def test_score_families_batches_families_with_same_union(
    repo_root: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Two families sharing the same {child}∪parents run in one score_families process."""
    if not (repo_root / "bin" / "score_families").is_file():
        pytest.skip("score_families not built")
    monkeypatch.setattr(
        app_settings,
        "allowed_data_roots",
        str(repo_root / "tests" / "data"),
    )
    iris_vd = repo_root / "tests" / "data" / "iris" / "iris.vd"
    iris_idt = repo_root / "tests" / "data" / "iris" / "iris.idt"
    c = TestClient(app)
    r = c.post(
        "/v1/score-families",
        json={
            "vdfile": str(iris_vd),
            "datafile": str(iris_idt),
            "score": "BIC",
            "families": [
                {"child": 2, "parents": [0, 1]},
                {"child": 0, "parents": [1, 2]},
            ],
        },
    )
    assert r.status_code == 200, r.text
    data = r.json()
    assert len(data["scores"]) == 2
    assert {data["scores"][0]["child_global"], data["scores"][1]["child_global"]} == {0, 2}


def test_learn_local_scores_match_score_families_parity(
    repo_root: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Each learn local_scores row matches score-families for the same (child, parents) globals."""
    if not (repo_root / "bin" / "net_local_scores").is_file():
        pytest.skip("net_local_scores not built")
    if not (repo_root / "bin" / "score_families").is_file():
        pytest.skip("score_families not built")
    monkeypatch.setattr(
        app_settings,
        "allowed_data_roots",
        str(repo_root / "tests" / "data"),
    )
    iris_vd = repo_root / "tests" / "data" / "iris" / "iris.vd"
    iris_idt = repo_root / "tests" / "data" / "iris" / "iris.idt"
    c = TestClient(app)
    r_learn = c.post(
        "/v1/learn",
        json={
            "vdfile": str(iris_vd),
            "datafile": str(iris_idt),
            "variables": [0, 1, 2, 3, 4],
            "score": "BIC",
        },
    )
    assert r_learn.status_code == 200, r_learn.text
    learn = r_learn.json()
    for row in learn["local_scores"]:
        r_sf = c.post(
            "/v1/score-families",
            json={
                "vdfile": str(iris_vd),
                "datafile": str(iris_idt),
                "score": "BIC",
                "families": [
                    {
                        "child": row["node_global"],
                        "parents": row["parents_global"],
                    },
                ],
            },
        )
        assert r_sf.status_code == 200, r_sf.text
        sf_score = r_sf.json()["scores"][0]["score"]
        assert abs(sf_score - row["score"]) < 1e-4
