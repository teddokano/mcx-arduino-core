#!/usr/bin/env bash
# Runs a list of sketch builds, SWEEP_JOBS of them at a time (1 unless
# set), for compile_examples.sh and compile_third_party.sh. Sourced by
# them, not run on its own.
#
# run_builds <list> <failed> <work dir> <build function>
#   <list>: one build per line, "<label><TAB><sketch dir>"
#   <build function>: builds the sketch dir given as $1 into the build
#   cache ARDUINO_BUILD_CACHE_PATH, removes the sketch's build from it, and
#   returns non-zero if the build failed
#   For each build, in the list's order, it prints "::group::<label>", the
#   build's output, "OK" or "FAIL: <label>", and "::endgroup::", and it
#   adds the label of each failed build to <failed>. The output is the same
#   whatever SWEEP_JOBS is.
#
# With SWEEP_JOBS above 1, each worker has a build cache of its own (runs
# sharing one build the same core.a over each other and fail at random).
# The first build runs by itself, and builds the core into
# ARDUINO_BUILD_CACHE_PATH; that cache, now holding only the core, is then
# copied for each of the other workers (into <work dir>), so the core is
# built once, not once per worker.

run_builds() {
  local list="$1" failed="$2" work="$3" build="$4"
  local jobs="${SWEEP_JOBS:-1}" total label dir i
  total="$(wc -l < "$list" | tr -d ' ')"
  [[ "$jobs" =~ ^[1-9][0-9]*$ ]] || jobs=1

  if [[ "$jobs" -eq 1 || "$total" -le 1 ]]; then
    while IFS=$'\t' read -r label dir; do
      echo "::group::${label}"
      if "$build" "$dir" < /dev/null; then
        echo "OK"
      else
        echo "$label" >> "$failed"
        echo "FAIL: ${label}"
      fi
      echo "::endgroup::"
    done < "$list"
    return
  fi

  local out="${work}/builds"
  mkdir -p "${out}/claimed" "${out}/result" "${out}/log"
  local base="$ARDUINO_BUILD_CACHE_PATH"

  # Each worker goes down the list and takes the builds no one has taken
  # (mkdir is atomic; macOS has no flock); the first is taken already
  worker() {  # <build cache>
    local i=0 label dir
    while IFS=$'\t' read -r label dir; do
      i=$((i + 1))
      mkdir "${out}/claimed/$i" 2>/dev/null || continue
      ARDUINO_BUILD_CACHE_PATH="$1" "$build" "$dir" < /dev/null > "${out}/log/$i" 2>&1
      echo $? > "${out}/result/$i.tmp" && mv "${out}/result/$i.tmp" "${out}/result/$i"
    done < "$list"
  }

  # the first build, by itself, to build the core
  IFS=$'\t' read -r label dir < "$list"
  mkdir "${out}/claimed/1"
  "$build" "$dir" < /dev/null > "${out}/log/1" 2>&1
  echo $? > "${out}/result/1"

  local pids=() w
  for w in $(seq 1 "$jobs"); do
    if [[ $w -gt 1 ]]; then
      mkdir -p "${work}/cache-$w"
      [[ -d "$base" ]] && cp -Rp "${base}/." "${work}/cache-$w/"
      worker "${work}/cache-$w" &
    else
      worker "$base" &
    fi
    pids+=($!)
  done

  # each build's output in the list's order, as soon as it is done
  i=0
  while IFS=$'\t' read -r label dir; do
    i=$((i + 1))
    while [[ ! -e "${out}/result/$i" ]]; do
      if [[ -z "$(jobs -rp)" ]]; then
        echo "a build worker stopped before ${label} was built" >&2
        echo "$label" >> "$failed"
        break
      fi
      sleep 0.2
    done
    echo "::group::${label}"
    cat "${out}/log/$i" 2>/dev/null
    if [[ "$(cat "${out}/result/$i" 2>/dev/null)" == 0 ]]; then
      echo "OK"
    else
      grep -qxF "$label" "$failed" 2>/dev/null || echo "$label" >> "$failed"
      echo "FAIL: ${label}"
    fi
    echo "::endgroup::"
  done < "$list"
  wait "${pids[@]}"

  for w in $(seq 2 "$jobs"); do
    rm -rf "${work}/cache-$w"
  done
  rm -rf "$out"
}
