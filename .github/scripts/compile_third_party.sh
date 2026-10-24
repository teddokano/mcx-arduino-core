#!/usr/bin/env bash
# Compile-checks the examples of the third-party libraries listed in
# .github/third_party/libraries.txt for one board, against the core
# installed for arduino-cli.
#
# Usage: compile_third_party.sh <board> [--no-install]
#        compile_third_party.sh --install-only
#   board: frdm_mcxa153 | frdm_mcxn947 | frdm_mcxa156 | frdm_mcxn236 |
#          frdm_mcxc444
#   --install-only installs the libraries into TP_USER_DIR and stops;
#   --no-install builds against the ones already there. Runs for several
#   boards at once share TP_USER_DIR this way (compile_all_boards.sh does).
#
# Each run has a build cache of its own, as in compile_examples.sh: the core
# is built once and kept, and each sketch's build directory is removed after
# it (80 sketches' would take ~20GB). SWEEP_JOBS=N builds N at a time, as
# in compile_examples.sh.
#
# An example listed in .github/third_party/expected_failures.txt for the
# board (or for "*") is expected not to build; the check fails on any other
# failure, and on a listed example that now builds, so that the list stays
# true. On frdm_mcxn947 the Ethernet library's examples are also built
# against the bundled MCXEthernet, with only their #include changed.
# This repository's sketches listed in .github/third_party/repo_sketches.txt
# (which need these libraries) are built too.
#
# The libraries go into a directory of their own (ARDUINO_DIRECTORIES_USER),
# not the sketchbook. Set TP_USER_DIR to reuse one between runs.
set -uo pipefail
BOARD="$1"
INSTALL=1
[[ "${2:-}" == "--no-install" ]] && INSTALL=0
if [[ "$BOARD" == "--install-only" && -z "${TP_USER_DIR:-}" ]]; then
  echo "--install-only needs TP_USER_DIR" >&2
  exit 2
fi
FQBN="nxp:mcx:${BOARD}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "${REPO_ROOT}/.github/scripts/run_builds.sh"
LIB_LIST="${REPO_ROOT}/.github/third_party/libraries.txt"
EXPECTED="${REPO_ROOT}/.github/third_party/expected_failures.txt"
REPO_SKETCHES="${REPO_ROOT}/.github/third_party/repo_sketches.txt"
# a name ending in .noindex keeps Spotlight from indexing the build files
WORK="$(mktemp -d)" && mv "$WORK" "${WORK}.noindex" && WORK="${WORK}.noindex"
export ARDUINO_DIRECTORIES_USER="${TP_USER_DIR:-${WORK}/user}"
export ARDUINO_BUILD_CACHE_PATH="${ARDUINO_BUILD_CACHE_PATH:-${WORK}/cache}"
LIBS="${ARDUINO_DIRECTORIES_USER}/libraries"

if [[ $INSTALL -eq 1 ]]; then
  arduino-cli lib update-index >/dev/null
  while IFS= read -r lib; do
    [[ -z "$lib" || "$lib" == \#* ]] && continue
    if ! arduino-cli lib install --no-deps "$lib" >/dev/null; then
      echo "could not install $lib" >&2
      exit 2
    fi
  done < "$LIB_LIST"
fi
if [[ "$BOARD" == "--install-only" ]]; then
  rm -rf "$WORK"
  exit 0
fi

# the examples expected to fail on this board, as paths under $LIBS
expected_for_board() {
  grep -v '^#' "$EXPECTED" | awk -v b="$BOARD" 'NF >= 2 && ($1 == b || $1 == "*") { print $2 }' | sort -u
}
EXPECTED_HERE="${WORK}/expected"
expected_for_board > "$EXPECTED_HERE"

SKETCHES="${WORK}/sketches"
(cd "$LIBS" && find . -path '*/examples/*' -name '*.ino' | sed 's|^\./||' | sort) > "$SKETCHES"

# On FRDM-MCXN947, the Ethernet examples once more, on MCXEthernet
if [[ "$BOARD" == "frdm_mcxn947" ]]; then
  mkdir -p "${WORK}/mcxethernet"
  cp -R "${LIBS}/Ethernet/examples/." "${WORK}/mcxethernet/"
  for f in "${WORK}"/mcxethernet/*/*.ino; do
    sed -i.bak -e 's/#include <Ethernet\.h>/#include <MCXEthernet.h>/' -e '/#include <EthernetUdp\.h>/d' "$f"
    rm -f "$f.bak"
  done
fi

FAILED="${WORK}/failed"
: > "$FAILED"

compile() {  # <sketch dir>
  arduino-cli compile --fqbn "$FQBN" "$1" < /dev/null
  local status=$?
  rm -rf "${ARDUINO_BUILD_CACHE_PATH}/sketches"
  return $status
}

# the builds, as "<label><TAB><sketch dir>"
BUILDS="${WORK}/builds.list"
while IFS= read -r rel; do
  printf '%s\t%s\n' "$rel" "${LIBS}/$(dirname "$rel")"
done < "$SKETCHES" > "$BUILDS"
if [[ -d "${WORK}/mcxethernet" ]]; then
  for d in "${WORK}"/mcxethernet/*/; do
    printf '%s\t%s\n' "MCXEthernet:$(basename "$d")" "$d"
  done >> "$BUILDS"
fi
grep -v -e '^#' -e '^$' "$REPO_SKETCHES" | while IFS= read -r rel; do
  printf '%s\t%s\n' "repo:${rel}" "${REPO_ROOT}/${rel}"
done >> "$BUILDS"
total="$(wc -l < "$BUILDS" | tr -d ' ')"

run_builds "$BUILDS" "$FAILED" "$WORK" compile

sort -u "$FAILED" -o "$FAILED"
unexpected="$(comm -23 "$FAILED" "$EXPECTED_HERE")"
now_pass="$(comm -13 "$FAILED" "$EXPECTED_HERE")"

echo "Board: $BOARD  Sketches: $total  Failed: $(wc -l < "$FAILED" | tr -d ' ')  Expected to fail: $(wc -l < "$EXPECTED_HERE" | tr -d ' ')"
status=0
if [[ -n "$unexpected" ]]; then
  echo "Failed, not in expected_failures.txt:"
  echo "$unexpected" | sed 's/^/  /'
  status=1
fi
if [[ -n "$now_pass" ]]; then
  echo "In expected_failures.txt for this board, but built (take it off the list):"
  echo "$now_pass" | sed 's/^/  /'
  status=1
fi
[[ $status -eq 0 ]] && echo "All as expected"
rm -rf "$WORK"
exit $status
