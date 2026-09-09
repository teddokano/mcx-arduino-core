#!/usr/bin/env python3
"""Static checks for the mistakes this project has actually made more than once.

Every check here exists because the same slip got through review at least
twice and was caught by a human late -- usually during release prep, once
by a compiler error hours after the fact. None of them need hardware or a
toolchain, so they run on every push.

Run with --release to additionally apply the checks that only make sense
once a version is being released (see check_changelog_heading).
"""

import argparse
import json
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

PLATFORM_TXT = os.path.join(REPO, "hardware/nxp/mcx/platform.txt")
DOXYFILE = os.path.join(REPO, "Doxyfile")
CHANGELOG = os.path.join(REPO, "CHANGELOG.md")
PACKAGE_INDEX = os.path.join(REPO, "package_nxp_mcx_index.json")
ARDUINO_IO_H = os.path.join(REPO, "hardware/nxp/mcx/cores/arduino/arduino_api/arduino_io.h")
PIN_STATE_CPP = os.path.join(REPO, "hardware/nxp/mcx/libraries/mcxPinState/src/PinState.cpp")

# Sources to scan for the comment-termination bug. Vendor SDK files are
# excluded: they are imported unmodified, so a finding there would be
# neither ours to fix nor a sign of a mistake made here.
SCAN_ROOTS = [
    "hardware/nxp/mcx/cores/arduino/r01lib",
    "hardware/nxp/mcx/cores/arduino/arduino_api",
    "hardware/nxp/mcx/libraries",
    "examples",
]
SCAN_EXTS = (".c", ".cpp", ".h", ".hpp", ".ino")

failures = []
notes = []


def fail(check, message):
    failures.append((check, message))


def read(path):
    with open(path, newline="") as f:
        return f.read()


def platform_version():
    """Return (version, major, minor, patch) as written in platform.txt."""
    text = read(PLATFORM_TXT)

    def field(name):
        m = re.search(r"^%s=(.+)$" % re.escape(name), text, re.MULTILINE)
        return m.group(1).strip() if m else None

    return field("version"), field("version.major"), field("version.minor"), field("version.patch")


def check_version_fields():
    """platform.txt's version= and its split major/minor/patch must agree.

    These are three separate lines maintained by hand because platform.txt
    has no way to split a version string, so they drift silently -- the
    macros built from them (MCX_ARDUINO_CORE_VERSION_*) would then report
    a different version than the package itself claims.
    """
    version, major, minor, patch = platform_version()
    if not version:
        fail("version-fields", "platform.txt has no version= line")
        return
    if None in (major, minor, patch):
        fail("version-fields", "platform.txt is missing one of version.major/minor/patch")
        return

    expected = "%s.%s.%s" % (major, minor, patch)
    if version != expected:
        fail(
            "version-fields",
            "platform.txt version=%s but version.major/minor/patch spell %s" % (version, expected),
        )


def check_doxyfile_version():
    """Doxyfile's PROJECT_NUMBER must match platform.txt's version.

    Bumped by hand at the start of each cycle and forgotten at least once
    (v0.4.1), where it shipped stale into the generated docs.
    """
    version = platform_version()[0]
    text = read(DOXYFILE)
    m = re.search(r'^PROJECT_NUMBER\s*=\s*"?([^"\r\n]*)"?\s*$', text, re.MULTILINE)
    if not m:
        fail("doxyfile-version", "Doxyfile has no PROJECT_NUMBER line")
        return
    found = m.group(1).strip()
    if found != version:
        fail(
            "doxyfile-version",
            'Doxyfile PROJECT_NUMBER is "%s" but platform.txt says %s' % (found, version),
        )


def changelog_top_version():
    """Return the newest ## heading in CHANGELOG.md: a version, or 'Unreleased'."""
    for line in read(CHANGELOG).splitlines():
        m = re.match(r"^##\s*\[([^\]]+)\]", line)
        if m:
            return m.group(1).strip()
    return None


def check_changelog_heading():
    """At release time CHANGELOG's newest heading must name the version being released.

    Only meaningful with --release: during development the newest heading
    is legitimately either [Unreleased] or the *previous* release (before
    any change has been recorded for the new cycle), so there is nothing
    to compare against.
    """
    version = platform_version()[0]
    top = changelog_top_version()
    if top is None:
        fail("changelog-heading", "CHANGELOG.md has no '## [...]' heading")
        return
    if top != version:
        fail(
            "changelog-heading",
            "CHANGELOG.md's newest heading is [%s] but platform.txt says %s -- "
            "confirm [Unreleased] to [%s] before releasing" % (top, version, version),
        )


def check_package_index_entry():
    """package_nxp_mcx_index.json must carry an entry for the version being released.

    update_package_index.yml only rewrites the checksum/size of an entry
    that already exists; it never creates one. Forgetting the placeholder
    made that workflow fail confusingly twice (v0.4.0, v0.4.1).
    """
    version = platform_version()[0]
    data = json.loads(read(PACKAGE_INDEX))
    versions = [p.get("version") for p in data["packages"][0]["platforms"]]
    if version not in versions:
        fail(
            "package-index-entry",
            "package_nxp_mcx_index.json has no platforms[] entry for %s "
            "(newest is %s) -- add a placeholder entry before running "
            "update_package_index.yml" % (version, versions[0] if versions else "none"),
        )


def scan_comment_terminators(path):
    """Yield (line_no, line) where a block comment ends mid-line on a continuation line.

    The bug this catches: writing something like `ARD_D*/ARD_A*` inside a
    /** ... */ block. The `*/` in the middle closes the comment early, so
    the rest of the prose becomes code and the build breaks somewhere that
    reads nothing like the actual mistake. Hit at least four separate times
    here, each time only found by a compiler error.

    A block comment that ends mid-line is legal and common on a *single*
    line (`/* n */ int n;`), so only continuation lines -- ones already
    inside a comment when the line started -- are flagged.
    """
    text = read(path)
    in_block = False
    for line_no, line in enumerate(text.splitlines(), 1):
        started_inside = in_block
        i = 0
        flagged = False
        while i < len(line):
            if in_block:
                end = line.find("*/", i)
                if end < 0:
                    break
                in_block = False
                i = end + 2
                # Only a comment we were already inside when the line began
                # can be the prose-interrupted-by-*/ case.
                if started_inside and line[i:].strip() and not flagged:
                    yield line_no, line
                    flagged = True
            else:
                nxt = min(
                    [p for p in (line.find("/*", i), line.find("//", i), line.find('"', i)) if p >= 0]
                    or [-1]
                )
                if nxt < 0:
                    break
                if line.startswith("//", nxt):
                    break
                if line.startswith("/*", nxt):
                    in_block = True
                    i = nxt + 2
                else:
                    # Step over a string literal so a "*/" inside one is
                    # not mistaken for comment structure.
                    i = nxt + 1
                    while i < len(line):
                        if line[i] == "\\":
                            i += 2
                            continue
                        if line[i] == '"':
                            i += 1
                            break
                        i += 1


def check_comment_terminators():
    hits = []
    for root in SCAN_ROOTS:
        base = os.path.join(REPO, root)
        for dirpath, _dirnames, filenames in os.walk(base):
            for name in sorted(filenames):
                if not name.endswith(SCAN_EXTS):
                    continue
                path = os.path.join(dirpath, name)
                for line_no, line in scan_comment_terminators(path):
                    hits.append((os.path.relpath(path, REPO), line_no, line.strip()))
    for rel, line_no, line in hits:
        fail("comment-terminator", "%s:%d ends a block comment mid-line: %s" % (rel, line_no, line))
    notes.append("comment-terminator: scanned for early '*/' in block comments")


def parse_pin_by_number():
    """Return the identifier sequence in arduino_io.h's arduino_pin_by_number[]."""
    text = read(ARDUINO_IO_H)
    m = re.search(r"arduino_pin_by_number\s*\[\s*\]\s*=\s*\{(.*?)\}\s*;", text, re.DOTALL)
    if not m:
        return None
    return re.findall(r"[A-Za-z_][A-Za-z0-9_]*", m.group(1))


def parse_alias_names():
    """Return the string sequence in PinState.cpp's ALIAS_NAMES[]."""
    text = read(PIN_STATE_CPP)
    m = re.search(r"ALIAS_NAMES\s*\[\s*\]\s*=\s*\{(.*?)\}\s*;", text, re.DOTALL)
    if not m:
        return None
    return re.findall(r'"([^"]*)"', m.group(1))


def check_mcxpinstate_aliases():
    """mcxPinState's ALIAS_NAMES[] must name arduino_pin_by_number[]'s entries, in order.

    PinState.cpp already static_asserts that the two have the same length,
    which catches an added or removed pin. It cannot catch a reordering or
    a renamed pin -- same count, wrong labels -- which until now was left
    to a manual diff at each release. That is what this closes.
    """
    pins = parse_pin_by_number()
    aliases = parse_alias_names()
    if pins is None:
        fail("mcxpinstate-aliases", "could not find arduino_pin_by_number[] in arduino_io.h")
        return
    if aliases is None:
        fail("mcxpinstate-aliases", "could not find ALIAS_NAMES[] in PinState.cpp")
        return

    if len(pins) != len(aliases):
        fail(
            "mcxpinstate-aliases",
            "arduino_pin_by_number[] has %d entries but ALIAS_NAMES[] has %d"
            % (len(pins), len(aliases)),
        )
        return

    for idx, (pin, alias) in enumerate(zip(pins, aliases)):
        if pin != alias:
            fail(
                "mcxpinstate-aliases",
                "entry %d: arduino_pin_by_number[] has %s but ALIAS_NAMES[] has \"%s\" -- "
                "mcxPinState would label that pin wrongly" % (idx, pin, alias),
            )
    notes.append("mcxpinstate-aliases: %d pin names match in order" % len(pins))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--release",
        action="store_true",
        help="also run the checks that only apply to a version being released",
    )
    args = parser.parse_args()

    check_version_fields()
    check_doxyfile_version()
    check_comment_terminators()
    check_mcxpinstate_aliases()

    if args.release:
        check_changelog_heading()
        check_package_index_entry()

    for note in notes:
        print("ok: %s" % note)

    if failures:
        print()
        for check, message in failures:
            print("FAIL [%s] %s" % (check, message))
        print()
        print("%d check(s) failed" % len(failures))
        return 1

    print("ok: all repo hygiene checks passed%s" % ("" if args.release else " (non-release set)"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
