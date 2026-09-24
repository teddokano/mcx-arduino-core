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
if [ -n "$PROBE_ARGS" ]; then
    echo "Probe: $PORT_SERIAL"
fi

# PROBE_ARGS unquoted on purpose: empty must vanish, not become "".
"$LINKSERVER" flash $PROBE_ARGS "$LINKSERVER_TARGET" load "$ELF"
STATUS=$?

# LinkServer's own message asks for --probe, which an IDE user can't pass.
if [ $STATUS -ne 0 ] && [ -z "$PROBE_ARGS" ]; then
    echo "============================================"
    echo "If more than one board is connected, select the port of the board"
    echo "to upload to (Arduino IDE: Tools > Port; arduino-cli: -p)."
    echo "============================================"
fi
exit $STATUS
