#!/bin/sh
# FRDM-MCXA153 upload script using OpenOCD + GDB

OPENOCD="$1"
GDB="$2"
SCRIPTS="$(dirname $OPENOCD)/../share/openocd/scripts"
ELF="$3"

# OpenOCDをバックグラウンドで起動
"$OPENOCD" -s "$SCRIPTS" \
  -f interface/cmsis-dap.cfg \
  -c "transport select swd" \
  -c "adapter speed 1000" \
  -c "set WORKAREASIZE 0x2000" \
  -c "source [find target/ke1xz.cfg]" \
  -c "gdb_flash_program disable" \
  -c "gdb_memory_map disable" \
  -c "init" \
  -c "reset" \
  -c "sleep 200" \
  -c "halt" &
OPENOCD_PID=$!
sleep 2

# GDBで書き込み
"$GDB" "$ELF" \
  -batch \
  -ex "target remote localhost:3333" \
  -ex "load" \
  -ex "monitor resume" \
  -ex "disconnect" \
  -ex "quit"

kill $OPENOCD_PID 2>/dev/null
