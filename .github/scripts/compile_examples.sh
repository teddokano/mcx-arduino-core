#!/usr/bin/env bash
# Compile-checks every example sketch for one board.
#
# Usage: compile_examples.sh <board> <mode>
#   board: frdm_mcxa153 | frdm_mcxn947
#   mode:  fast | full
#     fast - examples/release_check/** + hello_world + mcxPinState examples
#            (runs on every push/PR; a few minutes)
#     full - every .ino under examples/ and mcxPinState/examples/
#            (runs on push to main, workflow_dispatch, and release tags)
#
# Board-exclusive sketches (directory name ends in _N947, currently the
# only such suffix in use) are skipped on the other board -- that's
# expected and not a failure; see examples/release_check/README.md.
set -uo pipefail

BOARD="$1"
MODE="$2"
FQBN="nxp:mcx:${BOARD}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
STUB_LIB="${REPO_ROOT}/.github/ci-stubs/P3T1755"
FAILLOG="$(mktemp)"

SKETCH_LIST="$(mktemp)"

if [[ "$MODE" == "fast" ]]; then
  find "${REPO_ROOT}/examples/release_check" \
       "${REPO_ROOT}/examples/Arduino_compatible_API/hello_world" \
       "${REPO_ROOT}/hardware/nxp/mcx/libraries/mcxPinState/examples" \
       -name "*.ino" 2>/dev/null | sort > "$SKETCH_LIST"
elif [[ "$MODE" == "full" ]]; then
  find "${REPO_ROOT}/examples/Arduino_compatible_API" \
       "${REPO_ROOT}/examples/Arduino_incompatible_API" \
       "${REPO_ROOT}/examples/release_check" \
       "${REPO_ROOT}/hardware/nxp/mcx/libraries/mcxPinState/examples" \
       -name "*.ino" 2>/dev/null | sort > "$SKETCH_LIST"
else
  echo "Unknown mode: $MODE (expected 'fast' or 'full')" >&2
  exit 2
fi

echo "Board: $BOARD  Mode: $MODE  Sketches found: $(wc -l < "$SKETCH_LIST" | tr -d ' ')"

while IFS= read -r sketch; do
  dir="$(dirname "$sketch")"
  name="$(basename "$dir")"

  if [[ "$name" == *_N947 && "$BOARD" != "frdm_mcxn947" ]]; then
    continue
  fi

  echo "::group::${sketch#"$REPO_ROOT"/}"
  if arduino-cli compile --fqbn "$FQBN" --library "$STUB_LIB" --warnings all "$dir"; then
    echo "OK"
  else
    echo "FAIL: ${sketch#"$REPO_ROOT"/}" | tee -a "$FAILLOG"
  fi
  echo "::endgroup::"
done < "$SKETCH_LIST"

rm -f "$SKETCH_LIST"

if [[ -s "$FAILLOG" ]]; then
  echo ""
  echo "=== FAILED SKETCHES ($BOARD, $MODE) ==="
  cat "$FAILLOG"
  exit 1
fi

echo ""
echo "All sketches compiled OK ($BOARD, $MODE)"
