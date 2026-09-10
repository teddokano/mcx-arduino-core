#!/bin/sh
# Picks the right gdb-bridge binary for this OS/arch and passes everything
# through untouched. Board-agnostic: gdb-bridge reads which board to debug
# from the OpenOCD script boards.txt names for it (see gdb-bridge/src/main.go),
# so adding a board needs no change here.
DIR=$(cd "$(dirname "$0")" && pwd)

case "$(uname -s)" in
    Darwin)
        case "$(uname -m)" in
            arm64) BIN="$DIR/gdb-bridge-darwin-arm64" ;;
            *)     BIN="$DIR/gdb-bridge-darwin-amd64" ;;
        esac
        ;;
    Linux)
        case "$(uname -m)" in
            aarch64|arm64) BIN="$DIR/gdb-bridge-linux-arm64" ;;
            *)             BIN="$DIR/gdb-bridge-linux-amd64" ;;
        esac
        ;;
    *)
        echo "gdb-bridge: unsupported OS: $(uname -s)" >&2
        exit 1
        ;;
esac

exec "$BIN" "$@"
