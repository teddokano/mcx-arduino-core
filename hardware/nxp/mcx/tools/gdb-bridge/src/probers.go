// The probe-rs backend: Tools > Upload method > probe-rs points the board at
// a .cfg carrying a `gdb-bridge-probe-rs-chip:` line (a153-probe-rs.cfg and
// the like), and gdb-bridge then runs `probe-rs gdb` for that chip in place
// of LinkServer's gdbserver, relaying to it the same way. probe-rs takes
// what cortex-debug sends (`monitor reset halt`, `load`), checked on an
// FRDM-MCXA153.
package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"runtime"
	"sort"
	"strconv"
	"strings"
)

const (
	// probe-rs's name for the chip; its presence selects this backend
	probeRsChipDirective = "gdb-bridge-probe-rs-chip:"
	// the board's name as its on-board probe reports it ("FRDM-MCXA153"),
	// to pick the probe when more than one is connected
	boardDirective = "gdb-bridge-board:"
	// the newest probe-rs version that can't debug the board: gdb-bridge
	// stops with a message there in place of a session that ends at once
	probeRsDebugBrokenDirective = "gdb-bridge-probe-rs-debug-broken-up-to:"
	// what `probe-rs gdb` prints once it listens
	probeRsReadyLine = "Firing up GDB stub"
)

var probeRsVersionRe = regexp.MustCompile(`probe-rs (\d+(?:\.\d+)+)`)

// probeRsVersion returns probe-rs's version ("probe-rs 0.32.0 (git commit:
// 48f5e4d)" -> "0.32.0"), or "" if it can't be read.
func probeRsVersion(probeRs string) string {
	out, err := exec.Command(probeRs, "--version").CombinedOutput()
	if err != nil {
		return ""
	}
	if m := probeRsVersionRe.FindSubmatch(out); m != nil {
		return string(m[1])
	}
	return ""
}

// debugBroken says whether probe-rs version can't debug a board whose .cfg
// says it can't up to brokenUpTo. An unknown version, or no such line, is
// let through: probe-rs then shows what goes wrong itself.
func debugBroken(version, brokenUpTo string) bool {
	if version == "" || brokenUpTo == "" {
		return false
	}
	return !versionLess(brokenUpTo, version)
}

// scriptDirective returns the value of the first `directive` line in the
// scripts the invoking tool named (-f/--file), or "" if there is none.
func scriptDirective(args []string, directive string) string {
	var scriptsDir string
	var scripts []string
	for i := 0; i < len(args); i++ {
		switch args[i] {
		case "-s", "--search":
			if i+1 < len(args) {
				scriptsDir = args[i+1]
				i++
			}
		case "-f", "--file":
			if i+1 < len(args) {
				scripts = append(scripts, args[i+1])
				i++
			}
		}
	}
	for _, script := range scripts {
		for _, path := range scriptCandidates(script, scriptsDir) {
			data, err := os.ReadFile(path)
			if err != nil {
				continue
			}
			for _, line := range strings.Split(string(data), "\n") {
				if idx := strings.Index(line, directive); idx >= 0 {
					if v := strings.TrimSpace(line[idx+len(directive):]); v != "" {
						return v
					}
				}
			}
		}
	}
	return ""
}

// findProbeRs looks for the probe-rs executable: MCX_PROBE_RS if set, then
// the one that came with the package (bundledProbeRs()), then PATH, then
// where its own installer and Homebrew put it. The upload scripts look in
// the same order.
func findProbeRs() (string, error) {
	name := "probe-rs"
	if runtime.GOOS == "windows" {
		name = "probe-rs.exe"
	}
	if p := os.Getenv("MCX_PROBE_RS"); p != "" && isExecutable(p) {
		return p, nil
	}
	for _, p := range bundledProbeRs(name) {
		if isExecutable(p) {
			return p, nil
		}
	}
	if p, err := exec.LookPath(name); err == nil {
		return p, nil
	}
	var candidates []string
	if home, err := os.UserHomeDir(); err == nil {
		candidates = append(candidates, filepath.Join(home, ".cargo", "bin", name))
	}
	candidates = append(candidates, "/opt/homebrew/bin/"+name, "/usr/local/bin/"+name)
	for _, p := range candidates {
		if isExecutable(p) {
			return p, nil
		}
	}
	return "", fmt.Errorf("probe-rs not found")
}

// bundledProbeRs lists where Boards Manager puts the probe-rs tool of
// package_nxp_mcx_index.json, newest version first. This binary sits in
// <packages>/nxp/hardware/mcx/<version>/tools/gdb-bridge, and the tool in
// <packages>/nxp/tools/probe-rs/<version>/. The folder is taken from
// MCX_GDB_BRIDGE_DIR (launch.sh: the path as reached, through the
// development symlink to the repository if there is one) and from this
// executable's own path, as started and with symlinks resolved.
func bundledProbeRs(name string) []string {
	var dirs []string
	if d := os.Getenv("MCX_GDB_BRIDGE_DIR"); d != "" {
		dirs = append(dirs, d)
	}
	if exe, err := os.Executable(); err == nil {
		dirs = append(dirs, filepath.Dir(exe))
		if resolved, err := filepath.EvalSymlinks(exe); err == nil {
			dirs = append(dirs, filepath.Dir(resolved))
		}
	}
	var found []string
	for _, d := range dirs {
		packager := filepath.Join(d, "..", "..", "..", "..", "..")
		matches, _ := filepath.Glob(filepath.Join(packager, "tools", "probe-rs", "*", name))
		sort.Sort(sort.Reverse(sort.StringSlice(matches)))
		found = append(found, matches...)
	}
	return found
}

type rsProbe struct {
	name     string // "MCU-LINK FRDM-MCXA153 (r0E7) CMSIS-DAP V3.128"
	selector string // "1fc9:0143-0:VZPF1KW3WACLY", which --probe takes
}

// parseRsProbes reads `probe-rs list`:
//
//	[0]: MCU-LINK FRDM-MCXA153 (r0E7) CMSIS-DAP V3.128 -- 1fc9:0143-0:VZPF1KW3WACLY (CMSIS-DAP)
func parseRsProbes(out string) []rsProbe {
	var probes []rsProbe
	for _, line := range strings.Split(out, "\n") {
		line = strings.TrimSpace(line)
		if !strings.HasPrefix(line, "[") {
			continue
		}
		colon := strings.Index(line, "]: ")
		sep := strings.LastIndex(line, " -- ")
		if colon < 0 || sep < colon {
			continue
		}
		fields := strings.Fields(line[sep+4:])
		if len(fields) == 0 {
			continue
		}
		probes = append(probes, rsProbe{name: line[colon+3 : sep], selector: fields[0]})
	}
	return probes
}

// pickRsProbe chooses the probe for the board, as probeFor does for
// LinkServer: none to pass when there is at most one; else the one whose
// name has the board's; else, when exactly one names no FRDM board at all
// (the FRDM-MCXA156's on-board probe reports none), that one.
func pickRsProbe(probes []rsProbe, board string) (string, error) {
	if len(probes) <= 1 {
		return "", nil
	}
	var named, unnamed []rsProbe
	for _, p := range probes {
		if board != "" && strings.Contains(p.name, board) {
			named = append(named, p)
		}
		if !strings.Contains(p.name, "FRDM-") {
			unnamed = append(unnamed, p)
		}
	}
	switch {
	case len(named) == 1:
		return named[0].selector, nil
	case len(named) == 0 && len(unnamed) == 1:
		return unnamed[0].selector, nil
	}
	return "", fmt.Errorf("%d debug probes are connected and gdb-bridge can't tell which one is the %s: "+
		"Arduino IDE does not pass it the selected port. Leave only the board to debug connected", len(probes), board)
}

// startProbeRs runs `probe-rs gdb` for chip on 127.0.0.1:port and returns it
// with a channel that says whether it came up. Its output, both streams,
// goes on to stderr.
func startProbeRs(probeRs, chip, selector string, port int) (*exec.Cmd, <-chan bool, error) {
	// --reset-halt: see runProbeRs()
	args := []string{"gdb", "--chip", chip, "--reset-halt", "--non-interactive",
		"--gdb-connection-string", "127.0.0.1:" + strconv.Itoa(port)}
	if selector != "" {
		args = append(args, "--probe", selector)
	}
	cmd := exec.Command(probeRs, args...)
	// It has been seen to print the ready line on either stream, so both
	// are watched. The pipes close when the process ends (killProcess's
	// Wait), which ends the watchers
	stdout, err := cmd.StdoutPipe()
	if err != nil {
		return nil, nil, err
	}
	stderr, err := cmd.StderrPipe()
	if err != nil {
		return nil, nil, err
	}
	if err := cmd.Start(); err != nil {
		return nil, nil, err
	}
	return cmd, eitherReady(watchForReady(stdout, os.Stderr, probeRsReadyLine),
		watchForReady(stderr, os.Stderr, probeRsReadyLine)), nil
}

// eitherReady says true as soon as one of a and b does, and false once both
// have said false.
func eitherReady(a, b <-chan bool) <-chan bool {
	result := make(chan bool, 1)
	go func() {
		for pending := 2; pending > 0; pending-- {
			select {
			case ok := <-a:
				a = nil
				if ok {
					result <- true
					return
				}
			case ok := <-b:
				b = nil
				if ok {
					result <- true
					return
				}
			}
		}
		result <- false
	}()
	return result
}
