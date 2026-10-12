#!/bin/bash
# Moves the local development environment (packages/nxp with the
# <version>-dev symlink) out of packages/, so that Arduino IDE and
# arduino-cli see no nxp core. Use it before installing a release
# from Boards Manager or taking screenshots of a clean IDE.
# Undo with dev_env_restore.command. Double-click to run on macOS.

ARD="$HOME/Library/Arduino15"
SRC="$ARD/packages/nxp"
DST="$ARD/nxp-dev-env"

finish() {
	echo
	read -r -p "Enterキーで閉じます" _
	exit "$1"
}

if [ -e "$DST" ]; then
	echo "退避先がもうあります: $DST"
	echo "すでに退避済みです。戻すときは dev_env_restore.command を使ってください。"
	finish 1
fi

if [ ! -d "$SRC" ]; then
	echo "packages/nxp がありません: $SRC"
	finish 1
fi

dev_links=$(find "$SRC/hardware/mcx" -maxdepth 1 -type l -name '*-dev' 2>/dev/null)
if [ -z "$dev_links" ]; then
	echo "packages/nxp に開発版のsymlink（*-dev）がありません。"
	echo "Boards Managerで入れたものかもしれないので、何もしません。"
	ls -la "$SRC/hardware/mcx" 2>/dev/null
	finish 1
fi

link_info=$(for l in $dev_links; do echo "  $(basename "$l") -> $(readlink "$l")"; done)
mv "$SRC" "$DST" || finish 1
echo "開発環境を退避しました: $DST"
echo "$link_info"

if command -v arduino-cli >/dev/null 2>&1 || [ -x /opt/homebrew/bin/arduino-cli ]; then
	cli=$(command -v arduino-cli || echo /opt/homebrew/bin/arduino-cli)
	echo
	echo "arduino-cli core list:"
	"$cli" core list
fi

if pgrep -q -f "Arduino IDE.app/Contents/MacOS/Arduino IDE"; then
	echo
	echo "Arduino IDEが起動しています。再起動すると反映されます。"
fi

finish 0
