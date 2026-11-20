#!/usr/bin/env python3
"""The probe-rs tool entry of package_nxp_mcx_index.json, so that Boards
Manager installs probe-rs with the board package (Tools > Upload method >
probe-rs).

  probe_rs_tool.py <probe-rs version> <tag of this repository's release>
                   [--out <dir>]

Reads the probe-rs release's assets (name, size and SHA-256, from GitHub's
API through `gh`), and writes the tool's entry into the index (replacing an
entry of the same version, keeping the others):

- macOS and Linux: probe-rs's own archives, by their own URL. Each holds a
  single folder, as Boards Manager requires.
- Windows: probe-rs's zip has its files at the top, which arduino-cli
  refuses ("files in archive must be placed in a subdirectory"). It is
  downloaded, checked against its SHA-256, and packed again with them in a
  folder, into <dir> (the scratch folder by default), to be attached to
  this repository's release <tag>; the entry points there.

The platform entry of each release then lists the tool in its
toolsDependencies (release checklist). Nothing here runs at build time.
"""

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import urllib.request
import zipfile

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
INDEX = os.path.join(REPO, "package_nxp_mcx_index.json")
OWN_REPO = "teddokano/mcx-arduino-core"

# Boards Manager's host names, and probe-rs's target for each. i686-mingw32
# is the name the toolchain entry uses for Windows too (64-bit Windows
# takes it); probe-rs builds for 64-bit only
HOSTS = [
    ("arm64-apple-darwin", "aarch64-apple-darwin"),
    ("x86_64-apple-darwin", "x86_64-apple-darwin"),
    ("x86_64-linux-gnu", "x86_64-unknown-linux-gnu"),
    ("aarch64-linux-gnu", "aarch64-unknown-linux-gnu"),
    ("i686-mingw32", "x86_64-pc-windows-msvc"),
]


def assets(version):
    out = subprocess.run(
        ["gh", "api", f"repos/probe-rs/probe-rs/releases/tags/v{version}",
         "--jq", ".assets[] | [.name, .size, .digest, .browser_download_url] | @tsv"],
        check=True, capture_output=True, text=True).stdout
    found = {}
    for line in out.splitlines():
        name, size, digest, url = line.split("\t")
        found[name] = (int(size), digest.removeprefix("sha256:"), url)
    return found


def repack_windows(url, sha256, folder, out_path):
    """Download the Windows zip, check it, and pack its files into folder/."""
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "src.zip")
        urllib.request.urlretrieve(url, src)
        with open(src, "rb") as f:
            got = hashlib.sha256(f.read()).hexdigest()
        if got != sha256:
            sys.exit(f"{url}: SHA-256 {got}, the release says {sha256}")
        with zipfile.ZipFile(src) as zin, zipfile.ZipFile(out_path, "w", zipfile.ZIP_DEFLATED) as zout:
            for info in zin.infolist():
                data = zin.read(info)
                info.filename = f"{folder}/{info.filename}"
                zout.writestr(info, data)
    with open(out_path, "rb") as f:
        data = f.read()
    return len(data), hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("version", help="probe-rs's version, as 0.32.0")
    parser.add_argument("tag", help="this repository's release the Windows zip is attached to, as 0.13.0")
    parser.add_argument("--out", default=tempfile.gettempdir(), help="where to write the Windows zip")
    args = parser.parse_args()

    available = assets(args.version)
    systems = []
    for host, target in HOSTS:
        if target.endswith("windows-msvc"):
            name = f"probe-rs-tools-{target}.zip"
            size, sha256, url = available[name]
            own = f"probe-rs-tools-{args.version}-{target}.zip"
            out_path = os.path.join(args.out, own)
            size, sha256 = repack_windows(url, sha256, f"probe-rs-tools-{target}", out_path)
            url = f"https://github.com/{OWN_REPO}/releases/download/{args.tag}/{own}"
            print(f"attach {out_path} to release {args.tag} ({size} bytes)")
            name = own
        else:
            name = f"probe-rs-tools-{target}.tar.xz"
            size, sha256, url = available[name]
        systems.append({"host": host, "url": url, "archiveFileName": name,
                        "checksum": f"SHA-256:{sha256}", "size": str(size)})

    with open(INDEX, encoding="utf-8") as f:
        index = json.load(f)
    tools = index["packages"][0]["tools"]
    tools[:] = [t for t in tools if not (t["name"] == "probe-rs" and t["version"] == args.version)]
    tools.append({"name": "probe-rs", "version": args.version, "systems": systems})
    with open(INDEX, "w", encoding="utf-8", newline="\n") as f:
        json.dump(index, f, indent=2, ensure_ascii=False)   # no newline at the end, as the file has
    print(f"wrote probe-rs {args.version} into {os.path.relpath(INDEX, REPO)} ({len(systems)} hosts)")


if __name__ == "__main__":
    main()
