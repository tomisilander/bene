#!/usr/bin/env bash
set -euo pipefail

# usage: run_regression.sh [-s]
# -s  save current outputs as the new baseline rather than comparing
#     against existing data.  Saved outputs are written under
#     tests/regression/expected/<dataset>/<score>.


# parse options
save_baseline=0
while getopts "s" opt; do
    case $opt in
        s) save_baseline=1 ;;
        *) echo "Usage: $0 [-s]" >&2; exit 1 ;;
    esac
done
shift $((OPTIND -1))

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
DATA_DIR="$ROOT/tests/data"
BIN_DIR="$ROOT/bin"

REQUIRED=(
  data2net.sh
  get_local_scores
  split_local_scores
  reverse_local_scores
  get_best_parents
  get_best_sinks
  get_best_order
  get_best_net
  score_net
)

missing_bins=()
for b in "${REQUIRED[@]}"; do
  if [ ! -x "$BIN_DIR/$b" ]; then
    missing_bins+=("$b")
  fi
done

if [ ${#missing_bins[@]} -ne 0 ]; then
  echo "Warning: missing required binaries in $BIN_DIR: ${missing_bins[*]}"
  echo "Tests will skip datasets that require those binaries."
fi

failed=()
passed=()
skipped=()

# define the set of scoring criteria we want to exercise; these
# correspond to the options recognised by get_local_scores.c's
# init_scorer() function
SCORES=(BIC AIC HQC fNML qNML LOO 0.5 1.0 2.0 1.0Q 1.0L)

for ds in "$DATA_DIR"/*; do
  [ -d "$ds" ] || continue
  name=$(basename "$ds")
  echo "\n=== Dataset: $name ==="

  vd_file=$(ls "$ds"/*.vd 2>/dev/null | head -n1 || true)
  idt_file=$(ls "$ds"/*.idt 2>/dev/null | head -n1 || true)
  if [ -z "$vd_file" ] || [ -z "$idt_file" ]; then
    echo "Skipping $name: missing .vd or .idt file"
    skipped+=("$name")
    continue
  fi

  # ensure required bins exist for running data2net.sh
  if [ ! -x "$BIN_DIR/data2net.sh" ]; then
    echo "Skipping $name: $BIN_DIR/data2net.sh not executable"
    skipped+=("$name")
    continue
  fi

  # create a result dir preferring under the repo root; fall back to system /tmp
  if rdir=$(mktemp -d "$ROOT/resdir_${name}.XXXX" 2>/dev/null); then
    true
  elif rdir=$(mktemp -d -t "resdir_${name}.XXXX" 2>/dev/null); then
    true
  else
    rdir="$ROOT/resdir_${name}.$$.$RANDOM"
    mkdir -p "$rdir"
  fi
  echo "Using result dir: $rdir"

  # run data2net for each supported score type, record per-score results
  for score in "${SCORES[@]}"; do
    echo "  -> score = $score"
    score_rdir="$rdir/$score"
    mkdir -p "$score_rdir"

    score_failed=0

    set +e
    out=$("$BIN_DIR/data2net.sh" "$vd_file" "$idt_file" "$score" "$score_rdir" 2>&1)
    rc=$?
    set -e

    echo "$out"

    if [ $rc -ne 0 ]; then
      echo "FAIL: data2net.sh exited $rc for dataset $name (score $score)"
      echo "$out" > "$score_rdir/run.log"
      failed+=("${name}:${score}")
      score_failed=1
    fi

    if [ $score_failed -eq 0 ] && [ ! -f "$score_rdir/net" ]; then
      echo "FAIL: expected $score_rdir/net does not exist for dataset $name (score $score)"
      echo "$out" > "$score_rdir/run.log"
      failed+=("${name}:${score}")
      score_failed=1
    fi

    if [ $score_failed -eq 0 ]; then
      if [ "$save_baseline" -eq 1 ]; then
        # save the standard output and ascii net representation
        expdir="$ROOT/tests/regression/expected/$name/$score"
        mkdir -p "$expdir"
        printf "%s" "$out" > "$expdir/stdout.txt"
        $BIN_DIR/net2parents "$score_rdir/net" - | $BIN_DIR/parents2arcs - - > "$expdir/net.txt"
        echo "SAVED baseline for $name (score $score)"
      else
        # compare to existing baseline if present
        expdir="$ROOT/tests/regression/expected/$name/$score"
        if [ -d "$expdir" ]; then
          # compare stdout
          if ! diff -u "$expdir/stdout.txt" <(printf "%s" "$out"); then
            echo "FAIL: stdout differs for $name (score $score)"
            failed+=("${name}:${score}")
            score_failed=1
          fi
          # compare ascii net
          tmpnet=$(mktemp)
          $BIN_DIR/net2parents "$score_rdir/net" - | $BIN_DIR/parents2arcs - - > "$tmpnet"
          if ! diff -u "$expdir/net.txt" "$tmpnet"; then
            echo "FAIL: net layout differs for $name (score $score)"
            failed+=("${name}:${score}")
            score_failed=1
          fi
          rm -f "$tmpnet"
        else
          echo "No baseline for $name (score $score); skipping comparison"
        fi
      fi
    fi

    if [ $score_failed -eq 0 ]; then
      echo "PASS: $name (score $score)"
      passed+=("${name}:${score}")
    fi

    # clean up successful run
    rm -rf "$score_rdir"
  done

  # remove the temporary result directory now that all score-specific
  # subdirectories have been handled.  preserves parent for debugging
  rm -rf "$rdir"
done

# Summary
echo "\n=== Summary ==="
echo "Passed: ${#passed[@]} ${passed[*]:-}" 
echo "Failed: ${#failed[@]} ${failed[*]:-}"
echo "Skipped: ${#skipped[@]} ${skipped[*]:-}"

if [ ${#failed[@]} -ne 0 ]; then
  exit 1
fi

exit 0
