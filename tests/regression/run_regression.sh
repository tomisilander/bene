#!/usr/bin/env bash
set -euo pipefail

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

  set +e
  out=$("$BIN_DIR/data2net.sh" "$vd_file" "$idt_file" 1 "$rdir" 2>&1)
  rc=$?
  set -e

  echo "$out"

  if [ $rc -ne 0 ]; then
    echo "FAIL: data2net.sh exited $rc for dataset $name"
    echo "$out" > "$rdir/run.log"
    failed+=("$name")
    continue
  fi

  if [ ! -f "$rdir/net" ]; then
    echo "FAIL: expected $rdir/net does not exist for dataset $name"
    echo "$out" > "$rdir/run.log"
    failed+=("$name")
    continue
  fi

  echo "PASS: $name"
  passed+=("$name")
  # remove the temporary result directory since the run succeeded
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
