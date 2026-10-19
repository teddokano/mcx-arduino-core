#!/usr/bin/env python3
"""Generate the Fritzing part (.fzpz) for the NXP FRDM-MCXA153.

Pin positions come from the board schematic (ref/FRDM-MCXA153.pdf, sheet 8) and
the pin diagram (img/pins-FRDM-MCXA153-ard.png). Only the Arduino-compatible
sockets (J1-J4) are modelled: the Arduino signals sit on the even pins of
J1/J2 (right side) and J3/J4 (left side); PWM0-PWM5 are on the inner column of
J3 (rows RESET..VIN).

Usage: python3 generate_frdm_mcxa153.py   (writes into ./FRDM-MCXA153/)
"""
import os
import zipfile
from xml.sax.saxutils import escape

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "FRDM-MCXA153")
# Fritzing wants a hex id and a "part."-prefixed .fzp name inside the .fzpz (plain names are rejected)
MODULE_ID = "a30321f30d74dfa203ca85f4c1f7130c"
FZP_NAME = "part.FRDM-MCXA153.fzp"
BASE = "FRDM-MCXA153"
P = 2.54
BOARD_W, BOARD_H = 53.34, 88.9
XR, XL, XI = 50.4, 2.9, 2.9 + P  # right outer, left outer, left inner column (mm)

# (name, description, group, side) in connector-id order.  group is used for
# the schematic layout; side: R/L.
pins = []  # dicts: name, desc, x, y, kind, label


def add(name, desc, x, y, kind, label=None, key=None):
    pins.append(dict(name=name, desc=desc, x=x, y=y, kind=kind, label=label or name,
                     key=key or name))


# J2 (right, outer column = even pins 20..2, top to bottom)
j2 = [("D19/SCL", "D19 / I2C_SCL  (P1_9, Wire SCL)", "io"),
      ("D18/SDA", "D18 / I2C_SDA  (P1_8, Wire SDA)", "io"),
      ("AREF", "AREF", "power"),
      ("GND", "GND", "gnd"),
      ("D13/SCK", "D13 / SPI SCLK  (P2_12)", "io"),
      ("D12/MISO", "D12 / SPI MISO  (P2_16)", "io"),
      ("D11/MOSI", "D11 / SPI MOSI  (P2_13)", "io"),
      ("D10/CS", "D10 / SPI CS  (P2_6)", "io"),
      ("D9", "D9  (P3_14)", "io"),
      ("D8", "D8  (P3_15)", "io")]
for i, (n, d, k) in enumerate(j2):
    add(n, d, XR, 26.3 + P * i, k, key="GND#2" if n == "GND" else None)
# J1 (right): D7..D0, top to bottom
j1 = [("D7", "D7  (P3_1)"), ("D6", "D6  (P3_13, green LED)"),
      ("D5", "D5  (P3_12, red LED)"), ("D4", "D4  (P2_5)"),
      ("D3", "D3  (P3_0, blue LED)"), ("D2", "D2  (P2_4)"),
      ("D1/TX", "D1 / Serial1 TX  (P1_5)"), ("D0/RX", "D0 / Serial1 RX  (P1_4)")]
for i, (n, d) in enumerate(j1):
    add(n, d, XR, 53.2 + P * i, "io")
# J3 (left outer): power header
j3 = [("NC", "Not connected", "nc"), ("IOREF", "IOREF (3.3V)", "power"),
      ("RESET", "RESET (MCU reset, P1_29)", "io"), ("3V3", "3.3V regulated out", "power"),
      ("5V", "5.0V out (USB)", "power"), ("GND", "GND", "gnd"),
      ("GND", "GND", "gnd"), ("VIN", "VIN (P5V_VIN)", "power")]
gcount = 0
for i, (n, d, k) in enumerate(j3):
    key = None
    if n == "GND":
        key = "GND#%d" % gcount
        gcount += 1
    add(n, d, XL, 35.8 + P * i, k, key=key)
# J4 (left outer): analog
j4 = [("A0", "A0  (P1_10, ADC)"), ("A1", "A1  (P1_12, ADC)"), ("A2", "A2  (P1_13, ADC)"),
      ("A3", "A3  (P2_0, ADC)"), ("A4", "A4  (P3_31, digital only)"),
      ("A5", "A5  (P3_30, digital only)")]
for i, (n, d) in enumerate(j4):
    add(n, d, XL, 58.3 + P * i, "analog")
# J3 inner column: PWM0..5 (rows RESET..VIN); P3_11 .. P3_6
for i in range(6):
    add("PWM%d" % i, "PWM%d  (P3_%d, FlexPWM0)" % (i, 11 - i), XI, 35.8 + P * (2 + i), "pwm")

# MikroBus sockets J6 (left, x~15.4) and J5 (right, x~38.7), 8 pins each, top to bottom.
# MB_AN/RST/PWM/INT/RX/TX are the same MCU pins as A5/D7/D5/D4/D9/D8 (see PIN_MAPPING_A153.md).
XM6, XM5, YM = 15.4, 38.7, 52.3
mb6 = [("MB_AN", "AN  (P3_30, same pin as A5)", "mb", None), ("MB_RST", "RST  (P3_1, same pin as D7)", "mb", None),
       ("MB_CS", "CS  (P1_3, SPI1 CS)", "mb", None), ("MB_SCK", "SCK  (P1_1, SPI1 SCK)", "mb", None),
       ("MB_MISO", "MISO  (P1_2, SPI1 MISO)", "mb", None), ("MB_MOSI", "MOSI  (P1_0, SPI1 MOSI)", "mb", None),
       ("MB_3V3", "mikroBUS 3.3V", "power", "MB3V3"), ("MB_GND", "mikroBUS GND", "gnd", "MBGND0")]
mb5 = [("MB_PWM", "PWM  (P3_12, same pin as D5)", "mb", None), ("MB_INT", "INT  (P2_5, same pin as D4)", "mb", None),
       ("MB_RX", "RX  (P3_14, same pin as D9)", "mb", None), ("MB_TX", "TX  (P3_15, same pin as D8)", "mb", None),
       ("MB_SCL", "SCL  (P3_27, I2C)", "mb", None), ("MB_SDA", "SDA  (P3_28, I2C)", "mb", None),
       ("MB_5V", "mikroBUS 5V", "power", "MB5V"), ("MB_GND", "mikroBUS GND", "gnd", "MBGND1")]
for i, (n, d, k, key) in enumerate(mb6):
    add(n, d, XM6, YM + P * i, k, label=n[3:] if k == "mb" else n[3:], key=key or n)
for i, (n, d, k, key) in enumerate(mb5):
    add(n, d, XM5, YM + P * i, k, label=n[3:], key=key or n)

COLORS = {"mb": "#2a9d4a", "io": "#2a7ab5", "analog": "#e07b00", "pwm": "#a020c0", "power": "#d02020",
          "gnd": "#222222", "nc": "#888888"}


def f(v):
    return ("%.3f" % v).rstrip("0").rstrip(".")


# ---------------------------------------------------------------- breadboard
def breadboard():
    s = []
    s.append('<?xml version="1.0" encoding="UTF-8"?>')
    s.append('<svg xmlns="http://www.w3.org/2000/svg" width="%smm" height="%smm" '
             'viewBox="0 0 %s %s" version="1.2" baseProfile="tiny">'
             % (f(BOARD_W), f(BOARD_H), f(BOARD_W), f(BOARD_H)))
    s.append('<g id="breadboardbreadboard">')
    s.append('<rect x="0" y="0" width="%s" height="%s" rx="1.5" fill="#1d1d1f" stroke="#000" stroke-width="0.2"/>'
             % (f(BOARD_W), f(BOARD_H)))
    # mounting holes
    for (x, y) in [(3, 3), (50.3, 3), (3, 85.9), (50.3, 85.9)]:
        s.append('<circle cx="%s" cy="%s" r="1.3" fill="#c9a227"/><circle cx="%s" cy="%s" r="0.7" fill="#fff"/>'
                 % (f(x), f(y), f(x), f(y)))
    # two USB-C connectors at the top (MCU-Link, target USB)
    for x in (16, 30):
        s.append('<rect x="%s" y="-1" width="8.4" height="7.5" fill="#b8b8b8" stroke="#666" stroke-width="0.2"/>' % f(x))
        s.append('<rect x="%s" y="1" width="5.4" height="1.4" rx="0.7" fill="#333"/>' % f(x + 1.5))
    # MCU
    s.append('<rect x="22" y="55" width="10" height="10" fill="#2b2b2b" stroke="#555" stroke-width="0.2"/>')
    s.append('<circle cx="23.5" cy="56.5" r="0.6" fill="#777"/>')
    s.append('<text x="27" y="60.5" font-size="1.4" font-family="Helvetica,Arial,sans-serif" '
             'fill="#ddd" text-anchor="middle">MCX A153</text>')
    # push buttons
    # SW1 RESET (top right), SW2 ISP (bottom left), SW3 WAKEUP (bottom right)
    for (x, y, t, sub) in [(48.9, 9.2, "SW1", "RESET"), (4.1, 78, "SW2", "ISP"), (48.9, 78, "SW3", "WAKEUP")]:
        s.append('<rect x="%s" y="%s" width="4.5" height="4.5" fill="#ccc" stroke="#555" stroke-width="0.2"/>'
                 '<circle cx="%s" cy="%s" r="1.3" fill="#333"/>' % (f(x - 2.25), f(y - 2.25), f(x), f(y)))
        s.append('<text x="%s" y="%s" font-size="1.4" font-family="Helvetica,Arial,sans-serif" fill="#fff" '
                 'text-anchor="middle">%s</text>' % (f(x), f(y + 4.2), t))
        s.append('<text x="%s" y="%s" font-size="1.1" font-family="Helvetica,Arial,sans-serif" fill="#bbb" '
                 'text-anchor="middle">%s</text>' % (f(x), f(y + 5.7), sub))
    # header sockets (double rows)
    def header(x0, y0, rows):
        s.append('<rect x="%s" y="%s" width="%s" height="%s" fill="#111" stroke="#444" stroke-width="0.2"/>'
                 % (f(x0 - 1.4), f(y0 - 1.4), f(P + 2.8), f(P * (rows - 1) + 2.8)))
    header(XR - P, 26.3, 10)
    header(XR - P, 53.2, 8)
    header(XL, 35.8, 8)
    header(XL, 58.3, 6)
    for x0 in (XM6, XM5):
        s.append('<rect x="%s" y="%s" width="2.8" height="%s" fill="#111" stroke="#444" stroke-width="0.2"/>'
                 % (f(x0 - 1.4), f(YM - 1.4), f(P * 7 + 2.8)))
    s.append('<text x="26.67" y="70.5" font-size="1.6" font-family="Helvetica,Arial,sans-serif" '
             'fill="#9be8ae" text-anchor="middle">mikroBUS</text>')
    s.append('<text x="22" y="32" transform="rotate(90 22 32)" font-size="3.2" font-family="Helvetica,Arial,sans-serif" '
             'font-weight="bold" fill="#fff" text-anchor="middle">FRDM-MCXA153</text>')
    s.append('<text x="27" y="32" transform="rotate(90 27 32)" font-size="2" font-family="Helvetica,Arial,sans-serif" '
             'fill="#bbb" text-anchor="middle">NXP</text>')
    # labels and pins
    for i, p in enumerate(pins):
        c = COLORS[p["kind"]]
        right = abs(p["x"] - XR) < 0.01
        if p["x"] in (XM6, XM5):
            if p["x"] == XM6:
                s.append('<text x="%s" y="%s" font-size="1.5" font-family="Helvetica,Arial,sans-serif" '
                         'fill="#9be8ae">%s</text>' % (f(p["x"] + 1.2), f(p["y"] + 0.5), escape(p["label"])))
            else:
                s.append('<text x="%s" y="%s" font-size="1.5" font-family="Helvetica,Arial,sans-serif" '
                         'fill="#9be8ae" text-anchor="end">%s</text>' % (f(p["x"] - 1.2), f(p["y"] + 0.5), escape(p["label"])))
        elif right:
            s.append('<text x="%s" y="%s" font-size="1.7" font-family="Helvetica,Arial,sans-serif" '
                     'fill="#fff" text-anchor="end">%s</text>' % (f(p["x"] - 1.2), f(p["y"] + 0.6), escape(p["label"])))
        elif p["kind"] == "pwm":
            s.append('<text x="%s" y="%s" font-size="1.15" font-family="Helvetica,Arial,sans-serif" '
                     'fill="#e6a0ff">%s</text>' % (f(11.5), f(p["y"] + 0.55), escape(p["label"])))
        else:
            s.append('<text x="%s" y="%s" font-size="1.4" font-family="Helvetica,Arial,sans-serif" '
                     'fill="#fff">%s</text>' % (f(XL + 1.3 + (P if p["kind"] != "analog" else 0)),
                                                f(p["y"] + 0.6), escape(p["label"])))
        s.append('<rect id="connector%dpin" x="%s" y="%s" width="1.6" height="1.6" fill="%s" stroke="#d4a017" stroke-width="0.25"/>'
                 % (i, f(p["x"] - 0.8), f(p["y"] - 0.8), "#000"))
    s.append('</g></svg>')
    return "\n".join(s)


# ----------------------------------------------------------------- schematic
def schematic():
    left_groups = [["IOREF", "RESET", "3V3", "5V", "VIN", "GND#0", "GND#1", "NC"],
                   ["A0", "A1", "A2", "A3", "A4", "A5"],
                   ["PWM0", "PWM1", "PWM2", "PWM3", "PWM4", "PWM5"],
                   ["MB_AN", "MB_RST", "MB_CS", "MB_SCK", "MB_MISO", "MB_MOSI", "MB3V3", "MBGND0"]]
    right_groups = [["D0/RX", "D1/TX", "D2", "D3", "D4", "D5", "D6", "D7"],
                    ["D8", "D9", "D10/CS", "D11/MOSI", "D12/MISO", "D13/SCK"],
                    ["GND#2", "AREF", "D18/SDA", "D19/SCL"],
                    ["MB_PWM", "MB_INT", "MB_RX", "MB_TX", "MB_SCL", "MB_SDA", "MB5V", "MBGND1"]]
    idx = {p["key"]: i for i, p in enumerate(pins)}
    rows_l = sum(len(g) for g in left_groups) + len(left_groups) - 1
    rows_r = sum(len(g) for g in right_groups) + len(right_groups) - 1
    rows = max(rows_l, rows_r)
    W, H = 160, 10 * rows + 40
    BX0, BX1, BY0 = 30, 130, 20  # box (units = 0.01in)
    out = ['<?xml version="1.0" encoding="UTF-8"?>',
           '<svg xmlns="http://www.w3.org/2000/svg" width="%sin" height="%sin" viewBox="0 0 %d %d" version="1.2" baseProfile="tiny">'
           % (f(W / 100), f(H / 100), W, H),
           '<g id="schematic">',
           '<rect x="%d" y="%d" width="%d" height="%d" fill="#fffde8" stroke="#000" stroke-width="1"/>'
           % (BX0, BY0 - 10, BX1 - BX0, 10 * rows + 20),
           '<text x="%d" y="%d" font-size="8" font-family="Helvetica,Arial,sans-serif" font-weight="bold" '
           'text-anchor="middle">FRDM-MCXA153</text>' % ((BX0 + BX1) // 2, BY0 - 1)]

    def place(groups, left):
        y = BY0 + 5
        for gi, g in enumerate(groups):
            for key in g:
                i = idx[key]
                name = pins[i]["name"] if pins[i]["kind"] == "mb" else pins[i]["label"]
                if left:
                    out.append('<line id="connector%dpin" x1="%d" y1="%d" x2="%d" y2="%d" stroke="#000" stroke-width="1"/>'
                               % (i, BX0 - 20, y, BX0, y))
                    out.append('<rect id="connector%dterminal" x="%d" y="%s" width="0.5" height="0.5" fill="none"/>'
                               % (i, BX0 - 20, f(y - 0.25)))
                    out.append('<text x="%d" y="%d" font-size="6" font-family="Helvetica,Arial,sans-serif">%s</text>'
                               % (BX0 + 3, y + 2, escape(name)))
                else:
                    out.append('<line id="connector%dpin" x1="%d" y1="%d" x2="%d" y2="%d" stroke="#000" stroke-width="1"/>'
                               % (i, BX1, y, BX1 + 20, y))
                    out.append('<rect id="connector%dterminal" x="%d" y="%s" width="0.5" height="0.5" fill="none"/>'
                               % (i, BX1 + 19.5, f(y - 0.25)))
                    out.append('<text x="%d" y="%d" font-size="6" font-family="Helvetica,Arial,sans-serif" '
                               'text-anchor="end">%s</text>' % (BX1 - 3, y + 2, escape(name)))
                y += 10
            y += 10
    place(left_groups, True)
    place(right_groups, False)
    out.append('</g></svg>')
    return "\n".join(out)


# ----------------------------------------------------------------------- pcb
def pcb():
    out = ['<?xml version="1.0" encoding="UTF-8"?>',
           '<svg xmlns="http://www.w3.org/2000/svg" width="%smm" height="%smm" viewBox="0 0 %s %s" version="1.2" baseProfile="tiny">'
           % (f(BOARD_W), f(BOARD_H), f(BOARD_W), f(BOARD_H)),
           '<g id="silkscreen">',
           '<rect x="0.15" y="0.15" width="%s" height="%s" fill="none" stroke="#fff" stroke-width="0.3"/>'
           % (f(BOARD_W - 0.3), f(BOARD_H - 0.3)),
           '<text x="26.67" y="14" font-size="3" font-family="Helvetica,Arial,sans-serif" fill="#fff" '
           'text-anchor="middle">FRDM-MCXA153</text>',
           '</g>',
           '<g id="copper1"><g id="copper0">']
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
def fzp():
    o = ['<?xml version="1.0" encoding="UTF-8"?>',
         '<module fritzingVersion="0.9.3b" moduleId="%s">' % MODULE_ID,
         '  <version>1</version>',
         '  <author>mcx-arduino-core</author>',
         '  <title>FRDM-MCXA153</title>',
         '  <label>FRDM-MCXA153</label>',
         '  <date>2026-10-20</date>',
         '  <tags><tag>NXP</tag><tag>MCX</tag><tag>MCXA153</tag><tag>FRDM</tag><tag>Arduino</tag></tags>',
         '  <properties>',
         '    <property name="family">microcontroller board (arduino)</property>',
         '    <property name="type">NXP FRDM-MCXA153</property>',
         '    <property name="part number">FRDM-MCXA153</property>',
         '  </properties>',
         '  <description>NXP FRDM-MCXA153 (Arm Cortex-M33) development board. Arduino-compatible sockets J1-J4 '
         '(D0-D13, D18/D19, A0-A5, power), PWM0-PWM5 on the inner column of J3 and the mikroBUS sockets J5/J6. '
         'Pin names follow the mcx-arduino-core board package.</description>',
         '  <views>',
         '    <iconView><layers image="icon/%s_icon.svg"><layer layerId="icon"/></layers></iconView>' % BASE,
         '    <breadboardView><layers image="breadboard/%s_breadboard.svg"><layer layerId="breadboardbreadboard"/></layers></breadboardView>' % BASE,
         '    <schematicView><layers image="schematic/%s_schematic.svg"><layer layerId="schematic"/></layers></schematicView>' % BASE,
         '    <pcbView><layers image="pcb/%s_pcb.svg"><layer layerId="copper1"/><layer layerId="copper0"/>'
         '<layer layerId="silkscreen"/></layers></pcbView>' % BASE,
         '  </views>',
         '  <connectors>']
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
    byname = {}
    for i, p in enumerate(pins):
        byname.setdefault(p["name"], []).append(i)
    groups = [("gnd", [i for i, p in enumerate(pins) if p["kind"] == "gnd"]),
              ("3v3", byname["3V3"] + byname["MB_3V3"]), ("5v", byname["5V"] + byname["MB_5V"])]
    for a, b in [("A5", "MB_AN"), ("D7", "MB_RST"), ("D5", "MB_PWM"), ("D4", "MB_INT"),
                 ("D9", "MB_RX"), ("D8", "MB_TX")]:
        groups.append((b.lower(), byname[a] + byname[b]))
    o.append('  <buses>')
    for bid, ids in groups:
        o.append('    <bus id="%s">' % bid + "".join('<nodeMember connectorId="connector%d"/>' % i for i in ids) + '</bus>')
    o.append('  </buses>')
    o.append('</module>')
    return "\n".join(o)


def main():
    files = {
        FZP_NAME: fzp(),
        "svg.icon.%s_icon.svg" % BASE: icon(),
        "svg.breadboard.%s_breadboard.svg" % BASE: breadboard(),
        "svg.schematic.%s_schematic.svg" % BASE: schematic(),
        "svg.pcb.%s_pcb.svg" % BASE: pcb(),
    }
    os.makedirs(OUT, exist_ok=True)
    for n, c in files.items():
        with open(os.path.join(OUT, n), "w", encoding="utf-8") as fh:
            fh.write(c)
    z = os.path.join(os.path.dirname(OUT), "%s.fzpz" % BASE)
    with zipfile.ZipFile(z, "w", zipfile.ZIP_DEFLATED) as zf:
        for n in files:
            zf.write(os.path.join(OUT, n), n)
    print("wrote", z, "with", len(pins), "connectors")


if __name__ == "__main__":
    main()
