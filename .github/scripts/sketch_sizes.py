#!/usr/bin/env python3
"""Flash and RAM of each sketch the regression check builds, against a baseline.

From the first commit to 0.12.0 the core linked the full newlib instead of
newlib-nano, and every sketch was 15-55KB larger for it, unnoticed until
examples stopped fitting FRDM-MCXA153's flash: nothing looked at the sizes. compile_examples.sh and
compile_third_party.sh now measure every sketch they build and compare it
with .github/sizes/<board>.tsv, so a change that makes sketches larger has
to come with a baseline that says so.

  sketch_sizes.py measure <elf>
      Prints "<flash><TAB><ram>" for a built sketch, as the IDE counts them:
      the sections platform.txt's recipe.size.regex and
      recipe.size.regex.data name (the heap counts as RAM).

  sketch_sizes.py check <board> <measured> [--update]
      <measured>: lines "<label><TAB><flash><TAB><ram>", one per sketch.
      Compares them with .github/sizes/<board>.tsv: lists every sketch
      that moved by more than NOISE bytes, and fails if one grew by more
      than --flash-limit (1024) bytes of flash or --ram-limit (512) of RAM.
      Growth adds up against the baseline until someone updates it, so a
      series of small increases fails in the end too. A sketch that got
      smaller by more than the limit gets a warning, to update the
      baseline and keep the gain. Sketches not in the baseline are listed
      as new; baseline entries not measured (the fast tier builds only
      some) are left alone.
      --update writes the measured sizes into the baseline instead (the
      others stay), for when the change is meant. Run the full tier and
      third_party with it, so the baseline covers every sketch. The same
      toolchain should give the same sizes on macOS and in CI, but a
      library that keeps __FILE__ can move by a few bytes with the path
      it is installed at; NOISE leaves such moves out.
"""

import argparse
import os
import re
import struct
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PLATFORM_TXT = os.path.join(REPO, "hardware/nxp/mcx/platform.txt")
BASELINE_DIR = os.path.join(REPO, ".github/sizes")

NOISE = 64  # moves this small are left out of the report


def size_sections():
    """The section names in platform.txt's size regexes: (flash, ram)."""
    names = {}
    with open(PLATFORM_TXT, encoding="utf-8") as f:
        for line in f:
            m = re.match(r"recipe\.size\.regex(\.data)?=(.*)", line.strip())
            if m:
                alt = re.search(r"\(\?:([^)]*)\)", m.group(2))
                names["ram" if m.group(1) else "flash"] = [
                    n.replace("\\", "") for n in alt.group(1).split("|")
                ]
    return names["flash"], names["ram"]


def elf_sections(path):
    """{name: size} of every section of a 32-bit little-endian ELF."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"\x7fELF" or data[4] != 1 or data[5] != 1:
        raise ValueError(f"{path}: not a 32-bit little-endian ELF")
    shoff, = struct.unpack_from("<I", data, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 0x2E)
    headers = [struct.unpack_from("<IIIIII", data, shoff + i * shentsize) for i in range(shnum)]
    strtab = headers[shstrndx][4]
    sections = {}
    for name_off, _type, _flags, _addr, _offset, size in headers:
        end = data.index(b"\0", strtab + name_off)
        sections[data[strtab + name_off:end].decode()] = size
    return sections


def measure(elf):
    flash_names, ram_names = size_sections()
    sections = elf_sections(elf)
    flash = sum(sections.get(n, 0) for n in flash_names)
    ram = sum(sections.get(n, 0) for n in ram_names)
    print(f"{flash}\t{ram}")


def read_sizes(path):
    sizes = {}
    if os.path.exists(path):
        with open(path, encoding="utf-8") as f:
            for line in f:
                if not line.strip() or line.startswith("#"):
                    continue
                label, flash, ram = line.rstrip("\n").split("\t")
                sizes[label] = (int(flash), int(ram))
    return sizes


def write_baseline(path, sizes, board):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(f"# Flash and RAM in bytes of each sketch on {board}, as the IDE counts them.\n")
        f.write("# The regression check fails when one grows by much more; see\n")
        f.write("# .github/scripts/sketch_sizes.py, which writes this file (--update).\n")
        for label in sorted(sizes):
            flash, ram = sizes[label]
            f.write(f"{label}\t{flash}\t{ram}\n")


def plural(n):
    return f"{n} sketch" if n == 1 else f"{n} sketches"


def check(board, measured_path, update, flash_limit, ram_limit):
    baseline_path = os.path.join(BASELINE_DIR, f"{board}.tsv")
    rel_baseline = os.path.relpath(baseline_path, REPO)
    measured = read_sizes(measured_path)
    baseline = read_sizes(baseline_path)

    if update:
        baseline.update(measured)
        write_baseline(baseline_path, baseline, board)
        print(f"Sizes: wrote {len(measured)} sketches into {rel_baseline} ({len(baseline)} in it)")
        return 0

    if not measured:
        print(f"Sizes ({board}): nothing measured")
        return 0
    if not baseline:
        print(f"Sizes ({board}): no baseline ({rel_baseline}); write one with --update")
        return 0

    new = sorted(set(measured) - set(baseline))
    moved = []  # (label, d_flash, d_ram, old, now)
    for label in sorted(set(measured) & set(baseline)):
        old, now = baseline[label], measured[label]
        d_flash, d_ram = now[0] - old[0], now[1] - old[1]
        if abs(d_flash) > NOISE or abs(d_ram) > NOISE:
            moved.append((label, d_flash, d_ram, old, now))
    grown = [m for m in moved if m[1] > flash_limit or m[2] > ram_limit]
    shrunk = [m for m in moved if m[1] < -flash_limit or m[2] < -ram_limit]

    print(f"Sizes ({board}) against {rel_baseline}: {len(measured)} measured, "
          f"{len(moved)} moved by more than {NOISE} bytes, {len(new)} not in it")
    for label, d_flash, d_ram, old, now in sorted(moved, key=lambda m: -abs(m[1])):
        print(f"  flash {d_flash:+7d} ({old[0]} -> {now[0]})  ram {d_ram:+6d}  {label}")
    for label in new:
        print(f"  new: {label} (flash {measured[label][0]}, ram {measured[label][1]})")

    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary and (moved or new):
        with open(summary, "a", encoding="utf-8") as f:
            f.write(f"### Sketch sizes on {board}\n\n| sketch | flash | RAM |\n|---|---|---|\n")
            for label, d_flash, d_ram, old, now in sorted(moved, key=lambda m: -abs(m[1])):
                f.write(f"| {label} | {old[0]} → {now[0]} ({d_flash:+d}) | {old[1]} → {now[1]} ({d_ram:+d}) |\n")
            for label in new:
                f.write(f"| {label} (new) | {measured[label][0]} | {measured[label][1]} |\n")
            f.write("\n")

    hint = (f"If the change is meant, run the full tier and third_party with SIZE_UPDATE=1 "
            f"and commit {rel_baseline}")
    if shrunk:
        print(f"::warning::{plural(len(shrunk))} on {board} got smaller by more than the limit. {hint}, to keep the gain")
    if grown:
        print(f"::error::{plural(len(grown))} on {board} grew by more than {flash_limit} bytes of flash "
              f"or {ram_limit} of RAM. {hint}")
        return 1
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("measure")
    p.add_argument("elf")
    p = sub.add_parser("check")
    p.add_argument("board")
    p.add_argument("measured")
    p.add_argument("--update", action="store_true")
    p.add_argument("--flash-limit", type=int, default=1024)
    p.add_argument("--ram-limit", type=int, default=512)
    args = parser.parse_args()

    if args.command == "measure":
        measure(args.elf)
        return 0
    return check(args.board, args.measured, args.update, args.flash_limit, args.ram_limit)


if __name__ == "__main__":
    sys.exit(main())
