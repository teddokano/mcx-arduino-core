#!/bin/sh
# Upload with probe-rs: Tools > Upload method > probe-rs (boards.txt).
# The counterpart of upload.sh, which uses LinkServer.
#
# Usage: upload_probe-rs.sh <elf> <probe-rs chip> <port serial number> <port vid> <port pid> <bundled probe-rs's folder> [verify|noverify]
ELF="$1"
CHIP="$2"
PORT_SERIAL="$3"
PORT_VID="$4"
PORT_PID="$5"
BUNDLED="$6"

# Read back and check what was written: as Arduino IDE's "Verify code
# after upload" says (platform.txt; off by default), and when run by hand
# with no seventh argument. It takes about 1.8 of the 4s on the
# FRDM-MCXA153 and FRDM-MCXA156 (at 4MHz, below)
case "${7-verify}" in
    noverify) VERIFY_ARG="" ;;
    *) VERIFY_ARG="--verify" ;;
esac

# probe-rs: MCX_PROBE_RS if set, then the one that came with the package
# ({runtime.tools.probe-rs.path}, which arrives unexpanded, in braces, when
# it isn't installed), then PATH, then where its own installer and Homebrew
# put it (gdb-bridge's findProbeRs() looks in the same places)
case "$BUNDLED" in
    ""|"{"*) BUNDLED="" ;;
    *) BUNDLED="$BUNDLED/probe-rs" ;;
esac
PROBE_RS=""
for CANDIDATE in "$MCX_PROBE_RS" "$BUNDLED" "$(command -v probe-rs 2>/dev/null)" \
                 "$HOME/.cargo/bin/probe-rs" /opt/homebrew/bin/probe-rs /usr/local/bin/probe-rs; do
    if [ -n "$CANDIDATE" ] && [ -x "$CANDIDATE" ]; then
        PROBE_RS=$CANDIDATE
        break
    fi
done
if [ -z "$PROBE_RS" ]; then
    echo "============================================"
    echo "ERROR: probe-rs not found. It comes with the board package:"
    echo "reinstall the package in Boards Manager, install probe-rs"
    echo "from https://probe.rs, or choose Tools > Upload method > LinkServer."
    echo "============================================"
    exit 1
fi
echo "Using: $PROBE_RS ($("$PROBE_RS" --version 2>/dev/null | head -1))"

# The selected port's USB serial number is the MCU-LINK's own, and with
# the port's VID and PID it makes the VID:PID:SERIAL that probe-rs's
# --probe takes, to pick the board when more than one is connected. Left
# out when no port was selected (the placeholder arrives unexpanded), when
# the port isn't an NXP probe, or when probe-rs doesn't list that serial
# number, as upload.sh does.
PROBE_ARGS=""
case "$PORT_SERIAL" in
    ""|"{"*) ;;
    *)
        case "$PORT_VID" in
            0x1FC9|0x1fc9)
                if "$PROBE_RS" list 2>/dev/null | grep -F -q -- "$PORT_SERIAL"; then
                    PID=$(echo "$PORT_PID" | sed 's/^0[xX]//' | tr 'A-F' 'a-f')
                    PROBE_ARGS="--probe 1fc9:$PID:$PORT_SERIAL"
                    echo "Probe: 1fc9:$PID:$PORT_SERIAL"
                else
                    echo "The port's serial number $PORT_SERIAL is not among probe-rs's probes; uploading without --probe"
                fi
                ;;
        esac
        ;;
esac

# The SWD clock: 4MHz unless PROBE_RS_SPEED (kHz, which probe-rs reads
# itself) says otherwise. probe-rs's own 1MHz took 8s to write and check
# a Blink on the FRDM-MCXA153 and FRDM-MCXA156, and 3.5 to 4s at 4MHz,
# about LinkServer's 3s (10MHz gained 0.2 to 0.4s more). On an
# FRDM-MCXN947 running a sketch, probe-rs 0.32.0's reset-and-halt failed
# every other time at 1MHz, and not once in 16 at 4MHz
: "${PROBE_RS_SPEED:=4000}"
export PROBE_RS_SPEED

# Written, checked (VERIFY_ARG) and reset to run. PROBE_ARGS and VERIFY_ARG
# unquoted on purpose: empty must vanish, not become "".
# Reset in the same session (--reset): a separate `probe-rs reset` right
# after could reach the chip while it was still starting, and get a FAULT.
# Tried up to three times, a second apart: on an FRDM-MCXN947 running a
# sketch, probe-rs 0.32.0's reset-and-halt before writing fails every
# time, and the next one usually succeeds; after a debug session in
# Arduino IDE, the second failed too. The FRDM-MCXN236 has needed a
# second try now and then as well.
#
# What a failed try says is kept back, and shown only if every try fails:
# on the FRDM-MCXN947 a successful upload would otherwise show a page of
# errors from its first try. A try that succeeds says it came after
# failed ones, in one line.
#
# probe-rs writes all it says to stderr, which Arduino IDE shows as an
# error, and with color and bold codes even when it isn't a terminal
# ("[m[1mFinished[0m in 8.26s" in the IDE). It comes here on stdout,
# with the codes taken out; a failure is said on stderr below.
#
# stdin is closed (</dev/null): with more than one probe connected and no
# --probe, probe-rs asks which one to use and waits for the answer, even
# with --non-interactive (0.32.0); with nothing to read it stops at once
ESC=$(printf '\033')
FAILED_TRIES=""
ATTEMPT=1
while :; do
    OUTPUT=$("$PROBE_RS" download --chip "$CHIP" $PROBE_ARGS $VERIFY_ARG --reset --disable-progressbars "$ELF" </dev/null 2>&1)
    STATUS=$?
    OUTPUT=$(printf '%s\n' "$OUTPUT" | sed "s/${ESC}\[[0-9;]*m//g")
    if [ $STATUS -eq 0 ]; then
        case $ATTEMPT in
            1) ;;
            2) echo "Written on try 2 of 3 (the first try failed; what it said is left out)" ;;
            *) echo "Written on try $ATTEMPT of 3 (the tries before it failed; what they said is left out)" ;;
        esac
        printf '%s\n' "$OUTPUT"
        break
    fi
    FAILED_TRIES="$FAILED_TRIES--- try $ATTEMPT of 3 ---
$OUTPUT
"
    if [ $ATTEMPT -ge 3 ]; then
        printf '%s' "$FAILED_TRIES"
        break
    fi
    ATTEMPT=$((ATTEMPT + 1))
    sleep 1
done

if [ $STATUS -ne 0 ]; then
    echo "ERROR: probe-rs could not write the board in 3 tries (above: what it said each time)" >&2
fi
if [ $STATUS -ne 0 ] && [ -z "$PROBE_ARGS" ]; then
    echo "============================================"
    echo "If more than one board is connected, select the port of the board"
    echo "to upload to (Arduino IDE: Tools > Port; arduino-cli: -p)."
    echo "============================================"
fi
exit $STATUS
