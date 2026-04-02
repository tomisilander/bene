"""Multipart dataset staging, lookup, and TTL cleanup."""

from __future__ import annotations

import json
import logging
import shutil
import time
import uuid
from pathlib import Path

logger = logging.getLogger(__name__)


class UploadTooLargeError(Exception):
    """Uploaded content exceeds the configured total byte limit."""


VD_NAME = "vars.vd"
DATA_NAME = "data.idt"
META_NAME = "meta.json"


def is_valid_dataset_id(s: str) -> bool:
    try:
        uuid.UUID(hex=s)
        return True
    except (ValueError, AttributeError):
        return False


def dataset_dir(staging_root: Path, dataset_id: str) -> Path:
    if not is_valid_dataset_id(dataset_id):
        raise ValueError("invalid dataset_id")
    return staging_root.resolve() / dataset_id


def paths_for_dataset(staging_root: Path, dataset_id: str) -> tuple[Path, Path]:
    """Return (vd_path, data_path) for a staged dataset."""
    root = dataset_dir(staging_root, dataset_id)
    vd = root / VD_NAME
    data = root / DATA_NAME
    return vd, data


def write_meta(staging_root: Path, dataset_id: str, vd_bytes: int, data_bytes: int) -> None:
    d = dataset_dir(staging_root, dataset_id)
    d.mkdir(parents=True, exist_ok=True)
    meta = {
        "uploaded_at": time.time(),
        "vd_bytes": vd_bytes,
        "data_bytes": data_bytes,
    }
    meta_path = d / META_NAME
    meta_path.write_text(json.dumps(meta), encoding="ascii")


async def save_multipart_pair(
    *,
    staging_root: Path,
    dataset_id: str,
    vd_stream,
    data_stream,
    max_total_bytes: int,
) -> tuple[int, int]:
    """
    Stream two uploads to ``staging_root/<dataset_id>/`` with a shared byte budget.

    ``vd_stream`` and ``data_stream`` are async file-like objects with ``read`` (e.g. Starlette ``UploadFile``).
    """
    root = dataset_dir(staging_root, dataset_id)
    root.mkdir(parents=True, exist_ok=True)
    vd_path = root / VD_NAME
    data_path = root / DATA_NAME

    remaining = max_total_bytes

    async def pump(src, dest: Path) -> int:
        nonlocal remaining
        written = 0
        chunk_size = 1024 * 1024
        with dest.open("wb") as f:
            while True:
                chunk = await src.read(chunk_size)
                if not chunk:
                    break
                if len(chunk) > remaining:
                    raise UploadTooLargeError()
                remaining -= len(chunk)
                f.write(chunk)
                written += len(chunk)
        return written

    try:
        vd_written = await pump(vd_stream, vd_path)
        data_written = await pump(data_stream, data_path)
        write_meta(staging_root, dataset_id, vd_written, data_written)
        return vd_written, data_written
    except Exception:
        shutil.rmtree(root, ignore_errors=True)
        raise


def cleanup_expired(staging_root: Path, ttl_seconds: float) -> int:
    """
    Remove dataset directories older than ``ttl_seconds`` (by directory mtime).
    Returns the number of directories removed.
    """
    staging_root = staging_root.resolve()
    if not staging_root.is_dir():
        return 0

    now = time.time()
    removed = 0
    for entry in staging_root.iterdir():
        if not entry.is_dir():
            continue
        if not is_valid_dataset_id(entry.name):
            continue
        try:
            age = now - entry.stat().st_mtime
        except OSError:
            continue
        if age <= ttl_seconds:
            continue
        try:
            shutil.rmtree(entry)
            removed += 1
        except OSError as e:
            logger.warning("Failed to remove %s: %s", entry, e)
    return removed


def delete_dataset(staging_root: Path, dataset_id: str) -> bool:
    """Remove a staged dataset. Returns True if the directory existed."""
    root = dataset_dir(staging_root, dataset_id)
    if not root.is_dir():
        return False
    shutil.rmtree(root)
    return True


def ensure_staged_files_exist(staging_root: Path, dataset_id: str) -> tuple[Path, Path]:
    vd, data = paths_for_dataset(staging_root, dataset_id)
    if not vd.is_file() or not data.is_file():
        raise FileNotFoundError(f"Staged dataset not found or incomplete: {dataset_id}")
    return vd, data
