#!/usr/bin/env python3
"""Generate the Fritzing parts (.fzpz) for the FRDM boards this core supports.

Usage: python3 generate_fritzing_parts.py [A153 A156 N236 N947 C444]   (default: all)
Writes <board>/ (the unpacked part) and FRDM-MCX<board>.fzpz next to this script.

What is modelled: the Arduino sockets J1-J4 (the Arduino signals sit on the outer
column, the even pins), PWM0-PWM5 on the inner column of J3 where the board has
them, and the mikroBUS sockets J5/J6. Pin names, MCU pins and notes are read from
the PIN_MAPPING_<board>.md tables, so they cannot drift from the documentation.

Positions are in mm from the board's top-left corner (the boards are drawn
portrait, USB at the top). The Arduino headers have the same layout on every board
(the standard Arduino R3 shield pattern), only their vertical offset differs. The
offsets, board sizes, and the mikroBUS/button/chip positions were measured from the
board photos in img/ (about +-1 mm), so the parts are not for PCB design.
"""
import hashlib
import os
import re
import sys
import zipfile
from xml.sax.saxutils import escape

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
P = 2.54
BOARD_W = 53.34
XL, XI, XR = 2.54, 2.54 * 2, 50.8  # left outer, left inner (PWM), right outer columns

# Arduino header contents, top to bottom.  (name, label, kind)
J2 = [("D19", "D19/SCL", "io"), ("D18", "D18/SDA", "io"), ("AREF", "AREF", "power"), ("GND", "GND", "gnd"),
      ("D13", "D13/SCK", "io"), ("D12", "D12/MISO", "io"), ("D11", "D11/MOSI", "io"), ("D10", "D10/CS", "io"),
      ("D9", "D9", "io"), ("D8", "D8", "io")]
J1 = [("D7", "D7", "io"), ("D6", "D6", "io"), ("D5", "D5", "io"), ("D4", "D4", "io"), ("D3", "D3", "io"),
      ("D2", "D2", "io"), ("D1", "D1/TX", "io"), ("D0", "D0/RX", "io")]
J3 = [("NC", "NC", "nc"), ("IOREF", "IOREF", "power"), ("RESET", "RESET", "io"), ("3V3", "3V3", "power"),
      ("5V", "5V", "power"), ("GND", "GND", "gnd"), ("GND", "GND", "gnd"), ("VIN", "VIN", "power")]
J4 = ["A0", "A1", "A2", "A3", "A4", "A5"]
MB6 = ["AN", "RST", "CS", "SCK", "MISO", "MOSI", "3V3", "GND"]
MB5 = ["PWM", "INT", "RX", "TX", "SCL", "SDA", "5V", "GND"]
FIXED_DESC = {"AREF": "AREF", "NC": "Not connected", "IOREF": "IOREF (3.3V)", "RESET": "RESET (MCU reset)",
              "3V3": "3.3V regulated out", "5V": "5.0V out (USB)", "GND": "GND", "VIN": "VIN (P5V_VIN)",
              "MB_3V3": "mikroBUS 3.3V", "MB_5V": "mikroBUS 5V", "MB_GND": "mikroBUS GND"}

# usb: (x, width) of the connectors at the top edge.  buttons: (x, y, name, caption).
# chip: (cx, cy, size, text).  mb: (x of J6, x of J5, y of the first pin).
# shared: (arduino pin, mikroBUS pin) pairs that are the same MCU pin (joined as a bus).
BOARDS = {
    "A153": dict(h=88.9, y0=26.3, usb=[(16, 8.4), (30, 8.4)], pwm=True,
                 buttons=[(48.9, 9.2, "SW1", "RESET"), (4.1, 78, "SW2", "ISP"), (48.9, 78, "SW3", "WAKEUP")],
                 chip=(27, 60, 10, "MCX A153"), mb=(15.3, 38.5, 52.3),
                 shared=[("A5", "MB_AN"), ("D7", "MB_RST"), ("D5", "MB_PWM"), ("D4", "MB_INT"),
                         ("D9", "MB_RX"), ("D8", "MB_TX")], seed=b"frdm-mcxa153-test"),
    "A156": dict(h=92.5, y0=15.5, usb=[(18.1, 8.3), (33, 8.3)], pwm=True,
                 buttons=[(49, 7.3, "SW1", "RESET"), (2.9, 80.5, "SW3", "ISP"), (48.9, 80.5, "SW2", "WAKEUP")],
                 chip=(26.9, 58, 11, "MCX A156"), mb=(15.3, 38.5, 49.7), shared=[]),
    "N236": dict(h=112.7, y0=12.1, usb=[(16.2, 8.5), (31, 8.5)], pwm=True,
                 buttons=[(49, 7.9, "SW1", "RESET"), (3.6, 101.5, "SW3", "ISP"), (48, 101.5, "SW2", "WAKEUP")],
                 chip=(28.2, 60.9, 11, "MCX N236"), mb=(15.3, 38.0, 76.4),
                 shared=[("D13", "MB_SCK"), ("D12", "MB_MISO"), ("D11", "MB_MOSI"), ("D18", "MB_CS"),
                         ("D0", "MB_RX"), ("D1", "MB_TX")]),
    "N947": dict(h=112.1, y0=13.2, usb=[(5.9, 8.6), (36.7, 8.5)], rj45=(17.7, 15.9), pwm=True, title_y=36,
                 buttons=[(48.7, 4.4, "SW1", "RESET"), (3.3, 102.5, "SW3", "ISP"), (46.8, 102.5, "SW2", "")],
                 chip=(27, 67.7, 9, "MCX N947"), mb=(17.3, 40.0, 81.1), skip=["A0", "A1"], shared=[]),
    "C444": dict(h=87.3, y0=25.3, usb=[(19.3, 7.6), (33.6, 8.5)], pwm=False,
                 buttons=[(48.6, 8.7, "SW1", "RESET"), (2.9, 77, "SW3", "ISP"), (48.5, 77, "SW2", "WAKEUP")],
                 chip=(26.3, 58, 9, "MCX C444"), mb=(15.3, 38.0, 50.6),
                 shared=[("A0", "MB_AN"), ("D7", "MB_RST"), ("D10", "MB_CS"), ("D13", "MB_SCK"),
                         ("D12", "MB_MISO"), ("D11", "MB_MOSI"), ("D3", "MB_PWM"), ("D2", "MB_INT"),
                         ("D19", "MB_RX"), ("D18", "MB_TX"), ("A5", "MB_SCL"), ("A4", "MB_SDA")]),
}
PWM_NAMES = ["PWM%d" % i for i in range(6)]
COLORS = {"mb": "#9be8ae", "pwm": "#e6a0ff"}


def f(v):
    return ("%.3f" % v).rstrip("0").rstrip(".")


def read_pin_docs(board):
    """name -> 'MCU pin  note' from the first table row that names each pin."""
    path = os.path.join(REPO, "PIN_MAPPING_%s.md" % board)
    out = {}
    for line in open(path, encoding="utf-8"):
        if not line.startswith("|"):
            continue
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) < 2:
            continue
        names = re.findall(r"`([^`]+)`", cells[0])
        # `A0`-`A3` stands for A0, A1, A2, A3
        m = re.fullmatch(r"`([A-Z]+)(\d+)`-`[A-Z]+(\d+)`", cells[0])
        if m:
            names = ["%s%d" % (m.group(1), i) for i in range(int(m.group(2)), int(m.group(3)) + 1)]
        if not names or not all(re.fullmatch(r"(D\d+|A\d|PWM\d|MB_[A-Z0-9]+)", n) for n in names):
            continue
        pins = re.findall(r"`([^`]+)`", cells[1])
        note = re.sub(r"\*\*|`", "", cells[2]).strip() if len(cells) > 2 and not re.fullmatch(r"sm\d", cells[2]) else ""
        for i, n in enumerate(names):
            mcu = pins[i] if len(pins) == len(names) else (pins[0] if len(pins) == 1 else "")
            out.setdefault(n, "%s%s%s" % (n, "  (%s)" % mcu if mcu else "", ("  " + note) if note else ""))
    return out


def build_pins(board, cfg):
    docs = read_pin_docs(board)
    y0 = cfg["y0"]
    pins = []

    def add(name, label, x, y, kind, key=None, desc=None):
        pins.append(dict(name=name, label=label, x=x, y=y, kind=kind, key=key or name,
                         desc=desc or docs.get(name) or FIXED_DESC.get(name, name)))

    for i, (n, lab, k) in enumerate(J2):
        add(n, lab, XR, y0 + P * i, k, key="GND#2" if n == "GND" else None)
    for i, (n, lab, k) in enumerate(J1):
        add(n, lab, XR, y0 + 26.9 + P * i, k)
    g = 0
    for i, (n, lab, k) in enumerate(J3):
        key = None
        if n == "GND":
            key = "GND#%d" % g
            g += 1
        add(n, lab, XL, y0 + 9.5 + P * i, k, key=key)
    for i, n in enumerate(J4):
        if n not in cfg.get("skip", []):
            add(n, n, XL, y0 + 32 + P * i, "analog")
    if cfg["pwm"]:
        for i, n in enumerate(PWM_NAMES):
            add(n, n, XI, y0 + 9.5 + P * (2 + i), "pwm")
    x6, x5, ym = cfg["mb"]
    for col, names, x in (("6", MB6, x6), ("5", MB5, x5)):
        for i, n in enumerate(names):
            full = "MB_" + n
            kind = "power" if n in ("3V3", "5V") else "gnd" if n == "GND" else "mb"
            key = {"3V3": "MB3V3", "5V": "MB5V"}.get(n, full if n != "GND" else "MBGND" + col)
            add(full, n, x, ym + P * i, kind, key=key)
    return pins


# ---------------------------------------------------------------- breadboard
def breadboard(board, cfg, pins):
    H = cfg["h"]
    x6, x5, ym = cfg["mb"]
    s = ['<?xml version="1.0" encoding="UTF-8"?>',
         '<svg xmlns="http://www.w3.org/2000/svg" width="%smm" height="%smm" viewBox="0 0 %s %s" '
         'version="1.2" baseProfile="tiny">' % (f(BOARD_W), f(H), f(BOARD_W), f(H)),
         '<g id="breadboardbreadboard">',
         '<rect x="0" y="0" width="%s" height="%s" rx="1.5" fill="#1d1d1f" stroke="#000" stroke-width="0.2"/>'
         % (f(BOARD_W), f(H))]
    for (x, y) in [(3, 3), (BOARD_W - 3, 3), (3, H - 3), (BOARD_W - 3, H - 3)]:
        s.append('<circle cx="%s" cy="%s" r="1.3" fill="#c9a227"/><circle cx="%s" cy="%s" r="0.7" fill="#fff"/>'
                 % (f(x), f(y), f(x), f(y)))
    for (x, w) in cfg["usb"]:
        s.append('<rect x="%s" y="-1" width="%s" height="7.5" fill="#b8b8b8" stroke="#666" stroke-width="0.2"/>'
                 '<rect x="%s" y="1" width="%s" height="1.4" rx="0.7" fill="#333"/>'
                 % (f(x), f(w), f(x + w * 0.18), f(w * 0.64)))
    if "rj45" in cfg:
        x, w = cfg["rj45"]
        s.append('<rect x="%s" y="-1" width="%s" height="14" fill="#a8a8a8" stroke="#666" stroke-width="0.2"/>'
                 '<rect x="%s" y="1" width="%s" height="7" fill="#444"/>' % (f(x), f(w), f(x + 2), f(w - 4)))
    cx, cy, size, text = cfg["chip"]
    s.append('<rect x="%s" y="%s" width="%s" height="%s" fill="#2b2b2b" stroke="#555" stroke-width="0.2"/>'
             '<circle cx="%s" cy="%s" r="0.6" fill="#777"/>'
             '<text x="%s" y="%s" font-size="1.4" font-family="Helvetica,Arial,sans-serif" fill="#ddd" '
             'text-anchor="middle">%s</text>'
             % (f(cx - size / 2), f(cy - size / 2), f(size), f(size), f(cx - size / 2 + 1.5), f(cy - size / 2 + 1.5),
                f(cx), f(cy + 0.5), text))
    for (x, y, t, sub) in cfg["buttons"]:
        s.append('<rect x="%s" y="%s" width="4.5" height="4.5" fill="#ccc" stroke="#555" stroke-width="0.2"/>'
                 '<circle cx="%s" cy="%s" r="1.3" fill="#333"/>' % (f(x - 2.25), f(y - 2.25), f(x), f(y)))
        s.append('<text x="%s" y="%s" font-size="1.4" font-family="Helvetica,Arial,sans-serif" fill="#fff" '
                 'text-anchor="middle">%s</text>' % (f(x), f(y + 4.2), t))
        if sub:
            s.append('<text x="%s" y="%s" font-size="1.1" font-family="Helvetica,Arial,sans-serif" fill="#bbb" '
                     'text-anchor="middle">%s</text>' % (f(x), f(y + 5.7), sub))

    def header(x0, y0, rows, w=P + 2.8):
        s.append('<rect x="%s" y="%s" width="%s" height="%s" fill="#111" stroke="#444" stroke-width="0.2"/>'
                 % (f(x0 - 1.4), f(y0 - 1.4), f(w), f(P * (rows - 1) + 2.8)))

    y0 = cfg["y0"]
    header(XR - P, y0, 10)
    header(XR - P, y0 + 26.9, 8)
    header(XL, y0 + 9.5, 8)
    header(XL, y0 + 32, 6)
    for x in (x6, x5):
        header(x, ym, 8, 2.8)
    s.append('<text x="%s" y="%s" font-size="1.6" font-family="Helvetica,Arial,sans-serif" fill="#9be8ae" '
             'text-anchor="middle">mikroBUS</text>' % (f((x6 + x5) / 2), f(ym + P * 7 + 2.4)))
    # title, rotated 90 degrees clockwise
    ty = cfg.get("title_y", 22)
    s.append('<text x="22" y="%s" transform="rotate(90 22 %s)" font-size="3.2" font-family="Helvetica,Arial,sans-serif" '
             'font-weight="bold" fill="#fff" text-anchor="middle">FRDM-MCX%s</text>' % (ty, ty, board))
    s.append('<text x="27" y="%s" transform="rotate(90 27 %s)" font-size="2" font-family="Helvetica,Arial,sans-serif" '
             'fill="#bbb" text-anchor="middle">NXP</text>' % (ty, ty))
    for i, p in enumerate(pins):
        font = lambda sz, fill, anchor="start": ('font-size="%s" font-family="Helvetica,Arial,sans-serif" '
                                                 'fill="%s" text-anchor="%s"' % (sz, fill, anchor))
        if p["x"] == x6:
            s.append('<text x="%s" y="%s" %s>%s</text>' % (f(p["x"] + 1.2), f(p["y"] + 0.5), font(1.5, COLORS["mb"]), escape(p["label"])))
        elif p["x"] == x5:
            s.append('<text x="%s" y="%s" %s>%s</text>' % (f(p["x"] - 1.2), f(p["y"] + 0.5), font(1.5, COLORS["mb"], "end"), escape(p["label"])))
        elif p["x"] == XR:
            s.append('<text x="%s" y="%s" %s>%s</text>' % (f(p["x"] - 1.2), f(p["y"] + 0.6), font(1.7, "#fff", "end"), escape(p["label"])))
        elif p["kind"] == "pwm":
            s.append('<text x="%s" y="%s" %s>%s</text>' % (f(11.5), f(p["y"] + 0.4), font(1.15, COLORS["pwm"]), escape(p["label"])))
        else:
            dx = XL + 1.3 + (P if p["kind"] != "analog" and cfg["pwm"] else 0)
            if p["kind"] != "analog" and not cfg["pwm"]:
                dx = XL + 1.3
            s.append('<text x="%s" y="%s" %s>%s</text>' % (f(dx), f(p["y"] + 0.55), font(1.4, "#fff"), escape(p["label"])))
        s.append('<rect id="connector%dpin" x="%s" y="%s" width="1.6" height="1.6" fill="#000" stroke="#d4a017" stroke-width="0.25"/>'
                 % (i, f(p["x"] - 0.8), f(p["y"] - 0.8)))
    s.append('</g></svg>')
    return "\n".join(s)


# ----------------------------------------------------------------- schematic
def schematic(board, cfg, pins):
    have = {p["key"] for p in pins}
    left = [["IOREF", "RESET", "3V3", "5V", "VIN", "GND#0", "GND#1", "NC"],
            [n for n in J4 if n in have],
            PWM_NAMES if cfg["pwm"] else [],
            None]
    left[3] = ["MB_" + n for n in MB6[:6]] + ["MB3V3", "MBGND6"]
    right = [["D%d" % i for i in range(0, 8)], ["D8", "D9", "D10", "D11", "D12", "D13"],
             ["GND#2", "AREF", "D18", "D19"],
             ["MB_" + n for n in MB5[:6]] + ["MB5V", "MBGND5"]]
    left = [g for g in left if g]
    idx = {p["key"]: i for i, p in enumerate(pins)}
    rows = max(sum(len(g) for g in left) + len(left) - 1, sum(len(g) for g in right) + len(right) - 1)
    W, H = 160, 10 * rows + 40
    BX0, BX1, BY0 = 30, 130, 20
    out = ['<?xml version="1.0" encoding="UTF-8"?>',
           '<svg xmlns="http://www.w3.org/2000/svg" width="%sin" height="%sin" viewBox="0 0 %d %d" version="1.2" baseProfile="tiny">'
           % (f(W / 100), f(H / 100), W, H),
           '<g id="schematic">',
           '<rect x="%d" y="%d" width="%d" height="%d" fill="#fffde8" stroke="#000" stroke-width="1"/>'
           % (BX0, BY0 - 10, BX1 - BX0, 10 * rows + 20),
           '<text x="%d" y="%d" font-size="8" font-family="Helvetica,Arial,sans-serif" font-weight="bold" '
           'text-anchor="middle">FRDM-MCX%s</text>' % ((BX0 + BX1) // 2, BY0 - 1, board)]

    def place(groups, is_left):
        y = BY0 + 5
        for g in groups:
            for key in g:
                i = idx[key]
                name = pins[i]["name"] if pins[i]["kind"] == "mb" else pins[i]["label"]
                if is_left:
                    x1, x2, tx, anchor = BX0 - 20, BX0, BX0 + 3, "start"
                else:
                    x1, x2, tx, anchor = BX1, BX1 + 20, BX1 - 3, "end"
                out.append('<line id="connector%dpin" x1="%d" y1="%d" x2="%d" y2="%d" stroke="#000" stroke-width="1"/>'
                           % (i, x1, y, x2, y))
                out.append('<rect id="connector%dterminal" x="%s" y="%s" width="0.5" height="0.5" fill="none"/>'
                           % (i, f(x1 if is_left else x2 - 0.5), f(y - 0.25)))
                out.append('<text x="%d" y="%d" font-size="6" font-family="Helvetica,Arial,sans-serif" text-anchor="%s">%s</text>'
                           % (tx, y + 2, anchor, escape(name)))
                y += 10
            y += 10
    place(left, True)
    place(right, False)
    out.append('</g></svg>')
    return "\n".join(out)


# ----------------------------------------------------------------------- pcb
def pcb(board, cfg, pins):
    H = cfg["h"]
    out = ['<?xml version="1.0" encoding="UTF-8"?>',
           '<svg xmlns="http://www.w3.org/2000/svg" width="%smm" height="%smm" viewBox="0 0 %s %s" version="1.2" baseProfile="tiny">'
           % (f(BOARD_W), f(H), f(BOARD_W), f(H)),
           '<g id="silkscreen">',
           '<rect x="0.15" y="0.15" width="%s" height="%s" fill="none" stroke="#fff" stroke-width="0.3"/>'
           % (f(BOARD_W - 0.3), f(H - 0.3)),
           '<text x="26.67" y="14" font-size="3" font-family="Helvetica,Arial,sans-serif" fill="#fff" '
           'text-anchor="middle">FRDM-MCX%s</text>' % board,
           '</g>', '<g id="copper1"><g id="copper0">']
    for i, p in enumerate(pins):
        out.append('<circle id="connector%dpad" cx="%s" cy="%s" r="0.95" fill="none" stroke="#f7bd13" stroke-width="0.7"/>'
                   % (i, f(p["x"]), f(p["y"])))
    out.append('</g></g></svg>')
    return "\n".join(out)


def icon():
    return ('<?xml version="1.0" encoding="UTF-8"?>\n'
            '<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 32 32">'
            '<g id="icon"><rect x="8" y="1" width="16" height="30" rx="1" fill="#1d1d1f" stroke="#000"/>'
            '<rect x="11" y="1" width="4" height="4" fill="#bbb"/><rect x="17" y="1" width="4" height="4" fill="#bbb"/>'
            '<rect x="12" y="12" width="8" height="8" fill="#444"/></g></svg>')


# ----------------------------------------------------------------------- fzp
def fzp(board, cfg, pins, module_id, base):
    names = {"A153": "Cortex-M33", "A156": "Cortex-M33", "N236": "Cortex-M33", "N947": "Cortex-M33", "C444": "Cortex-M0+"}
    o = ['<?xml version="1.0" encoding="UTF-8"?>',
         '<module fritzingVersion="0.9.3b" moduleId="%s">' % module_id,
         '  <version>1</version>', '  <author>mcx-arduino-core</author>',
         '  <title>%s</title>' % base, '  <label>%s</label>' % base, '  <date>2026-10-20</date>',
         '  <tags><tag>NXP</tag><tag>MCX</tag><tag>MCX%s</tag><tag>FRDM</tag><tag>Arduino</tag></tags>' % board,
         '  <properties>',
         '    <property name="family">microcontroller board (arduino)</property>',
         '    <property name="type">NXP %s</property>' % base,
         '    <property name="part number">%s</property>' % base,
         '  </properties>',
         '  <description>NXP %s (%s) development board. Arduino-compatible sockets J1-J4 (D0-D13, D18/D19, A0-A5, power)%s '
         'and the mikroBUS sockets J5/J6. Pin names follow the mcx-arduino-core board package.</description>'
         % (base, names[board], ", PWM0-PWM5 on the inner column of J3" if cfg["pwm"] else ""),
         '  <views>',
         '    <iconView><layers image="icon/%s_icon.svg"><layer layerId="icon"/></layers></iconView>' % base,
         '    <breadboardView><layers image="breadboard/%s_breadboard.svg"><layer layerId="breadboardbreadboard"/></layers></breadboardView>' % base,
         '    <schematicView><layers image="schematic/%s_schematic.svg"><layer layerId="schematic"/></layers></schematicView>' % base,
         '    <pcbView><layers image="pcb/%s_pcb.svg"><layer layerId="copper1"/><layer layerId="copper0"/>'
         '<layer layerId="silkscreen"/></layers></pcbView>' % base,
         '  </views>', '  <connectors>']
    for i, p in enumerate(pins):
        o.append('    <connector id="connector%d" name="%s" type="female">' % (i, escape(p["name"])))
        o.append('      <description>%s</description>' % escape(p["desc"]))
        o.append('      <views>')
        o.append('        <breadboardView><p layer="breadboardbreadboard" svgId="connector%dpin"/></breadboardView>' % i)
        o.append('        <schematicView><p layer="schematic" svgId="connector%dpin" terminalId="connector%dterminal"/></schematicView>' % (i, i))
        o.append('        <pcbView><p layer="copper0" svgId="connector%dpad"/><p layer="copper1" svgId="connector%dpad"/></pcbView>' % (i, i))
        o.append('      </views>')
        o.append('    </connector>')
    o.append('  </connectors>')
    by = {}
    for i, p in enumerate(pins):
        by.setdefault(p["name"], []).append(i)
    groups = [("gnd", [i for i, p in enumerate(pins) if p["kind"] == "gnd"]),
              ("3v3", by["3V3"] + by["MB_3V3"]), ("5v", by["5V"] + by["MB_5V"])]
    for a, b in cfg["shared"]:
        groups.append((b.lower(), by[a] + by[b]))
    o.append('  <buses>')
    for bid, ids in groups:
        o.append('    <bus id="%s">' % bid + "".join('<nodeMember connectorId="connector%d"/>' % i for i in ids) + '</bus>')
    o.append('  </buses>')
    o.append('</module>')
    return "\n".join(o)


def build(board):
    cfg = BOARDS[board]
    base = "FRDM-MCX%s" % board
    # Fritzing wants a hex module id and a "part."-prefixed .fzp inside the bundle
    module_id = hashlib.md5(cfg.get("seed", ("frdm-mcx%s-fritzing" % board.lower()).encode())).hexdigest()
    pins = build_pins(board, cfg)
    fzp_name = "part.%s.fzp" % base
    files = {
        fzp_name: fzp(board, cfg, pins, module_id, base),
        "svg.icon.%s_icon.svg" % base: icon(),
        "svg.breadboard.%s_breadboard.svg" % base: breadboard(board, cfg, pins),
        "svg.schematic.%s_schematic.svg" % base: schematic(board, cfg, pins),
        "svg.pcb.%s_pcb.svg" % base: pcb(board, cfg, pins),
    }
    out = os.path.join(HERE, base)
    os.makedirs(out, exist_ok=True)
    for n, c in files.items():
        with open(os.path.join(out, n), "w", encoding="utf-8") as fh:
            fh.write(c)
    z = os.path.join(HERE, base + ".fzpz")
    with zipfile.ZipFile(z, "w", zipfile.ZIP_DEFLATED) as zf:
        for n in files:
            zf.write(os.path.join(out, n), n)
    print("wrote", os.path.basename(z), "with", len(pins), "connectors")


if __name__ == "__main__":
    for b in (sys.argv[1:] or list(BOARDS)):
        build(b)
