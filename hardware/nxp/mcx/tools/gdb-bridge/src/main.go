// gdb-bridge masquerades as "openocd" for the two different ways this
// project's tooling ends up launching it, because arduino-cli's debug
// support and Arduino IDE 2's bundled cortex-debug extension both only know
// how to drive something called "openocd" -- the path to that executable
// is configurable, but the tool it names is not. This program actually
// launches NXP LinkServer's own gdbserver (which has correct, vendor-
// maintained support for the MCX chips this core targets, unlike upstream
// OpenOCD, which has none) and relays gdb's traffic to it.
//
// Usage: gdb-bridge <LinkServer DEVICE> [openocd-style args...]
//
// The first argument is supplied by this project's own per-board launcher
// scripts (launch-a153.sh/.bat, launch-n947.sh/.bat), not by whichever tool
// invoked us. Everything after it comes from that tool and is real
// OpenOCD command-line syntax we don't implement -- almost all of it is
// ignored, except one detail that matters: cortex-debug (Arduino IDE 2's
// debug UI) picks its own GDB port and passes it as `-c "gdb_port N"`,
// then waits for a log line matching `Listening on port N for gdb
// connections` before it will connect. So in that mode we must bind
// exactly the port it chose and print a matching line -- see run() below.
// arduino-cli's own `debug` CLI command works differently: it pipes gdb
// directly to our stdin/stdout instead of dialing a port, which needs no
// such announcement (see relayPiped()).
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

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintln(os.Stderr, "gdb-bridge: usage: gdb-bridge <LinkServer DEVICE> [openocd-style args...]")
		os.Exit(1)
	}
	device := os.Args[1]
	toolArgs := os.Args[2:]

	linkserver, err := findLinkServer()
	if err != nil {
		fmt.Fprintln(os.Stderr, "gdb-bridge: LinkServer not found. Install it from https://www.nxp.com/linkserver")
		os.Exit(1)
	}

	lsPort, lsListener, err := reserveFreePort()
	if err != nil {
		fmt.Fprintln(os.Stderr, "gdb-bridge: could not reserve a local TCP port:", err)
		os.Exit(1)
	}
	lsListener.Close() // released for LinkServer to bind -- see reserveFreePort's doc comment

	cmd := exec.Command(linkserver, "gdbserver", device,
		"--gdb-port", strconv.Itoa(lsPort),
		"--semihost-port", "-1", // this core's I/O goes over Serial, not semihosting
	)
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
