#!/bin/sh
# Picks the right gdb-bridge binary for this OS/arch and hands it FRDM-MCXA153's
# LinkServer device string. See gdb-bridge/src/main.go for what this does and why.
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

exec "$BIN" "MCXA153:FRDM-MCXA153" "$@"
