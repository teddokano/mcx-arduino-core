# Generates the Fritzing part for a W5500 Lite module: the .fzp, the four SVGs
# and the .fzpz. Dimensions are estimated from a photo (see README.md).
import os, sys, zipfile

OUT = sys.argv[1]
NAME = "W5500_Lite"
MODULE_ID = "W5500_Lite_Ethernet_module_mcx_arduino_core_v1"

P = 2.54                          # header pitch, mm
BW, BH = 25.0, 23.0               # PCB, mm (estimated)
X0 = 6.2                          # first pin from the PCB's left edge
Y_TOP, Y_BOT = 1.3, 1.3 + 8 * P   # the two header rows
JACK = (1.8, 1.8, 28.0, 18.0)     # x, y, w, h of the RJ45 jack; it overhangs the right edge
W = JACK[0] + JACK[2] + 0.5       # the drawing's width, with the overhang

# top view, jack to the right; (label, description, x index, row)
PINS = [
    ("INT",  "Interrupt output (INTn), active low",          0, "top"),
    ("CS",   "SPI chip select (SCSn), active low",           1, "top"),
    ("SCLK", "SPI clock",                                    2, "top"),
    ("MOSI", "SPI data in (MO)",                             3, "top"),
    ("GND",  "Ground",                                       4, "top"),
    ("GND",  "Ground",                                       5, "top"),
    ("MISO", "SPI data out (MI)",                            0, "bot"),
    ("RST",  "Reset (RSTn), active low",                     1, "bot"),
    ("NC",   "Not connected",                                2, "bot"),
    ("VIN",  "3.3 V supply",                                 3, "bot"),
    ("VIN",  "3.3 V supply",                                 4, "bot"),
    ("GND",  "Ground",                                       5, "bot"),
]

def pin_xy(i):
    _, _, k, row = PINS[i]
    return X0 + k * P, (Y_TOP if row == "top" else Y_BOT)

def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", newline="\n") as f:
        f.write(text)

# ---- breadboard (also the icon) ----
def breadboard(icon=False):
    s = [f'<?xml version="1.0" encoding="UTF-8"?>',
         f'<svg xmlns="http://www.w3.org/2000/svg" width="{W:.2f}mm" height="{BH:.2f}mm" viewBox="0 0 {W:.2f} {BH:.2f}">',
         f'<g id="{"icon" if icon else "breadboard"}">',
         f'<rect x="0" y="0" width="{BW}" height="{BH}" rx="0.6" fill="#1f7a3a" stroke="#14532a" stroke-width="0.2"/>']
    for i in range(len(PINS)):
        x, y = pin_xy(i)
        pid = f'connector{i}pin'
        s.append(f'<rect id="{pid}" x="{x-0.8:.3f}" y="{y-0.8:.3f}" width="1.6" height="1.6" fill="#d9b44a" stroke="#8a6d1d" stroke-width="0.15"/>')
        s.append(f'<rect x="{x-0.32:.3f}" y="{y-0.32:.3f}" width="0.64" height="0.64" fill="#7a5c12"/>')
    jx, jy, jw, jh = JACK
    s.append(f'<rect x="{jx}" y="{jy+1.0}" width="{jw}" height="{jh-2.0}" rx="0.4" fill="#c9ccd1" stroke="#6b6f76" stroke-width="0.25"/>')
    s.append(f'<rect x="{jx+jw-8.0}" y="{jy+3.0}" width="6.5" height="2.2" fill="#3a3d42"/>')
    s.append(f'<rect x="{jx+jw-8.0}" y="{jy+jh-5.2}" width="6.5" height="2.2" fill="#3a3d42"/>')
    if not icon:
        s.append(f'<text x="{jx+jw-11.5:.2f}" y="{jy+jh/2+0.8:.2f}" font-family="Droid Sans, sans-serif" font-size="2.0" fill="#55595f" text-anchor="middle">W5500 Lite</text>')
        # vertical, on the jack, so that six labels fit in the 2.54 mm pitch
        for i, (lab, _, k, row) in enumerate(PINS):
            x, y = pin_xy(i)
            if row == "top":
                tx, ty, rot, anchor = x + 0.4, y + 1.4, 90, "start"
            else:
                tx, ty, rot, anchor = x + 0.4, y - 1.4, -90, "start"
            s.append(f'<text x="{tx:.3f}" y="{ty:.3f}" transform="rotate({rot} {tx:.3f} {ty:.3f})" font-family="Droid Sans, sans-serif" font-size="1.2" fill="#22262b" text-anchor="{anchor}">{lab}</text>')
    s += ['</g>', '</svg>', '']
    return "\n".join(s)

# ---- schematic: a box, power on the left, signals on the right, 0.1 in grid ----
def schematic():
    G = 0.1 * 25.4                  # 0.1 in, mm
    left = [9, 10, 4, 5, 11, 8]     # VIN VIN GND GND GND NC
    right = [3, 6, 2, 1, 7, 0]      # MOSI MISO SCLK CS RST INT
    rows = max(len(left), len(right))
    bw, bh = 8 * G, (rows + 1) * G
    pl = 3 * G                      # pin length
    W2, H2 = bw + 2 * pl, bh + 2 * G
    bx, by = pl, G
    s = [f'<?xml version="1.0" encoding="UTF-8"?>',
         f'<svg xmlns="http://www.w3.org/2000/svg" width="{W2/25.4:.4f}in" height="{H2/25.4:.4f}in" viewBox="0 0 {W2:.3f} {H2:.3f}">',
         '<g id="schematic">',
         f'<rect x="{bx:.3f}" y="{by:.3f}" width="{bw:.3f}" height="{bh:.3f}" fill="#ffffff" stroke="#000000" stroke-width="0.25"/>',
         f'<text x="{W2/2:.3f}" y="{by-0.6:.3f}" font-family="Droid Sans, sans-serif" font-size="1.8" text-anchor="middle">W5500 Lite</text>']
    for side, lst in (("L", left), ("R", right)):
        for r, i in enumerate(lst):
            y = by + (r + 1) * G
            lab = PINS[i][0]
            if side == "L":
                x1, x2, tx, anchor = 0, bx, bx + 0.8, "start"
                tx_end = x1
            else:
                x1, x2, tx, anchor = bx + bw, W2, bx + bw - 0.8, "end"
                tx_end = x2
            s.append(f'<line id="connector{i}pin" x1="{x1:.3f}" y1="{y:.3f}" x2="{x2:.3f}" y2="{y:.3f}" stroke="#000000" stroke-width="0.25"/>')
            s.append(f'<rect id="connector{i}terminal" x="{tx_end-0.05:.3f}" y="{y-0.05:.3f}" width="0.1" height="0.1" fill="none" stroke="none"/>')
            s.append(f'<text x="{tx:.3f}" y="{y+0.6:.3f}" font-family="Droid Sans, sans-serif" font-size="1.6" text-anchor="{anchor}">{lab}</text>')
    s += ['</g>', '</svg>', '']
    return "\n".join(s)

# ---- pcb: 12 through-hole pads and the outline on the silkscreen ----
def pcb():
    s = [f'<?xml version="1.0" encoding="UTF-8"?>',
         f'<svg xmlns="http://www.w3.org/2000/svg" width="{W:.2f}mm" height="{BH:.2f}mm" viewBox="0 0 {W:.2f} {BH:.2f}">',
         '<g id="silkscreen">',
         f'<rect x="0.1" y="0.1" width="{BW-0.2}" height="{BH-0.2}" fill="none" stroke="#000000" stroke-width="0.2"/>',
         f'<rect x="{JACK[0]}" y="{JACK[1]+1.0}" width="{JACK[2]}" height="{JACK[3]-2.0}" fill="none" stroke="#000000" stroke-width="0.15" stroke-dasharray="0.6,0.4"/>',
         '</g>']
    for layer in ("copper0", "copper1"):
        s.append(f'<g id="{layer}">')
        for i in range(len(PINS)):
            x, y = pin_xy(i)
            # Fritzing: hole radius = r - stroke/2 (0.5), pad radius = r + stroke/2 (0.9)
            s.append(f'<circle id="connector{i}pad" cx="{x:.3f}" cy="{y:.3f}" r="0.7" fill="none" stroke="#F7BD13" stroke-width="0.4"/>')
        s.append('</g>')
    s += ['</svg>', '']
    return "\n".join(s)

def fzp():
    c = []
    for i, (lab, desc, _, _) in enumerate(PINS):
        c.append(f'''  <connector id="connector{i}" name="{lab}" type="male">
   <description>{desc}</description>
   <views>
    <breadboardView><p layer="breadboard" svgId="connector{i}pin"/></breadboardView>
    <schematicView><p layer="schematic" svgId="connector{i}pin" terminalId="connector{i}terminal"/></schematicView>
    <pcbView><p layer="copper0" svgId="connector{i}pad"/><p layer="copper1" svgId="connector{i}pad"/></pcbView>
   </views>
  </connector>''')
    def bus(name, label):
        members = "".join(f'<nodeMember connectorId="connector{i}"/>' for i, p in enumerate(PINS) if p[0] == label)
        return f'  <bus id="{name}">{members}</bus>'
    return f'''<?xml version="1.0" encoding="UTF-8"?>
<module fritzingVersion="0.9.6" moduleId="{MODULE_ID}">
 <version>1</version>
 <author>mcx-arduino-core</author>
 <title>W5500 Lite Ethernet module</title>
 <label>ETH</label>
 <date>2026-10-25</date>
 <tags>
  <tag>W5500</tag>
  <tag>Ethernet</tag>
  <tag>WIZnet</tag>
  <tag>SPI</tag>
 </tags>
 <properties>
  <property name="family">Ethernet module</property>
  <property name="chip">W5500</property>
  <property name="variant">Lite (2x6 pins)</property>
  <property name="supply">3.3V</property>
 </properties>
 <description>A small WIZnet W5500 Ethernet module with an RJ45 jack and two rows of six 2.54 mm pins (GND, VIN 3.3V, MOSI, MISO, SCLK, CS, RST, INT, NC). For the official Arduino Ethernet library: CS to the board's SPI chip select (D10), RST to a free pin or left to the module, INT unused. The outline is estimated from a photo, not measured.</description>
 <views>
  <iconView><layers image="icon/{NAME}_icon.svg"><layer layerId="icon"/></layers></iconView>
  <breadboardView><layers image="breadboard/{NAME}_breadboard.svg"><layer layerId="breadboard"/></layers></breadboardView>
  <schematicView><layers image="schematic/{NAME}_schematic.svg"><layer layerId="schematic"/></layers></schematicView>
  <pcbView><layers image="pcb/{NAME}_pcb.svg"><layer layerId="copper0"/><layer layerId="silkscreen"/><layer layerId="copper1"/></layers></pcbView>
 </views>
 <connectors>
{chr(10).join(c)}
 </connectors>
 <buses>
{bus("GND", "GND")}
{bus("VIN", "VIN")}
 </buses>
</module>
'''

# all in svg/, with no folder per view: a folder named icon/ matches the
# macOS "Icon" pattern of a global gitignore (case-insensitive) and drops out
files = {
    f"{NAME}.fzp": fzp(),
    f"svg/{NAME}_breadboard.svg": breadboard(),
    f"svg/{NAME}_icon.svg": breadboard(icon=True),
    f"svg/{NAME}_schematic.svg": schematic(),
    f"svg/{NAME}_pcb.svg": pcb(),
}
for rel, text in files.items():
    write(os.path.join(OUT, rel), text)

# the .fzpz: a flat zip, the part as part.<name>.fzp and each svg as svg.<view>.<name>
with zipfile.ZipFile(os.path.join(OUT, f"{NAME}.fzpz"), "w", zipfile.ZIP_DEFLATED) as z:
    z.writestr(f"part.{NAME}.fzp", files[f"{NAME}.fzp"])
    for view in ("breadboard", "icon", "schematic", "pcb"):
        z.writestr(f"svg.{view}.{NAME}_{view}.svg", files[f"svg/{NAME}_{view}.svg"])
print("written to", OUT)
