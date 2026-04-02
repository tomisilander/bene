"""Tests for global column index ↔ local subgraph index mapping."""

from __future__ import annotations

from pathlib import Path

import pytest

from bene_server.pipeline import globalize_arcs, run_subgraph_pipeline


def test_globalize_arcs_nonconsecutive_order() -> None:
    """Local indices refer to positions in ``variables``, not sorted global ids."""
    variables = [0, 2, 4]
    arcs_local = [(1, 0), (2, 1)]
    expected = [(2, 0), (4, 2)]
    assert globalize_arcs(arcs_local, variables) == expected


def test_globalize_arcs_single_gap() -> None:
    variables = [1, 3, 5, 7]
    arcs_local = [(0, 2)]
    assert globalize_arcs(arcs_local, variables) == [(1, 5)]


@pytest.mark.integration
def test_pipeline_nonconsecutive_global_endpoints(
    iris_vd: Path,
    iris_idt: Path,
    repo_root: Path,
) -> None:
    """
    Full pipeline with non-consecutive ``variables``; response arcs must use
    global column ids matching ``globalize_arcs`` (order of ``variables``).
    """
    bin_dir = repo_root / "bin"
    logreg = bin_dir / "logreg256x2000.bin"
    if not (bin_dir / "get_local_scores").is_file():
        pytest.skip("bene binaries not built (run build/build.sh)")
    if not logreg.is_file():
        pytest.skip("logreg file missing")

    variables = [0, 2, 4]
    result = run_subgraph_pipeline(
        vd_path=iris_vd,
        data_path=iris_idt,
        global_vars=variables,
        score="BIC",
        required_arcs=[],
        forbidden_arcs=[],
        bin_dir=bin_dir,
        logreg_path=logreg,
        zeta=False,
        max_parents=None,
        keep_workdir=False,
    )

    var_set = set(variables)
    for src_l, dst_l in result.arcs_local:
        assert 0 <= src_l < len(variables)
        assert 0 <= dst_l < len(variables)

    actual_global = globalize_arcs(result.arcs_local, variables)
    manual = [(variables[s], variables[d]) for s, d in result.arcs_local]
    assert manual == actual_global

    for src_g, dst_g in actual_global:
        assert src_g in var_set
        assert dst_g in var_set
