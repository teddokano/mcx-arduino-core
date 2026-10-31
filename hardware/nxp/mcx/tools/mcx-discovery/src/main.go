// mcx-discovery tells Arduino IDE 2 and arduino-cli which FRDM board is on
// a serial port, so the IDE selects the board by itself.
//
// The on-board MCU-LINK probe of every FRDM board this core supports has
// the same USB VID/PID, 0x1FC9:0x0143, and the builtin serial-discovery
// reports nothing else but the probe's serial number. A port therefore
// matched all five boards in boards.txt, and the IDE asked which board it
// was. The probe's USB product string does carry the board's name, as in
// "MCU-LINK FRDM-MCXN947 (r0E7) CMSIS-DAP V3.128". This program reports the
// MCU-LINK serial ports with that name as the port property `board`
// ("FRDM-MCXN947"), and boards.txt matches it with upload_port.0.board.
//
// The probe's firmware takes the name from target strings stored in the
// probe. A probe with none reports "MCU-LINK on-board (r0E7) ...": its port
// gets no `board` property, matches no board, and the IDE asks, as before.
//
// Only MCU-LINK ports are reported here. platform.txt keeps the builtin
// serial-discovery for every port, so it reports these ports too, without
// `board` (it runs anyway if any other core is installed). Arduino IDE 2
// keeps one entry per port and replaces it with whichever "add" comes last,
// so whether the board's name showed depended on which discovery answered
// later (it showed on macOS, and not on Linux or Windows). arduino-cli also replays the "add" events of all discoveries in
// no set order when the IDE starts watching again. While START_SYNC is on,
// this program therefore sends "add" again for every port it reports every
// few seconds, so its entry wins within that time whatever the order. The
// IDE redraws nothing when the entry it has is the same.
//
// The protocol is Arduino's pluggable discovery, version 1: one command per
// line on stdin, one JSON object per reply on stdout. The serial ports are
// looked up every second while START_SYNC is on; how they are found is per
// OS (ports_darwin.go, ports_linux.go, ports_windows.go).
package main

import (
	"bufio"
	"encoding/json"
	"io"
	"os"
	"regexp"
	"strings"
	"sync"
	"time"
)

const (
	mculinkVID = 0x1FC9
	mculinkPID = 0x0143
)

// probePort is one serial port of an MCU-LINK, as the per-OS code finds it.
type probePort struct {
	address string // "/dev/cu.usbmodem...", "/dev/ttyACM0", "COM5"
	serial  string // the probe's USB serial number
	product string // the probe's USB product string
}

type port struct {
	Address       string            `json:"address"`
	Label         string            `json:"label,omitempty"`
	Protocol      string            `json:"protocol"`
	ProtocolLabel string            `json:"protocolLabel,omitempty"`
	Properties    map[string]string `json:"properties,omitempty"`
	HardwareID    string            `json:"hardwareId,omitempty"`
}

var boardInProduct = regexp.MustCompile(`^MCU-LINK (FRDM-\S+) \(`)

// boardName is the board's name in an MCU-LINK product string, or "".
func boardName(product string) string {
	if m := boardInProduct.FindStringSubmatch(product); m != nil {
		return m[1]
	}
	return ""
}

// toPort gives the port the same address, labels, properties and hardware
// ID the builtin serial-discovery gives it, plus `board`.
func toPort(p probePort) port {
	props := map[string]string{
		"vid":          "0x1FC9",
		"pid":          "0x0143",
		"serialNumber": p.serial,
	}
	if b := boardName(p.product); b != "" {
		props["board"] = b
	}
	return port{
		Address:       p.address,
		Label:         p.address,
		Protocol:      "serial",
		ProtocolLabel: "Serial Port (USB)",
		Properties:    props,
		HardwareID:    p.serial,
	}
}

func scan(find func() []probePort) map[string]port {
	ports := map[string]port{}
	for _, p := range find() {
		ports[p.address] = toPort(p)
	}
	return ports
}

type discovery struct {
	find     func() []probePort
	interval time.Duration
	resend   time.Duration // how often "add" is sent again for the ports already reported

	outMu sync.Mutex
	out   io.Writer

	syncMu sync.Mutex
	stop   chan struct{} // closed to end START_SYNC's polling; nil while not syncing
	done   chan struct{} // closed when that polling has ended
}

func (d *discovery) send(v any) {
	d.outMu.Lock()
	defer d.outMu.Unlock()
	b, _ := json.Marshal(v)
	d.out.Write(append(b, '\n'))
}

func (d *discovery) reply(event string) {
	d.send(map[string]any{"eventType": event, "message": "OK"})
}

func (d *discovery) startSync() {
	d.syncMu.Lock()
	defer d.syncMu.Unlock()
	if d.stop != nil {
		return
	}
	stop, done := make(chan struct{}), make(chan struct{})
	d.stop, d.done = stop, done
	go func() {
		defer close(done)
		known := map[string]port{}
		lastResend := time.Now()
		for {
			now := scan(d.find)
			for a, p := range known {
				if q, ok := now[a]; !ok || q.HardwareID != p.HardwareID {
					d.send(map[string]any{"eventType": "remove", "port": port{Address: a, Protocol: p.Protocol}})
					delete(known, a)
				}
			}
			resend := time.Since(lastResend) >= d.resend
			if resend {
				lastResend = time.Now()
			}
			for a, p := range now {
				if _, ok := known[a]; !ok || resend {
					d.send(map[string]any{"eventType": "add", "port": p})
				}
			}
			known = now
			select {
			case <-stop:
				return
			case <-time.After(d.interval):
			}
		}
	}()
}

func (d *discovery) stopSync() {
	d.syncMu.Lock()
	defer d.syncMu.Unlock()
	if d.stop != nil {
		close(d.stop)
		<-d.done
		d.stop, d.done = nil, nil
	}
}

// run serves commands from in until QUIT or the end of in.
func (d *discovery) run(in io.Reader) {
	lines := bufio.NewScanner(in)
	for lines.Scan() {
		f := strings.Fields(lines.Text())
		if len(f) == 0 {
			continue
		}
		switch cmd := strings.ToUpper(f[0]); cmd {
		case "HELLO":
			d.send(map[string]any{"eventType": "hello", "protocolVersion": 1, "message": "OK"})
		case "START":
			d.reply("start")
		case "STOP":
			d.stopSync()
			d.reply("stop")
		case "LIST":
			list := []port{}
			for _, p := range scan(d.find) {
				list = append(list, p)
			}
			d.send(map[string]any{"eventType": "list", "ports": list})
		case "START_SYNC":
			d.reply("start_sync")
			d.startSync()
		case "QUIT":
			d.stopSync()
			d.reply("quit")
			return
		default:
			d.send(map[string]any{"eventType": "command_error", "error": true,
				"message": "Command " + cmd + " not supported"})
		}
	}
	d.stopSync()
}

func main() {
	d := &discovery{find: findProbePorts, interval: time.Second, resend: 3 * time.Second, out: os.Stdout}
	d.run(os.Stdin)
}
