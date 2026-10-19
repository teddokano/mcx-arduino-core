#!/bin/bash
# Copy the FRDM-MCXA153 part straight into Fritzing's user parts folder
# (use this when dragging the .fzpz into Fritzing fails).
# Quit Fritzing first, run this, then start Fritzing again.
set -e
SRC="$(cd "$(dirname "$0")" && pwd)/FRDM-MCXA153"
DST="$HOME/Documents/Fritzing/parts"
mkdir -p "$DST/user"
cp "$SRC/part.FRDM-MCXA153.fzp" "$DST/user/"
for view in breadboard schematic pcb icon; do
  mkdir -p "$DST/svg/user/$view"
  cp "$SRC"/svg.$view.*.svg "$DST/svg/user/$view/"
  for f in "$DST"/svg/user/$view/svg.$view.FRDM-MCXA153_*.svg; do
    mv "$f" "$DST/svg/user/$view/${f##*/svg.$view.}"
  done
done
echo "installed into $DST"
