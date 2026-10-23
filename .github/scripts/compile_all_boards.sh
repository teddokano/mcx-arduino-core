#!/usr/bin/env bash
# Runs compile_examples.sh or compile_third_party.sh for every board at once,
# one process per board, for a local sweep (CI runs the boards on separate
# machines instead).
#
# Usage: compile_all_boards.sh <fast|full|third_party> [--queue N] [board ...]
#   With no boards given, all five.
#   --queue N (fast and full): instead of one process per board, puts every
#   (board, sketch) build in one queue, taken in turn from each board, and
#   runs N builds at a time, so that no CPU waits at the end for the board
#   with the most sketches.
#
# Each of those scripts makes a build cache of its own, so the boards don't
# share one (runs sharing one build the same core.a over each other and fail
# at random). With --queue, each build takes a free cache of its board from a
# pool, so a board's core is built once per cache, not once per sketch. For
# third_party, the libraries are installed once, before the boards start, and
# every board builds against that one copy.
#
# Each board's output goes to <board>.log in the log directory printed at the
# start (SWEEP_LOG_DIR to choose it).
set -uo pipefail

MODE="${1:-}"
shift || true
QUEUE=""
if [[ "${1:-}" == "--queue" ]]; then
  QUEUE="${2:-}"
  shift 2 || true
fi
BOARDS=("$@")
[[ ${#BOARDS[@]} -eq 0 ]] && BOARDS=(frdm_mcxa153 frdm_mcxn947 frdm_mcxa156 frdm_mcxn236 frdm_mcxc444)
SCRIPTS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPTS}/../.." && pwd)"

case "$MODE" in
  fast|full|third_party) ;;
  *) echo "Usage: $0 <fast|full|third_party> [--queue N] [board ...]" >&2; exit 2 ;;
esac
if [[ -n "$QUEUE" ]] && { [[ ! "$QUEUE" =~ ^[1-9][0-9]*$ ]] || [[ "$MODE" == "third_party" ]]; }; then
  echo "--queue takes a number of builds, with fast or full" >&2
  exit 2
fi

# a name ending in .noindex keeps Spotlight from indexing the build files
WORK="$(mktemp -d)" && mv "$WORK" "${WORK}.noindex" && WORK="${WORK}.noindex"
LOGS="${SWEEP_LOG_DIR:-${WORK}/logs}"
mkdir -p "$LOGS"
echo "Mode: $MODE${QUEUE:+  Queue: $QUEUE at a time}  Boards: ${BOARDS[*]}"
echo "Logs: $LOGS"

if [[ "$MODE" == "third_party" ]]; then
  export TP_USER_DIR="${TP_USER_DIR:-${WORK}/user}"
  if ! "${SCRIPTS}/compile_third_party.sh" --install-only > "${LOGS}/install.log" 2>&1; then
    echo "Installing the libraries failed; see ${LOGS}/install.log" >&2
    exit 2
  fi
fi

start=$(date +%s)

if [[ -n "$QUEUE" ]]; then
  # the builds, as "<board><TAB><sketch dir>", taken in turn from each board
  JOBS="${WORK}/jobs"
  for b in "${BOARDS[@]}"; do
    "${SCRIPTS}/compile_examples.sh" "$b" "$MODE" --list | awk -v b="$b" '{ print NR "\t" b "\t" $0 }'
  done | sort -s -t $'\t' -k1,1n | cut -f2- > "$JOBS"
  mkdir -p "${WORK}/claimed" "${WORK}/pool" "${WORK}/result" "${LOGS}/jobs"

  # Each worker goes down the list and takes the builds no one has taken
  # (mkdir is atomic; macOS has no flock), and for each one the first free
  # cache of its board (there are never more than QUEUE in use).
  worker() {
    local i=0 board dir c
    while IFS=$'\t' read -r board dir; do
      i=$((i + 1))
      mkdir "${WORK}/claimed/$i" 2>/dev/null || continue
      for c in $(seq 1 "$QUEUE"); do
        mkdir "${WORK}/pool/${board}.${c}.lock" 2>/dev/null && break
      done
      ARDUINO_BUILD_CACHE_PATH="${WORK}/pool/${board}.${c}" \
        "${SCRIPTS}/compile_examples.sh" "$board" one "$dir" > "${LOGS}/jobs/$i.log" 2>&1
      echo "$? $board ${dir#"$REPO_ROOT"/}" > "${WORK}/result/$i"
      rmdir "${WORK}/pool/${board}.${c}.lock"
    done < "$JOBS"
  }
  for w in $(seq 1 "$QUEUE"); do
    worker &
  done
  wait

  # each board's log, in the order compile_examples.sh would write it
  for b in "${BOARDS[@]}"; do
    : > "${LOGS}/${b}.log"
  done
  total="$(wc -l < "$JOBS" | tr -d ' ')"
  for i in $(seq 1 "$total"); do
    read -r rc board rel < "${WORK}/result/$i"
    {
      echo "::group::${rel}"
      cat "${LOGS}/jobs/$i.log"
      if [[ "$rc" == 0 ]]; then echo "OK"; else echo "FAIL: ${rel}"; fi
      echo "::endgroup::"
    } >> "${LOGS}/${board}.log"
  done
  rm -rf "${LOGS}/jobs" "${WORK}/pool" "${WORK}/claimed"

  status=0
  echo ""
  for b in "${BOARDS[@]}"; do
    printf '%-14s %s OK, %s failed\n' "$b" "$(grep -c '^OK$' "${LOGS}/${b}.log")" "$(grep -c '^FAIL: ' "${LOGS}/${b}.log")"
  done
  failed="$(awk '$1 != 0 { print "FAIL: " $2 "  " $3 }' "${WORK}"/result/*)"
  if [[ -n "$failed" ]]; then
    echo ""
    echo "$failed" | sort
    status=1
  fi
  echo "Elapsed: $(( $(date +%s) - start ))s"
  [[ -z "${SWEEP_LOG_DIR:-}" ]] && echo "(the logs stay in $LOGS)"
  exit $status
fi

for b in "${BOARDS[@]}"; do
  (
    unset ARDUINO_BUILD_CACHE_PATH
    if [[ "$MODE" == "third_party" ]]; then
      "${SCRIPTS}/compile_third_party.sh" "$b" --no-install
    else
      "${SCRIPTS}/compile_examples.sh" "$b" "$MODE"
    fi > "${LOGS}/${b}.log" 2>&1
    echo $? > "${WORK}/${b}.rc"
    echo "  done: $b (exit $(cat "${WORK}/${b}.rc"), $(( $(date +%s) - start ))s)"
  ) &
done
wait

status=0
echo ""
for b in "${BOARDS[@]}"; do
  rc="$(cat "${WORK}/${b}.rc")"
  printf '%-14s exit %s  %s\n' "$b" "$rc" "$(tail -1 "${LOGS}/${b}.log")"
  [[ "$rc" != 0 ]] && status=1
done
echo "Elapsed: $(( $(date +%s) - start ))s"

rm -rf "${WORK}/user"
[[ -z "${SWEEP_LOG_DIR:-}" ]] && echo "(the logs stay in $LOGS)"
exit $status
