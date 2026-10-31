package main

import (
	"bytes"
	"encoding/json"
	"io"
	"strings"
	"sync"
	"testing"
	"time"
)

func TestBoardName(t *testing.T) {
	cases := map[string]string{
		"MCU-LINK FRDM-MCXA153 (r0E7) CMSIS-DAP V3.167": "FRDM-MCXA153",
		"MCU-LINK FRDM-MCXN947 (r0E7) CMSIS-DAP V3.128": "FRDM-MCXN947",
		"MCU-LINK on-board (r0E7) CMSIS-DAP V3.128":     "",
		"MCU-LINK (r0FF) CMSIS-DAP V3.174":              "",
		"":                                              "",
	}
	for product, want := range cases {
		if got := boardName(product); got != want {
			t.Errorf("boardName(%q) = %q, want %q", product, got, want)
		}
	}
}

func TestToPortWithoutBoard(t *testing.T) {
	p := toPort(probePort{address: "/dev/ttyACM0", serial: "S1", product: "MCU-LINK on-board (r0E7) CMSIS-DAP V3.128"})
	if _, ok := p.Properties["board"]; ok {
		t.Errorf("a probe with no board name got board=%q", p.Properties["board"])
	}
	if p.Properties["serialNumber"] != "S1" || p.HardwareID != "S1" || p.Protocol != "serial" {
		t.Errorf("unexpected port %+v", p)
	}
}

// syncWriter lets the test read what the polling goroutine writes.
type syncWriter struct {
	mu  sync.Mutex
	buf bytes.Buffer
}

func (w *syncWriter) Write(b []byte) (int, error) {
	w.mu.Lock()
	defer w.mu.Unlock()
	return w.buf.Write(b)
}

func (w *syncWriter) events(t *testing.T) []map[string]any {
	w.mu.Lock()
	defer w.mu.Unlock()
	var out []map[string]any
	dec := json.NewDecoder(bytes.NewReader(w.buf.Bytes()))
	for {
		var m map[string]any
		if err := dec.Decode(&m); err == io.EOF {
			return out
		} else if err != nil {
			t.Fatal(err)
		}
		out = append(out, m)
	}
}

func TestProtocol(t *testing.T) {
	var mu sync.Mutex
	present := []probePort{{address: "A", serial: "S1", product: "MCU-LINK FRDM-MCXC444 (r0E7) CMSIS-DAP V3.167"}}
	find := func() []probePort {
		mu.Lock()
		defer mu.Unlock()
		return append([]probePort(nil), present...)
	}
	w := &syncWriter{}
	d := &discovery{find: find, interval: 5 * time.Millisecond, out: w}

	inR, inW := io.Pipe()
	finished := make(chan struct{})
	go func() { d.run(inR); close(finished) }()

	io.WriteString(inW, "HELLO 1 \"test\"\nSTART\nLIST\nSTART_SYNC\n")
	time.Sleep(30 * time.Millisecond)
	mu.Lock()
	present = []probePort{{address: "B", serial: "S2", product: "MCU-LINK FRDM-MCXN236 (r0E7) CMSIS-DAP V3.130"}}
	mu.Unlock()
	time.Sleep(30 * time.Millisecond)
	io.WriteString(inW, "STOP\nFOO\nQUIT\n")
	<-finished

	var got []string
	for _, e := range w.events(t) {
		s := e["eventType"].(string)
		switch s {
		case "list":
			ports := e["ports"].([]any)
			s += ":" + ports[0].(map[string]any)["properties"].(map[string]any)["board"].(string)
		case "add", "remove":
			s += ":" + e["port"].(map[string]any)["address"].(string)
		}
		got = append(got, s)
	}
	want := "hello start list:FRDM-MCXC444 start_sync add:A remove:A add:B stop command_error quit"
	if strings.Join(got, " ") != want {
		t.Errorf("events\n got: %s\nwant: %s", strings.Join(got, " "), want)
	}
}
