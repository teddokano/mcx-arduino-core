#!/bin/sh
# Upload with probe-rs: Tools > Upload method > probe-rs (boards.txt).
# The counterpart of upload.sh, which uses LinkServer.
#
# Usage: upload_probe-rs.sh <elf> <probe-rs chip> <port serial number> <port vid> <port pid> <bundled probe-rs's folder>
ELF="$1"
CHIP="$2"
PORT_SERIAL="$3"
PORT_VID="$4"
PORT_PID="$5"
BUNDLED="$6"

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

# Written, checked and reset to run. PROBE_ARGS unquoted on purpose: empty
# must vanish, not become "".
# Reset in the same session (--reset): a separate `probe-rs reset` right
# after could reach the chip while it was still starting, and get a FAULT.
# Once more if it fails: on an FRDM-MCXN947 running a sketch, probe-rs
# 0.32.0's reset-and-halt before writing fails every time, and the next
# one, right after, succeeds
"$PROBE_RS" download --chip "$CHIP" $PROBE_ARGS --verify --reset --disable-progressbars "$ELF"
STATUS=$?
if [ $STATUS -ne 0 ]; then
    echo "(once more)"
    "$PROBE_RS" download --chip "$CHIP" $PROBE_ARGS --verify --reset --disable-progressbars "$ELF"
    STATUS=$?
fi

if [ $STATUS -ne 0 ] && [ -z "$PROBE_ARGS" ]; then
    echo "============================================"
    echo "If more than one board is connected, select the port of the board"
    echo "to upload to (Arduino IDE: Tools > Port; arduino-cli: -p)."
    echo "============================================"
fi
exit $STATUS
