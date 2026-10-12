#!/bin/bash
# Puts back the development environment moved aside by
# dev_env_evacuate.command. If a core was installed from Boards Manager
# in the meantime, it is moved to nxp-boardsmanager-<date-time> next to
# packages/ (kept, not deleted). Double-click to run on macOS.

ARD="$HOME/Library/Arduino15"
SRC="$ARD/packages/nxp"
DST="$ARD/nxp-dev-env"

finish() {
	echo
	read -r -p "Enterキーで閉じます" _
	exit "$1"
}

if [ ! -d "$DST" ]; then
	echo "退避した開発環境がありません: $DST"
	finish 1
fi

if [ -e "$SRC" ]; then
	if [ -n "$(find "$SRC/hardware/mcx" -maxdepth 1 -type l -name '*-dev' 2>/dev/null)" ]; then
		echo "packages/nxp に開発版のsymlinkがあります。退避先と両方あるので、何もしません。"
		echo "  $SRC"
		echo "  $DST"
		finish 1
	fi
	keep="$ARD/nxp-boardsmanager-$(date +%Y%m%d-%H%M%S)"
	mv "$SRC" "$keep" || finish 1
	echo "Boards Managerで入れたものを移しました: $keep"
	echo "  版: $(ls "$keep/hardware/mcx" 2>/dev/null | tr '\n' ' ')"
fi

mv "$DST" "$SRC" || finish 1
echo "開発環境を戻しました: $SRC"
for l in $(find "$SRC/hardware/mcx" -maxdepth 1 -type l -name '*-dev'); do
	echo "  $(basename "$l") -> $(readlink "$l")"
done

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
