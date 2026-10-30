#!/bin/sh
# Picks the right mcx-discovery binary for this OS/arch and passes
# everything through untouched (see mcx-discovery/src/main.go).
DIR=$(cd "$(dirname "$0")" && pwd)

case "$(uname -s)" in
    Darwin)
        case "$(uname -m)" in
            arm64) BIN="$DIR/mcx-discovery-darwin-arm64" ;;
            *)     BIN="$DIR/mcx-discovery-darwin-amd64" ;;
        esac
        ;;
    Linux)
        case "$(uname -m)" in
            aarch64|arm64) BIN="$DIR/mcx-discovery-linux-arm64" ;;
            *)             BIN="$DIR/mcx-discovery-linux-amd64" ;;
        esac
        ;;
    *)
        echo "mcx-discovery: unsupported OS: $(uname -s)" >&2
        exit 1
        ;;
esac

exec "$BIN" "$@"
