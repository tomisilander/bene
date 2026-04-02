"""Validate dataset paths against configured allowed roots."""

from pathlib import Path


class PathPolicyError(ValueError):
    """Raised when a path is outside allowed roots or not a regular file."""


def _is_under(path: Path, root: Path) -> bool:
    try:
        path.relative_to(root)
        return True
    except ValueError:
        return False


def resolve_allowed_file(path_str: str, allowed_roots: list[Path]) -> Path:
    """
    Resolve ``path_str`` to an absolute path and ensure it is a file under one of
    ``allowed_roots``. If ``allowed_roots`` is empty, reject (policy must be configured).
    """
    if not allowed_roots:
        raise PathPolicyError(
            "BENE_ALLOWED_DATA_ROOTS is not set; refusing to read arbitrary paths. "
            "Set it to a comma-separated list of directory roots (e.g. the repo example/ and tests/data/)."
        )
    path = Path(path_str).expanduser().resolve(strict=False)
    if not path.is_file():
        raise PathPolicyError(f"Not a regular file or missing: {path}")
    for root in allowed_roots:
        if _is_under(path, root):
            return path
    roots_display = ", ".join(str(r) for r in allowed_roots)
    raise PathPolicyError(f"Path {path} is not under any allowed root: {roots_display}")
