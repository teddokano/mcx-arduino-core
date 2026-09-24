// gdb-bridge masquerades as "openocd" for the two different ways this
// project's tooling ends up launching it, because arduino-cli's debug
// support and Arduino IDE 2's bundled cortex-debug extension both only know
// how to drive something called "openocd" -- the path to that executable
// is configurable, but the tool it names is not. This program actually
// launches NXP LinkServer's own gdbserver (which has correct, vendor-
// maintained support for the MCX chips this core targets, unlike upstream
// OpenOCD, which has none) and relays gdb's traffic to it.
//
// Usage: gdb-bridge [<LinkServer DEVICE>] [openocd-style args...]
//
// Everything but the optional leading DEVICE comes from whichever tool
// invoked us, and is real OpenOCD command-line syntax we don't implement.
// Almost all of it is ignored, except two details that matter. (A third
// thing, which probe to use when several boards are plugged in, comes from
// LinkServer rather than from the arguments; see probeFor().)
//
// First, the board. Both launch paths pass the configured OpenOCD script
// -- cortex-debug as `-s <dir> -f <script>`, arduino-cli as `-s "<dir>"
// --file "<script>"` (verified in arduino-cli's own service_debug.go) --
// and boards.txt gives each board its own script file carrying a
// `gdb-bridge-device:` line. Reading the board out of there is what keeps
// this one binary board-agnostic: adding a board means a new .cfg and two
// boards.txt lines, with no new executable and no new launcher script.
// A DEVICE given as the first argument overrides it, which is only useful
// when running this by hand to diagnose something.
//
// Second, the port. cortex-debug (Arduino IDE 2's debug UI) picks its own
// GDB port and passes it as `-c "gdb_port N"`, then waits for a log line
// matching `Listening on port N for gdb connections` before it will
// connect. So in that mode we must bind exactly the port it chose and
// print a matching line -- see run() below. arduino-cli's own `debug` CLI
// command works differently: it pipes gdb directly to our stdin/stdout
// instead of dialing a port, which needs no such announcement (see
// relayPiped()).
package main

import (
	"bufio"
	"fmt"
	"io"
	"net"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"regexp"
	"runtime"
	"sort"
	"strconv"
	"strings"
	"syscall"
	"time"
)

// deviceDirective is the key gdb-bridge looks for in the OpenOCD script
// boards.txt points each board at. It sits behind a `#` so the file stays
// a valid (and inert) OpenOCD script, which matters only because Arduino
// IDE 2 refuses to start a debug session unless one is configured.
const deviceDirective = "gdb-bridge-device:"

// deviceFromScripts digs the LinkServer DEVICE string out of the OpenOCD
// script the invoking tool passed. Accepts both spellings of each flag:
// cortex-debug uses -s/-f, arduino-cli uses -s/--file.
func deviceFromScripts(args []string) (string, error) {
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
	if len(scripts) == 0 {
		return "", fmt.Errorf("no OpenOCD script argument (-f/--file) to read the board from")
	}
	for _, script := range scripts {
		for _, path := range scriptCandidates(script, scriptsDir) {
			data, err := os.ReadFile(path)
			if err != nil {
				continue
			}
			for _, line := range strings.Split(string(data), "\n") {
				if idx := strings.Index(line, deviceDirective); idx >= 0 {
					device := strings.TrimSpace(line[idx+len(deviceDirective):])
					if device != "" {
						return device, nil
					}
				}
			}
		}
	}
	return "", fmt.Errorf("no %q line in %s (searched -s %q and %s)",
		deviceDirective, strings.Join(scripts, ", "), scriptsDir, exeDir())
}

// exeDir is where this binary lives, which is also where the board .cfg
// files sit. Returns "" if it cannot be determined.
func exeDir() string {
	exe, err := os.Executable()
	if err != nil {
		return ""
	}
	if resolved, err := filepath.EvalSymlinks(exe); err == nil {
		exe = resolved
	}
	return filepath.Dir(exe)
}

// scriptCandidates lists where to look for an OpenOCD script named on the
// command line, in the order to try.
//
// Looking next to this binary is what makes the Arduino IDE work. Its
// cortex-debug points -s at the *sketch build* directory and prepends its
// own helper script, so the board .cfg that boards.txt names arrives as a
// bare filename that -s cannot resolve -- boards.txt's own scripts_dir is
// never passed through. The .cfg does sit beside this binary, and that is
// a layout this repository controls. arduino-cli, by contrast, passes our
// scripts_dir as -s, so that case resolves on the first candidate.
func scriptCandidates(script, scriptsDir string) []string {
	if filepath.IsAbs(script) {
		return []string{script}
	}
	var paths []string
	if scriptsDir != "" {
		paths = append(paths, filepath.Join(scriptsDir, script))
	}
	paths = append(paths, script)
	if dir := exeDir(); dir != "" {
		paths = append(paths, filepath.Join(dir, script))
	}
	return paths
}

func main() {
	// A leading argument that isn't a flag is an explicit DEVICE override,
	// for running this by hand. Otherwise the board comes from the script
	// the invoking tool named -- see the usage comment above.
	var device string
	toolArgs := os.Args[1:]
	if len(toolArgs) > 0 && !strings.HasPrefix(toolArgs[0], "-") {
		device = toolArgs[0]
		toolArgs = toolArgs[1:]
	} else {
		var err error
		device, err = deviceFromScripts(toolArgs)
		if err != nil {
			fmt.Fprintln(os.Stderr, "gdb-bridge: could not determine which board to debug:", err)
			fmt.Fprintln(os.Stderr, "gdb-bridge: usage: gdb-bridge [<LinkServer DEVICE>] [openocd-style args...]")
			os.Exit(1)
		}
	}

	linkserver, err := findLinkServer()
	if err != nil {
		fmt.Fprintln(os.Stderr, "gdb-bridge: LinkServer not found. Install it from https://www.nxp.com/linkserver")
		os.Exit(1)
	}

	probe, err := probeFor(linkserver, device)
	if err != nil {
		fmt.Fprintln(os.Stderr, "gdb-bridge:", err)
		os.Exit(1)
	}

	lsPort, lsListener, err := reserveFreePort()
	if err != nil {
		fmt.Fprintln(os.Stderr, "gdb-bridge: could not reserve a local TCP port:", err)
		os.Exit(1)
	}
	lsListener.Close() // released for LinkServer to bind -- see reserveFreePort's doc comment

	lsArgs := []string{"gdbserver"}
	if probe != "" {
		lsArgs = append(lsArgs, "--probe", probe)
	}
	lsArgs = append(lsArgs, device,
		"--gdb-port", strconv.Itoa(lsPort),
		"--semihost-port", "-1", // this core's I/O goes over Serial, not semihosting
	)
	cmd := exec.Command(linkserver, lsArgs...)
	// LinkServer's gdbserver ends its whole session -- not just the
	// connection -- the moment its first TCP client disconnects, so
	// readiness can't be checked by connect-then-close (that disconnect
	// alone was enough to make LinkServer shut down before any real client
	// got a chance, confirmed the hard way against real hardware). Watch
	// its own stdout for the ready line instead, and never close a
	// connection to it before the real client's connection is done.
	stdout, err := cmd.StdoutPipe()
	if err != nil {
		fmt.Fprintln(os.Stderr, "gdb-bridge: failed to pipe LinkServer's stdout:", err)
		os.Exit(1)
	}
	cmd.Stderr = os.Stderr
	if err := cmd.Start(); err != nil {
		fmt.Fprintln(os.Stderr, "gdb-bridge: failed to start LinkServer:", err)
		os.Exit(1)
	}
	defer killProcess(cmd)
	setupSignalCleanup(cmd)

	ready := watchForReady(stdout, os.Stderr)
	select {
	case ok := <-ready:
		if !ok {
			fmt.Fprintln(os.Stderr, "gdb-bridge: LinkServer's gdbserver exited before becoming ready")
			os.Exit(1)
		}
	case <-time.After(15 * time.Second):
		fmt.Fprintln(os.Stderr, "gdb-bridge: LinkServer's gdbserver never came up")
		os.Exit(1)
	}

	if idePort, ok := findGdbPortArg(toolArgs); ok {
		// Arduino IDE 2 / cortex-debug mode: it's waiting for us to listen
		// on idePort and announce it; each accepted client connection dials
		// LinkServer fresh.
		runServerMode(idePort, lsPort)
		return
	}

	// arduino-cli `debug` CLI mode: gdb is piping directly to our own
	// stdin/stdout. Dial LinkServer once and hold that single connection
	// for the whole session.
	lsConn, err := net.Dial("tcp", fmt.Sprintf("127.0.0.1:%d", lsPort))
	if err != nil {
		fmt.Fprintln(os.Stderr, "gdb-bridge: could not reach LinkServer's gdbserver:", err)
		os.Exit(1)
	}
	defer lsConn.Close()
	relay(os.Stdin, os.Stdout, lsConn)
}

// probeFor picks the debug probe to hand LinkServer when more than one is
// connected, since LinkServer then refuses to start without --probe.
//
// The upload scripts pick it from the selected port's USB serial number,
// but that is not available here: Arduino IDE 2 asks arduino-cli for the
// debug configuration without passing the port at all (vscode-arduino-
// tools' buildDebugInfoArgs() sends only the FQBN, programmer and sketch),
// so {debug.port} never expands on that path. What is known is the chip,
// from the board's DEVICE string, and LinkServer lists the chip each probe
// is wired to. That tells an FRDM-MCXA153 from an FRDM-MCXN947, not two
// boards of the same kind; for those, this says so instead of leaving
// LinkServer's own error, whose advice (pass --probe) no IDE user can take.
//
// Returns "" when there is at most one probe, which LinkServer picks by
// itself.
func probeFor(linkserver, device string) (string, error) {
	probes := listProbes(linkserver)
	if len(probes) < 2 {
		return "", nil
	}

	chip := device
	if i := strings.Index(device, ":"); i >= 0 {
		chip = device[:i]
	}
	var matches []string
	for _, p := range probes {
		if p.chip == chip {
			matches = append(matches, p.serial)
		}
	}

	switch len(matches) {
	case 1:
		fmt.Fprintf(os.Stderr, "gdb-bridge: %d probes connected; using %s, the one on the %s\n", len(probes), matches[0], chip)
		return matches[0], nil
	case 0:
		return "", fmt.Errorf("%d debug probes are connected, but none of them reports an %s on it. "+
			"Leave only the board to debug connected", len(probes), chip)
	default:
		return "", fmt.Errorf("%d boards with an %s are connected (probes %s), and the debugger cannot tell "+
			"which one is meant: Arduino IDE does not pass it the selected port. Leave only the board to debug connected",
			len(matches), chip, strings.Join(matches, ", "))
	}
}

type linkProbe struct {
	serial string
	chip   string
}

// listProbes runs `LinkServer probes` and reads each probe's serial number
// and the chip it reports from the table it prints. The table is fixed-
// width, so the columns are cut where the header's "Serial", "Device" and
// "Board" titles start. Output that isn't in that shape gives an empty list,
// which leaves the choice to LinkServer, as before this existed.
//
// LinkServer's output is taken from stdout and stderr together, since which
// one the table goes to wasn't consistent when checked, and rows are kept
// once per serial number in case it shows up on both.
func listProbes(linkserver string) []linkProbe {
	out, err := exec.Command(linkserver, "probes").CombinedOutput()
	if err != nil {
		return nil
	}

	serialCol, deviceCol, boardCol := -1, -1, -1
	seen := map[string]bool{}
	var probes []linkProbe
	for _, line := range strings.Split(string(out), "\n") {
		line = strings.TrimRight(line, "\r")
		s, d, b := strings.Index(line, "Serial"), strings.Index(line, "Device"), strings.Index(line, "Board")
		if s >= 0 && d > s && b > d {
			serialCol, deviceCol, boardCol = s, d, b
			continue
		}
		if serialCol < 0 || len(line) <= deviceCol || strings.HasPrefix(strings.TrimSpace(line), "---") {
			continue
		}
		serial := strings.TrimSpace(line[serialCol:deviceCol])
		chip := strings.TrimSpace(line[deviceCol:min(boardCol, len(line))])
		if serial == "" || seen[serial] {
			continue
		}
		seen[serial] = true
		probes = append(probes, linkProbe{serial, chip})
	}
	return probes
}

// watchForReady copies r's lines to echo (so they still surface in whatever
// debug console is watching our own output) and reports on the returned
// channel once a line containing "GDB server listening" is seen (LinkServer's
// own readiness message, confirmed against its real output), or false if r
// hit EOF first without ever seeing one.
func watchForReady(r io.Reader, echo io.Writer) <-chan bool {
	result := make(chan bool, 1)
	go func() {
		scanner := bufio.NewScanner(r)
		found := false
		for scanner.Scan() {
			line := scanner.Text()
			fmt.Fprintln(echo, line)
			if !found && strings.Contains(line, "GDB server listening") {
				found = true
				result <- true
			}
		}
		if !found {
			result <- false
		}
		// Keep draining after readiness so LinkServer's later log lines
		// still reach echo instead of blocking on a full pipe buffer.
	}()
	return result
}

// findGdbPortArg looks for `-c` followed by an argument like "gdb_port 3333"
// (cortex-debug's exact format, verified against the actual bundled
// extension) among the tool-supplied args, and returns the port number.
func findGdbPortArg(args []string) (int, bool) {
	re := regexp.MustCompile(`^gdb_port\s+(\d+)$`)
	for i, a := range args {
		if a == "-c" && i+1 < len(args) {
			if m := re.FindStringSubmatch(args[i+1]); m != nil {
				port, err := strconv.Atoi(m[1])
				if err == nil {
					return port, true
				}
			}
		}
	}
	return 0, false
}

// runServerMode binds ideGdbPort (the port cortex-debug already decided on
// and told gdb about), announces readiness the way it's watching for, then
// relays every client connection to a fresh connection to LinkServer's
// gdbserver, for as long as this process lives.
func runServerMode(ideGdbPort int, lsPort int) {
	l, err := net.Listen("tcp", fmt.Sprintf("127.0.0.1:%d", ideGdbPort))
	if err != nil {
		fmt.Fprintln(os.Stderr, "gdb-bridge: could not bind port", ideGdbPort, "for the debug UI:", err)
		os.Exit(1)
	}
	defer l.Close()

	// Matches cortex-debug's OpenOCDServerController.initMatch():
	// /Info\s:[^\n]*Listening on port \d+ for gdb connection/i
	fmt.Printf("Info : Listening on port %d for gdb connections\n", ideGdbPort)
	os.Stdout.Sync()

	for {
		client, err := l.Accept()
		if err != nil {
			return // listener closed, e.g. we're shutting down
		}
		go func() {
			defer client.Close()
			lsConn, err := net.DialTimeout("tcp", fmt.Sprintf("127.0.0.1:%d", lsPort), 5*time.Second)
			if err != nil {
				fmt.Fprintln(os.Stderr, "gdb-bridge: could not reach LinkServer's gdbserver:", err)
				return
			}
			defer lsConn.Close()
			relay(client, client, lsConn)
		}()
	}
}

// relay bridges in/out (what gdb reads from / writes to) with conn (the
// TCP connection to LinkServer's gdbserver) in both directions, until
// either side closes.
func relay(in io.Reader, out io.Writer, conn net.Conn) {
	done := make(chan struct{}, 2)
	go func() {
		io.Copy(conn, in)
		done <- struct{}{}
	}()
	go func() {
		io.Copy(out, conn)
		done <- struct{}{}
	}()
	<-done
}

func setupSignalCleanup(cmd *exec.Cmd) {
	if runtime.GOOS == "windows" {
		return // no POSIX signals to catch; best-effort only on Unix for now
	}
	sigs := make(chan os.Signal, 1)
	signal.Notify(sigs, syscall.SIGTERM, syscall.SIGINT)
	go func() {
		<-sigs
		killProcess(cmd)
		os.Exit(0)
	}()
}

func killProcess(cmd *exec.Cmd) {
	if cmd.Process != nil {
		cmd.Process.Kill()
	}
	cmd.Wait()
}

// reserveFreePort asks the OS for an unused TCP port by binding to port 0,
// then immediately releases it. There's an inherent (tiny, single-user,
// local-only) race between the release and LinkServer's own bind, but
// there's no portable way to hand Go's already-open listening socket
// directly to another process's --gdb-port flag, so this is the standard
// compromise.
func reserveFreePort() (int, net.Listener, error) {
	l, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		return 0, nil, err
	}
	return l.Addr().(*net.TCPAddr).Port, l, nil
}

// findLinkServer mirrors the discovery logic in tools/upload.sh and
// tools/upload.bat, so this binary finds the same install regardless of
// which one a given user has.
func findLinkServer() (string, error) {
	switch runtime.GOOS {
	case "darwin":
		if p := newestVersionedDir("/Applications", "LinkServer", "LinkServer"); p != "" {
			return p, nil
		}
	case "linux":
		fixed := "/usr/local/LinkServer/LinkServer"
		if isExecutable(fixed) {
			return fixed, nil
		}
		if p := newestVersionedDir("/usr/local", "LinkServer_", "LinkServer"); p != "" {
			return p, nil
		}
		if p, err := exec.LookPath("LinkServer"); err == nil {
			return p, nil
		}
	case "windows":
		if p := newestVersionedDir(`C:\NXP`, "LinkServer", "LinkServer.exe"); p != "" {
			return p, nil
		}
	}
	return "", fmt.Errorf("not found")
}

// newestVersionedDir finds the entry directly under dir whose name starts
// with prefix, picks the highest by version-aware comparison (so
// "LinkServer_9.0.0" doesn't win over "LinkServer_26.6.137" the way a plain
// lexicographic sort would), and returns the path to exeName inside it if
// that file exists and is executable.
func newestVersionedDir(dir, prefix, exeName string) string {
	entries, err := os.ReadDir(dir)
	if err != nil {
		return ""
	}
	var candidates []string
	for _, e := range entries {
		if e.IsDir() && strings.HasPrefix(e.Name(), prefix) {
			candidates = append(candidates, e.Name())
		}
	}
	if len(candidates) == 0 {
		return ""
	}
	sort.Slice(candidates, func(i, j int) bool {
		return versionLess(candidates[i], candidates[j])
	})
	best := candidates[len(candidates)-1]
	p := filepath.Join(dir, best, exeName)
	if isExecutable(p) {
		return p
	}
	return ""
}

var numRe = regexp.MustCompile(`\d+`)

// versionLess compares two strings by their embedded numeric runs (e.g.
// "LinkServer_26.6.137" -> [26, 6, 137]), falling back to a plain string
// comparison if either has none -- a lightweight stand-in for `sort -V`.
func versionLess(a, b string) bool {
	an, bn := numRe.FindAllString(a, -1), numRe.FindAllString(b, -1)
	if len(an) == 0 || len(bn) == 0 {
		return a < b
	}
	for i := 0; i < len(an) && i < len(bn); i++ {
		ai, _ := strconv.Atoi(an[i])
		bi, _ := strconv.Atoi(bn[i])
		if ai != bi {
			return ai < bi
		}
	}
	return len(an) < len(bn)
}

func isExecutable(path string) bool {
	fi, err := os.Stat(path)
	if err != nil || fi.IsDir() {
		return false
	}
	if runtime.GOOS == "windows" {
		return true // no exec bit on Windows; existence is enough
	}
	return fi.Mode()&0111 != 0
}
