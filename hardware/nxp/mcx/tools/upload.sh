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

# LinkServer installs, one per line, the preferred one first.
linkserver_installs() {
    # macOS: newest version first
    if [ "$(uname)" = "Darwin" ]; then
        ls /Applications/ | grep "^LinkServer" | sort -V -r | sed 's|^\(.*\)$|/Applications/\1/LinkServer|'
    fi

    # Linux -- discovery order adapted from ArduinoCore-zephyr's
    # tools/upload_pyocd_or_linkserver.sh (Apache License 2.0): fixed install
    # path first, then versioned install dirs (LinkServer's installer names
    # these LinkServer_<version>), then fall back to PATH.
    if [ "$(uname)" = "Linux" ]; then
        echo /usr/local/LinkServer/LinkServer
        ls -d /usr/local/LinkServer_* 2>/dev/null | sort -V -r | sed 's|$|/LinkServer|'
        command -v LinkServer 2>/dev/null
    fi
}

# LinkServer versions whose flash driver takes the FRDM-MCXA153's 128KB of
# flash for 32KB, so a larger sketch fails to load ("Attempt to load into
# missing flash area"). They are passed over for that board (the MCXA153:
# case below) while another install is present. Keep in step with
# upload.bat and gdb-bridge's flashSizeBug.
flash_size_bug() {
    case "$1" in
        26.9.*) return 0 ;;
    esac
    return 1
}

linkserver_version() {
    "$1" --version 2>/dev/null | sed -n 's/^LinkServer v\([0-9.]*\).*/\1/p' | head -1
}

LINKSERVER=""
BUGGY=""
BUGGY_VERSION=""
SKIPPED=""
OLDIFS=$IFS
IFS='
'
for CANDIDATE in $(linkserver_installs); do
    [ -x "$CANDIDATE" ] || continue
    case "$LINKSERVER_TARGET" in
        MCXA153:*)
            VERSION=$(linkserver_version "$CANDIDATE")
            if flash_size_bug "$VERSION"; then
                if [ -z "$BUGGY" ]; then
                    BUGGY=$CANDIDATE
                    BUGGY_VERSION=$VERSION
                fi
                SKIPPED="$SKIPPED $VERSION"
                continue
            fi
            ;;
    esac
    LINKSERVER=$CANDIDATE
    break
done
IFS=$OLDIFS

# Only versions with the bug: use one anyway, since a sketch that fits in
# 32KB still loads, and explain if it fails.
if [ -z "$LINKSERVER" ] && [ -n "$BUGGY" ]; then
    LINKSERVER=$BUGGY
    SKIPPED=""
else
    BUGGY_VERSION=""
fi

if [ -z "$LINKSERVER" ]; then
    echo "============================================"
    echo "ERROR: LinkServer not found."
    echo "Please install LinkServer from:"
    echo "https://www.nxp.com/linkserver"
    echo "============================================"
    exit 1
fi

echo "Using: $LINKSERVER"
if [ -n "$SKIPPED" ]; then
    echo "(passed over LinkServer$SKIPPED, which reads the FRDM-MCXA153's flash as 32KB)"
fi

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

if [ $STATUS -ne 0 ] && [ -n "$BUGGY_VERSION" ]; then
    echo "============================================"
    echo "If the error above is \"Attempt to load into missing flash area\":"
    echo "LinkServer $BUGGY_VERSION reads the FRDM-MCXA153's flash as 32KB, so a"
    echo "sketch larger than that fails to upload. Install an earlier LinkServer"
    echo "(26.6 or before, https://www.nxp.com/linkserver) alongside it; uploads"
    echo "then use that one automatically."
    echo "============================================"
fi

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
