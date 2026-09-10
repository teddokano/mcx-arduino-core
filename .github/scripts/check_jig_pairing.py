#!/usr/bin/env python3
"""Validate the GPIO loopback jig wiring against what the sketches actually do.

NOT run by CI (yet). This is a design tool: it holds the wiring of the
shield jig that release_check/31_gpio_loopback is meant to use, and checks
that shorting those pin pairs cannot break any other release_check sketch
that might be run with the jig still installed.

Run it after adding or changing a release_check sketch. If a new sketch
starts driving a pin the jig has tied to something else, this says so --
that is the whole point. `--table` prints the wiring as a fabrication spec.

WHY THIS EXISTS
    The pairing cannot be worked out by hand, and that is not a figure of
    speech -- it was tried and it was wrong. Two examples that a person
    reading the sketches missed and this script caught:

      * MB_CS paired with A2. Sketch 21 drives MB_CS (SPI1) while calling
        analogRead(A2) in the same loop, so A2 would read the chip select.
      * Every MikroBus/Arduino pin pair on A153 (see ALIASES below).

    31 nets across 10 sketches is past what checking by eye survives.

WHAT IT DOES NOT KNOW
    The model is "who drives a pin and who reads it". It has no idea about
    capacitance, drive strength, or trace length. It happily proposed
    hanging N947's A5 -- which carries a 10k pull-up AND a 100nF cap to GND
    on that board -- off MB_RX, an I3C data line. That constraint had to be
    added by hand as CAP_LOADED. If another pin turns out to have unusual
    loading, this script will not discover it; the board figures and the
    schematics in ref/ are the source for that.

ALIASES
    On A153 six MikroBus names are the same silicon as Arduino header pins
    (D4=MB_INT, D5=MB_PWM, D7=MB_RST, D8=MB_TX, D9=MB_RX, A5=MB_AN), so a
    jig wire between two of them shorts nothing and a wire to each of them
    merges nets. N947 has no such aliasing. Everything below therefore
    works on resolved MCU pins, never on Arduino names, and the two boards
    need physically different jigs (14 wires vs 15).
"""

import argparse
import collections
import re
import sys

IO_H = "hardware/nxp/mcx/cores/arduino/r01lib/io.h"

# io.h holds five boards in one file; slice by the CPU guards, never by
# eyeballing line numbers (reading A156's block as A153's already cost one
# round of wrong conclusions).
BOARD_CPU = {"A153": "CPU_MCXA153VLH", "N947": "CPU_MCXN947VDF"}

# The pins release_check/31 walks: the same set 02's walk covers today.
WALK = (
    ["D%d" % n for n in range(14)]
    + ["D18", "D19"]
    + ["A%d" % n for n in range(6)]
    + ["MB_AN", "MB_RST", "MB_CS", "MB_SCK", "MB_MISO", "MB_MOSI",
       "MB_PWM", "MB_INT", "MB_RX", "MB_TX", "MB_SCL", "MB_SDA"]
)

# Pins that carry unusual passive loading and so must not be tied to a bus
# or any other timing-critical line. Board figures in img/ are the source.
CAP_LOADED = {"N947": ["A5"], "A153": []}   # N947 A5: 10k pull-up + 100nF to GND

# The adopted jig wiring -- this is the fabrication spec.
JIG = {
    "A153": [
        ("D0", "D1"), ("D2", "D3"), ("D4", "D5"), ("D6", "D7"),
        ("D11", "D12"), ("MB_MISO", "MB_MOSI"),
        ("D18", "A0"), ("D19", "A1"), ("D10", "A3"), ("D13", "A4"),
        ("MB_CS", "A5"), ("MB_SCK", "D8"), ("A2", "D9"),
        ("MB_SCL", "MB_SDA"),
    ],
    "N947": [
        ("D0", "D1"), ("D2", "D3"), ("D4", "D5"), ("D6", "D7"),
        ("D11", "D12"), ("MB_MISO", "MB_MOSI"),
        ("D8", "D9"), ("D10", "A2"), ("D13", "D18"), ("D19", "MB_CS"),
        ("A3", "A5"), ("A4", "MB_RX"), ("MB_RST", "MB_TX"),
        ("MB_PWM", "MB_SCL"), ("MB_INT", "MB_SDA"),
    ],
}

# N947 has 31 nets -- odd -- so exactly one pin cannot be paired. MB_SCK is
# the cheapest to give up: sketch 12 drives it as the SPI1 clock, so a dead
# one fails 12 outright. Any of the other 18 also admits a solution.
ORPHAN = {"A153": [], "N947": ["MB_SCK"]}

# ---------------------------------------------------------------------------
# Two boards, not one. JIG above is board A; the I2C/I3C devices live on a
# separate board B that is swapped in, never stacked with A.
#
# That split is what keeps JIG above valid. A device sitting on a bus makes
# that bus live in every sketch that touches it, so a bus pin's partner would
# have to be a pin no sketch ever drives *or reads*. Solving under that rule
# costs N947 at least three more unpaired pins. Keeping the devices on their
# own board costs nothing and loses no coverage.
#
# The logic analyser taps are split the same way, by which sketches actually
# run with each board fitted, so neither needs more than eight channels.
LA_TAPS = {
    "A": [  # fitted for 12 (SPI/SPI1 loopback) -- protocol and clock rate
        ("SPI SCLK", "D13"), ("SPI MOSI", "D11"),
        ("SPI MISO", "D12"), ("SPI CS", "D10"),
        ("SPI1 SCK", "MB_SCK"), ("SPI1 MOSI", "MB_MOSI"),
        ("SPI1 MISO", "MB_MISO"), ("SPI1 CS", "MB_CS"),
    ],
    "B": [  # fitted for 22 / 01 / 05 -- I2C and I3C protocol and clock rate
        ("Wire SDA", "D18"), ("Wire SCL", "D19"),
        ("Wire2 SDA", "MB_SDA"), ("Wire2 SCL", "MB_SCL"),
        ("Wire1 SDA", "MB_RX"), ("Wire1 SCL", "MB_TX"),
    ],
}

# A tap is only meaningful where that peripheral is actually on that pin.
# A153 has no Wire2 at all (one LPI2C), and its Wire1 is not on the MikroBus
# pins even though those names resolve there -- see LA_OFF_HEADER.
TAP_NA = {"A153": {"Wire2 SDA": "no Wire2 on this board (one LPI2C)",
                   "Wire2 SCL": "no Wire2 on this board (one LPI2C)",
                   "Wire1 SDA": "not on this pin here -- see below",
                   "Wire1 SCL": "not on this pin here -- see below"},
          "N947": {}}

# Board B carries one real device: the on-board P3T1755 (Wire1) and the Wire2
# scan need no external part, so the LM75-family sensor sketch 22 wants is the
# only thing to mount.
DEVICES = {"B": [("LM75-family sensor", "D18", "D19")]}

# Wire1 taps do not reach the same way on both boards. On N947 Wire1 is
# MB_RX/MB_TX, straight off the MikroBus connector. On A153 it is P0_16/P0_17,
# which the schematic (ref/FRDM-MCXA153.pdf p7, the I3C sensor sheet) brings
# out to J20/J21 -- separate points, not part of the Arduino header footprint,
# so board B needs flying leads there rather than a stacked connection.
LA_OFF_HEADER = {"A153": ["Wire1 via J20/J21 (P0_16/P0_17) -- flying leads"],
                 "N947": []}


def load_board(board):
    """Return (net -> [arduino names], arduino name -> net) for one board."""
    src = open(IO_H, newline="").read().split("\n")
    guard = BOARD_CPU[board]
    start = None
    for i, line in enumerate(src):
        if re.search(r"(if|elif|ifdef)\s.*" + guard, line):
            start = i
            break
    if start is None:                       # N947 is the leading block
        start = 0
    end = len(src)
    for i in range(start + 1, len(src)):
        if re.search(r"#\s*(elif|else)\b", src[i]) and "CPU_" in src[i]:
            end = i
            break

    defines = {}
    for line in src[start:end]:
        m = re.match(r"\s*#define\s+(\w+)\s+(\w+)", line)
        if m:
            defines.setdefault(m.group(1), m.group(2))

    def resolve(name):                      # follow alias chains to a raw pin
        v = defines.get(name)
        for _ in range(8):
            if v not in defines:
                break
            v = defines[v]
        return v

    nets, by_name = collections.OrderedDict(), {}
    for name in WALK:
        if name in defines and resolve(name) != "DISABLED_PIN":
            nets.setdefault(resolve(name), []).append(name)
            by_name[name] = resolve(name)
    return nets, by_name


def sketch_model(board):
    """Per sketch: (pins driven, pins whose level is read, intended shorts).

    A pair is illegal for a sketch when one end is driven and the other is
    driven or read -- unless that short is what the sketch wants, which is
    what the third set records.
    """
    serial1 = ("D0", "D1") if board == "A153" else ("MB_TX", "MB_RX")
    # A153's Wire1 (I3C) is on P0_16/P0_17, off the Arduino header and out
    # of the walk entirely; A153 has one LPI2C so it has no Wire2at all.
    wire1 = () if board == "A153" else ("MB_RX", "MB_TX")
    wire2 = () if board == "A153" else ("MB_SDA", "MB_SCL")
    blue = "D3" if board == "A153" else "D6"
    spi1 = ("MB_CS", "MB_SCK", "MB_MOSI")
    spi1_lb = frozenset(("MB_MOSI", "MB_MISO"))
    bus = lambda *g: set().union(*[set(x) for x in g]) if g else set()

    s = {}
    s["01"] = (bus(["D2"], wire1, wire2), bus(["D2", "A2"], wire1, wire2), set())
    s["03"] = ({blue}, {"A5"} if board == "N947" else set(), set())
    s["04"] = (bus(spi1, ["D13"], serial1, wire1),
               bus(["MB_MISO"], serial1, wire1),
               {frozenset(serial1), spi1_lb})
    if board == "N947":
        s["05"] = (set(wire2), set(wire2), set())
    s["11"] = (bus(["D2"], serial1), bus(["D3"], serial1),
               {frozenset(serial1), frozenset(("D2", "D3"))})
    s["12"] = (bus(["D10", "D11", "D13"], spi1), {"D12", "MB_MISO"},
               {frozenset(("D11", "D12")), spi1_lb})
    s["13"] = ({"D0", "D2", "D5", "D6"}, {"D1", "D3", "D4", "D7", "D8"},
               {frozenset(p) for p in (("D0", "D1"), ("D2", "D3"),
                                       ("D4", "D5"), ("D6", "D7"))})
    s["21"] = (bus(spi1, ["D13"], serial1, wire1, wire2),
               bus(["MB_MISO", "A2"], serial1, wire1, wire2),
               {frozenset(serial1), spi1_lb})
    s["22"] = ({"D18", "D19"}, {"D18", "D19"}, set())
    return s


def same_bus_pairs(board):
    """SDA and its own SCL: shorting these kills the bus outright."""
    pairs = {frozenset(("D18", "D19"))}
    if board == "N947":
        pairs |= {frozenset(("MB_SDA", "MB_SCL")),
                  frozenset(("MB_RX", "MB_TX"))}
    return pairs


def check(board):
    nets, by_name = load_board(board)
    model, forbidden = sketch_model(board), same_bus_pairs(board)
    roled = {by_name[p] for _, (dr, rd, _) in model.items()
             for p in set(dr) | set(rd) if p in by_name}
    capped = {by_name[p] for p in CAP_LOADED[board] if p in by_name}
    problems = []

    for a, b in JIG[board]:
        if a not in by_name or b not in by_name:
            problems.append("%s-%s: name not on this board" % (a, b))
            continue
        na, nb = by_name[a], by_name[b]
        if na == nb:
            problems.append("%s-%s: same MCU pin, the wire shorts nothing" % (a, b))
        if any({na, nb} == {by_name[x], by_name[y]}
               for x, y in (tuple(f) for f in forbidden)):
            problems.append("%s-%s: SDA tied to its own SCL" % (a, b))
        if (na in capped and nb in roled) or (nb in capped and na in roled):
            problems.append("%s-%s: passive-loaded pin hung off a peripheral line" % (a, b))
        for name, (dr, rd, lb) in model.items():
            drn = {by_name[x] for x in dr if x in by_name}
            rdn = {by_name[x] for x in rd if x in by_name}
            lbn = {frozenset({by_name[x] for x in p if x in by_name}) for p in lb}
            if frozenset({na, nb}) in lbn:
                continue
            if na in drn and nb in drn:
                problems.append("%s: %s-%s both driven -- drive fight" % (name, a, b))
            elif na in drn and nb in rdn:
                problems.append("%s: %s driven, corrupts read of %s" % (name, a, b))
            elif nb in drn and na in rdn:
                problems.append("%s: %s driven, corrupts read of %s" % (name, b, a))

    seen = collections.Counter()
    for a, b in JIG[board]:
        for p in (a, b):
            if p in by_name:
                seen[by_name[p]] += 1
    expected_orphans = {by_name[p] for p in ORPHAN[board] if p in by_name}
    for net, names in nets.items():
        label = "=".join(names)
        if seen[net] == 0 and net not in expected_orphans:
            problems.append("%s: not wired and not a declared orphan" % label)
        elif seen[net] > 1:
            problems.append("%s: wired %d times -- merged net, not a pair"
                            % (label, seen[net]))
    return nets, by_name, problems


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--table", action="store_true",
                    help="print the wiring as a fabrication spec")
    args = ap.parse_args()

    failed = False
    for board in ("A153", "N947"):
        nets, by_name, problems = check(board)
        orphans = ORPHAN[board]
        print("%s: %d nets, %d jig wires, orphan: %s"
              % (board, len(nets), len(JIG[board]),
                 ", ".join(orphans) if orphans else "none"))
        if args.table:
            fmt = lambda n: "=".join(nets[by_name[n]]) if n in by_name else n + "(?)"
            print("  board A -- loopback wiring:")
            for i, (a, b) in enumerate(JIG[board], 1):
                print("    %2d  %-20s - %s" % (i, fmt(a), fmt(b)))
            for tag in ("A", "B"):
                print("  board %s -- logic analyser taps:" % tag)
                for ch, (role, pin) in enumerate(LA_TAPS[tag]):
                    note = TAP_NA[board].get(role, "")
                    if pin not in by_name:
                        note = "not on this board"
                    print("    CH%-2d %-11s %-16s%s"
                          % (ch, role, fmt(pin), "  (%s)" % note if note else ""))
                for extra in (LA_OFF_HEADER[board] if tag == "B" else []):
                    print("    +    %s" % extra)
                for name, sda, scl in DEVICES.get(tag, []):
                    print("    dev  %s on %s / %s" % (name, fmt(sda), fmt(scl)))
        for p in problems:
            print("  ! " + p)
        if problems:
            failed = True
        else:
            print("  ok: no pair can disturb any other release_check sketch")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
