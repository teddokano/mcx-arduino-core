#!/usr/bin/env bash
# Compile-checks every example sketch for one board.
#
# Usage: compile_examples.sh <board> <mode> [--list]
#        compile_examples.sh <board> one <sketch dir>
#   board: frdm_mcxa153 | frdm_mcxn947 | frdm_mcxa156 | frdm_mcxn236 |
#          frdm_mcxc444
#   mode:  fast | full
#     fast - examples/release_check/** + hello_world + the bundled
#            libraries' examples (runs on every push/PR; a few minutes)
#     full - every .ino under examples/ and under libraries/*/examples/
#            (runs on push to main, workflow_dispatch, and release tags)
#   --list prints the sketch directories the mode builds for the board, one
#   per line, and builds nothing; "one" builds just the given sketch, with
#   the same flags (compile_all_boards.sh --queue uses both)
#
# The bundled libraries are globbed rather than named, so adding one to
# hardware/nxp/mcx/libraries/ puts its examples in the sweep by itself.
#
# Board-exclusive sketches (directory name ending in the board's chip, as
# _N947 or _A156) are skipped on every
# other board -- that's expected and not a failure; see
# examples/release_check/README.md.
#
# Each run has a build cache of its own (ARDUINO_BUILD_CACHE_PATH, in a
# temporary directory unless set), so that several boards can run at once
# (compile_all_boards.sh): runs sharing one build the same core.a over each
# other and fail at random. The core is built once and kept in that cache;
# each sketch's build directory in it is removed after the sketch. (Not
# --build-path: with it, arduino-cli builds the core again for every sketch.)
set -uo pipefail

BOARD="$1"
MODE="$2"
OPTION="${3:-}"
FQBN="nxp:mcx:${BOARD}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
STUB_LIB="${REPO_ROOT}/.github/ci-stubs/P3T1755"
# a name ending in .noindex keeps Spotlight from indexing the build files
WORK="$(mktemp -d)" && mv "$WORK" "${WORK}.noindex" && WORK="${WORK}.noindex"
export ARDUINO_BUILD_CACHE_PATH="${ARDUINO_BUILD_CACHE_PATH:-${WORK}/cache}"
FAILLOG="${WORK}/failed"
: > "$FAILLOG"

SKETCH_LIST="${WORK}/sketches"

compile_one() {  # <sketch dir>
  arduino-cli compile --fqbn "$FQBN" --library "$STUB_LIB" --warnings all "$1" < /dev/null
  local status=$?
  rm -rf "${ARDUINO_BUILD_CACHE_PATH}/sketches"
  return $status
}

if [[ "$MODE" == "one" ]]; then
  compile_one "$OPTION"
  status=$?
  rm -rf "$WORK"
  exit $status
fi

if [[ "$MODE" == "fast" ]]; then
  find "${REPO_ROOT}/examples/release_check" \
       "${REPO_ROOT}/examples/Arduino_compatible_API/hello_world" \
       "${REPO_ROOT}"/hardware/nxp/mcx/libraries/*/examples \
       -name "*.ino" 2>/dev/null | sort > "$SKETCH_LIST"
elif [[ "$MODE" == "full" ]]; then
  find "${REPO_ROOT}/examples/Arduino_compatible_API" \
       "${REPO_ROOT}/examples/Arduino_incompatible_API" \
       "${REPO_ROOT}/examples/release_check" \
       "${REPO_ROOT}"/hardware/nxp/mcx/libraries/*/examples \
       -name "*.ino" 2>/dev/null | sort > "$SKETCH_LIST"
else
  echo "Unknown mode: $MODE (expected 'fast', 'full' or 'one')" >&2
  exit 2
fi

if [[ "$OPTION" == "--list" ]]; then
  while IFS= read -r sketch; do
    dir="$(dirname "$sketch")"
    name="$(basename "$dir")"
    suffix="${name##*_}"
    if [[ "$suffix" =~ ^[ACN][0-9]{3}$ ]] && \
       [[ "$BOARD" != *"$(echo "$suffix" | tr '[:upper:]' '[:lower:]')" ]]; then
      continue
    fi
    echo "$dir"
  done < "$SKETCH_LIST"
  rm -rf "$WORK"
  exit 0
fi

echo "Board: $BOARD  Mode: $MODE  Sketches found: $(wc -l < "$SKETCH_LIST" | tr -d ' ')"

while IFS= read -r sketch; do
  dir="$(dirname "$sketch")"
  name="$(basename "$dir")"

  suffix="${name##*_}"
  if [[ "$suffix" =~ ^[ACN][0-9]{3}$ ]] && \
     [[ "$BOARD" != *"$(echo "$suffix" | tr '[:upper:]' '[:lower:]')" ]]; then
    continue
  fi

  echo "::group::${sketch#"$REPO_ROOT"/}"
  if compile_one "$dir"; then
    echo "OK"
  else
    echo "FAIL: ${sketch#"$REPO_ROOT"/}" | tee -a "$FAILLOG"
  fi
  echo "::endgroup::"
done < "$SKETCH_LIST"

if [[ -s "$FAILLOG" ]]; then
  echo ""
  echo "=== FAILED SKETCHES ($BOARD, $MODE) ==="
  cat "$FAILLOG"
  rm -rf "$WORK"
  exit 1
fi
rm -rf "$WORK"

echo ""
echo "All sketches compiled OK ($BOARD, $MODE)"
