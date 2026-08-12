#!/bin/sh
ELF="$1"

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

"$LINKSERVER" flash MCXA153:FRDM-MCXA153 load "$ELF"
