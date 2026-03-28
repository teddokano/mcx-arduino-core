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

# Linux
if [ "$(uname)" = "Linux" ]; then
    LINKSERVER=$(which LinkServer 2>/dev/null)
    if [ -z "$LINKSERVER" ]; then
        LINKSERVER="/usr/local/LinkServer/LinkServer"
    fi
fi

if [ -z "$LINKSERVER" ] || [ ! -f "$LINKSERVER" ]; then
    echo "============================================"
    echo "ERROR: LinkServer not found."
    echo "Please install LinkServer from:"
    echo "https://www.nxp.com/linkserver"
    echo "============================================"
    exit 1
fi

echo "Using: $LINKSERVER"

"$LINKSERVER" flash MCXA153:FRDM-MCXA153 load "$ELF"
