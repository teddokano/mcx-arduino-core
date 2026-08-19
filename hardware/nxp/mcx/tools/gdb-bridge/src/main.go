// gdb-bridge is invoked by gdb as a piped subprocess (gdb's
// `target extended-remote | <command>`), because arduino-cli's debug
// support only knows how to drive something it calls "openocd" -- the path
// to that executable is configurable, but the tool it names is not. This
// program masquerades as that "openocd", but really just launches NXP
// LinkServer's own gdbserver (which has correct, vendor-maintained support
// for the MCX chips this core targets, unlike upstream OpenOCD) and relays
// gdb's stdin/stdout to LinkServer's TCP gdb port.
//
// Usage: gdb-bridge <LinkServer DEVICE> [ignored args...]
//
// The trailing args are whatever arduino-cli's openocd-flavored command
// line construction appends (`-c "gdb_port pipe" -c "telnet_port 0"`,
// verified via a real gdb invocation) -- meaningless to LinkServer, so they
// are accepted and ignored rather than parsed.
package main

import (
	"fmt"
	"io"
	"net"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"runtime"
	"sort"
	"strconv"
	"time"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintln(os.Stderr, "gdb-bridge: usage: gdb-bridge <LinkServer DEVICE> [ignored args...]")
		os.Exit(1)
	}
	device := os.Args[1]

	linkserver, err := findLinkServer()
	if err != nil {
		fmt.Fprintln(os.Stderr, "gdb-bridge: LinkServer not found. Install it from https://www.nxp.com/linkserver")
		os.Exit(1)
	}

	port, listener, err := reserveFreePort()
	if err != nil {
		fmt.Fprintln(os.Stderr, "gdb-bridge: could not reserve a local TCP port:", err)
		os.Exit(1)
	}
	listener.Close() // release it for LinkServer to bind -- see reserveFreePort's doc comment

	cmd := exec.Command(linkserver, "gdbserver", device,
		"--gdb-port", strconv.Itoa(port),
		"--semihost-port", "-1", // this core's I/O goes over Serial, not semihosting
	)
	// LinkServer's own stdout/stderr must never touch our stdout: gdb treats
	// every byte there as part of the GDB remote protocol stream. Send both
	// to our stderr instead, so they still surface in the IDE's debug
	// console for troubleshooting.
	cmd.Stdout = os.Stderr
	cmd.Stderr = os.Stderr
	if err := cmd.Start(); err != nil {
		fmt.Fprintln(os.Stderr, "gdb-bridge: failed to start LinkServer:", err)
		os.Exit(1)
	}
	defer func() {
		if cmd.Process != nil {
			cmd.Process.Kill()
		}
		cmd.Wait()
	}()

	conn, err := waitForPort(port, 15*time.Second)
	if err != nil {
		fmt.Fprintln(os.Stderr, "gdb-bridge: LinkServer's gdbserver never came up:", err)
		os.Exit(1)
	}
	defer conn.Close()

	relay(conn)
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

func waitForPort(port int, timeout time.Duration) (net.Conn, error) {
	deadline := time.Now().Add(timeout)
	var lastErr error
	for time.Now().Before(deadline) {
		conn, err := net.DialTimeout("tcp", fmt.Sprintf("127.0.0.1:%d", port), 500*time.Millisecond)
		if err == nil {
			return conn, nil
		}
		lastErr = err
		time.Sleep(150 * time.Millisecond)
	}
	return nil, lastErr
}

// relay bridges this process's stdin/stdout (what gdb is piping to/from)
// with the TCP connection to LinkServer's gdbserver, in both directions,
// until either side closes.
func relay(conn net.Conn) {
	done := make(chan struct{}, 2)
	go func() {
		io.Copy(conn, os.Stdin)
		done <- struct{}{}
	}()
	go func() {
		io.Copy(os.Stdout, conn)
		done <- struct{}{}
	}()
	<-done
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
		if e.IsDir() && len(e.Name()) >= len(prefix) && e.Name()[:len(prefix)] == prefix {
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
