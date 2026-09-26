#!/bin/sh
ELF="$1"
LINKSERVER_TARGET="$2"
PORT_SERIAL="$3"
PORT_VID="$4"

# The selected port's USB serial number, which for an NXP probe (MCU-Link,
# VID 0x1FC9) is the probe serial LinkServer wants. Without it, LinkServer
# refuses to flash while more than one board is plugged in. Left out for
# any other port, and when no port was selected (the placeholder then
# arrives unexpanded, still in braces), so one board keeps working as
# before.
PROBE_ARGS=""
case "$PORT_SERIAL" in
    ""|"{"*) ;;
    *)
        case "$PORT_VID" in
            0x1FC9|0x1fc9) PROBE_ARGS="--probe $PORT_SERIAL" ;;
        esac
        ;;
esac

# LinkServerを探す（macOS）
LINKSERVER=""
if [ "$(uname)" = "Darwin" ]; then
    LINKSERVER_DIR=$(ls /Applications/ | grep "^LinkServer" | sort -V | tail -1)
    if [ -n "$LINKSERVER_DIR" ]; then
        LINKSERVER="/Applications/$LINKSERVER_DIR/LinkServer"
    fi
fi

# Linux -- discovery order adapted from ArduinoCore-zephyr's
# tools/upload_pyocd_or_linkserver.sh (Apache License 2.0): fixed install
# path first, then versioned install dirs (LinkServer's installer names
# these LinkServer_<version>), then fall back to PATH.
if [ "$(uname)" = "Linux" ]; then
    if [ -x /usr/local/LinkServer/LinkServer ]; then
        LINKSERVER=/usr/local/LinkServer/LinkServer
    else
        LINKSERVER_DIR=$(ls -d /usr/local/LinkServer_* 2>/dev/null | sort -V | tail -1)
        if [ -n "$LINKSERVER_DIR" ]; then
            LINKSERVER="$LINKSERVER_DIR/LinkServer"
        fi
    fi

    if [ -z "$LINKSERVER" ] || [ ! -x "$LINKSERVER" ]; then
        LINKSERVER=$(command -v LinkServer 2>/dev/null)
    fi
fi

if [ -z "$LINKSERVER" ] || [ ! -x "$LINKSERVER" ]; then
    echo "============================================"
    echo "ERROR: LinkServer not found."
    echo "Please install LinkServer from:"
    echo "https://www.nxp.com/linkserver"
    echo "============================================"
    exit 1
fi

echo "Using: $LINKSERVER"

# Only pass --probe when LinkServer lists that serial number, since it
# refuses to flash at all for one it doesn't know ("No probes matched").
# A port that reports its serial number in some other form then uploads as
# before --probe was passed: fine with one board, refused with several.
# A probe busy with a debug session is still listed, so this doesn't send
# the upload to another board.
UNLISTED=""
if [ -n "$PROBE_ARGS" ]; then
    if "$LINKSERVER" probes 2>&1 | grep -F -i -w -q -- "$PORT_SERIAL"; then
        echo "Probe: $PORT_SERIAL"
    else
        echo "The port's serial number $PORT_SERIAL is not among LinkServer's probes; uploading without --probe"
        PROBE_ARGS=""
        UNLISTED=1
    fi
fi

# PROBE_ARGS unquoted on purpose: empty must vanish, not become "".
"$LINKSERVER" flash $PROBE_ARGS "$LINKSERVER_TARGET" load "$ELF"
STATUS=$?

# LinkServer's own message asks for --probe, which an IDE user can't pass.
if [ $STATUS -ne 0 ] && [ -z "$PROBE_ARGS" ]; then
    echo "============================================"
    if [ -n "$UNLISTED" ]; then
        echo "The selected port could not be matched to a debug probe, so with"
        echo "more than one board connected, leave only the one to upload to."
    else
        echo "If more than one board is connected, select the port of the board"
        echo "to upload to (Arduino IDE: Tools > Port; arduino-cli: -p)."
    fi
    echo "============================================"
fi
exit $STATUS
