"""Multipart dataset upload and TTL helpers."""

from pathlib import Path

import pytest
from fastapi.testclient import TestClient

from bene_server.config import settings as app_settings
from bene_server.main import app


def test_upload_saves_under_staging(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(app_settings, "dataset_staging_dir", tmp_path)
    monkeypatch.setattr(app_settings, "max_upload_bytes_total", 1_000_000)
    c = TestClient(app)
    r = c.post(
        "/v1/datasets",
        files={
            "vd": ("iris.vd", b"v0\t0\t1\n"),
            "data": ("iris.idt", b"0\n1\n"),
        },
    )
    assert r.status_code == 200, r.text
    payload = r.json()
    did = payload["dataset_id"]
    root = tmp_path / did
    assert (root / "vars.vd").read_bytes() == b"v0\t0\t1\n"
    assert (root / "data.idt").read_bytes() == b"0\n1\n"
    assert payload["vd_bytes"] == len(b"v0\t0\t1\n")
    assert payload["data_bytes"] == len(b"0\n1\n")


def test_upload_rejects_oversized(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(app_settings, "dataset_staging_dir", tmp_path)
    monkeypatch.setattr(app_settings, "max_upload_bytes_total", 5)
    c = TestClient(app)
    r = c.post(
        "/v1/datasets",
        files={
            "vd": ("a.vd", b"12345"),
            "data": ("a.idt", b"6"),
        },
    )
    assert r.status_code == 413


def test_delete_dataset(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(app_settings, "dataset_staging_dir", tmp_path)
    monkeypatch.setattr(app_settings, "max_upload_bytes_total", 1_000_000)
    c = TestClient(app)
    r = c.post(
        "/v1/datasets",
        files={"vd": ("a.vd", b"x\n"), "data": ("a.idt", b"y\n")},
    )
    did = r.json()["dataset_id"]
    assert (tmp_path / did).is_dir()
    d = c.delete(f"/v1/datasets/{did}")
    assert d.status_code == 204
    assert not (tmp_path / did).exists()


def test_learn_with_dataset_id_json(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    repo_root: Path,
) -> None:
    """POST /v1/learn with dataset_id (requires built bene binaries)."""
    bin_dir = repo_root / "bin"
    if not (bin_dir / "get_local_scores").is_file():
        pytest.skip("bene binaries not built")

    iris_vd = repo_root / "tests" / "data" / "iris" / "iris.vd"
    iris_idt = repo_root / "tests" / "data" / "iris" / "iris.idt"

    monkeypatch.setattr(app_settings, "dataset_staging_dir", tmp_path)
    monkeypatch.setattr(app_settings, "max_upload_bytes_total", 50_000_000)

    c = TestClient(app)
    up = c.post(
        "/v1/datasets",
        files={
            "vd": ("iris.vd", iris_vd.read_bytes()),
            "data": ("iris.idt", iris_idt.read_bytes()),
        },
    )
    assert up.status_code == 200, up.text
    did = up.json()["dataset_id"]

    learn = c.post(
        "/v1/learn",
        json={
            "dataset_id": did,
            "variables": [0, 2, 4],
            "score": "BIC",
        },
    )
    assert learn.status_code == 200, learn.text
    body = learn.json()
    assert "score" in body
    assert "arcs_global" in body
