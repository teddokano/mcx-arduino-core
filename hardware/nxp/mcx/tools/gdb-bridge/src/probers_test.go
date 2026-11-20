package main

import (
	"os"
	"path/filepath"
	"testing"
)

// From `probe-rs list` (0.32.0) with an FRDM-MCXA153 connected, plus made-up
// lines in the same form for the other boards
const rsListing = `The following debug probes were found:
[0]: MCU-LINK FRDM-MCXA153 (r0E7) CMSIS-DAP V3.128 -- 1fc9:0143-0:VZPF1KW3WACLY (CMSIS-DAP)
[1]: MCU-LINK FRDM-MCXN947 (r0E7) CMSIS-DAP V3.128 -- 1fc9:0143-0:ABCDEFGHIJKLM (CMSIS-DAP)
[2]: MCU-LINK on-board (r0E7) CMSIS-DAP V3.128 -- 1fc9:0143-0:NOPQRSTUVWXYZ (CMSIS-DAP)
`

func TestParseRsProbes(t *testing.T) {
	probes := parseRsProbes(rsListing)
	if len(probes) != 3 {
		t.Fatalf("got %d probes, want 3: %+v", len(probes), probes)
	}
	if probes[0].name != "MCU-LINK FRDM-MCXA153 (r0E7) CMSIS-DAP V3.128" || probes[0].selector != "1fc9:0143-0:VZPF1KW3WACLY" {
		t.Errorf("first probe: %+v", probes[0])
	}
	if got := parseRsProbes("No debug probes were found.\n"); len(got) != 0 {
		t.Errorf("no probes: got %+v", got)
	}
}

func TestPickRsProbe(t *testing.T) {
	all := parseRsProbes(rsListing)
	cases := []struct {
		probes  []rsProbe
		board   string
		want    string
		wantErr bool
	}{
		{all[:1], "FRDM-MCXA153", "", false}, // one probe: none to pass
		{nil, "FRDM-MCXA153", "", false},     // none: probe-rs says so itself
		{all, "FRDM-MCXA153", "1fc9:0143-0:VZPF1KW3WACLY", false},
		{all, "FRDM-MCXN947", "1fc9:0143-0:ABCDEFGHIJKLM", false},
		{all, "FRDM-MCXA156", "1fc9:0143-0:NOPQRSTUVWXYZ", false}, // the one naming no board
		{all[:2], "FRDM-MCXA156", "", true},                       // not there, and every probe names a board
		{[]rsProbe{all[0], all[0]}, "FRDM-MCXA153", "", true},     // two of the same board
	}
	for i, c := range cases {
		got, err := pickRsProbe(c.probes, c.board)
		if (err != nil) != c.wantErr || got != c.want {
			t.Errorf("case %d: got %q, %v; want %q, error %v", i, got, err, c.want, c.wantErr)
		}
	}
}

// Boards Manager's layout: the tool beside the platform, under the packager
func TestBundledProbeRs(t *testing.T) {
	root := t.TempDir()
	bridge := filepath.Join(root, "nxp", "hardware", "mcx", "0.13.0", "tools", "gdb-bridge")
	if err := os.MkdirAll(bridge, 0o755); err != nil {
		t.Fatal(err)
	}
	for _, v := range []string{"0.31.0", "0.32.0"} {
		dir := filepath.Join(root, "nxp", "tools", "probe-rs", v)
		if err := os.MkdirAll(dir, 0o755); err != nil {
			t.Fatal(err)
		}
		if err := os.WriteFile(filepath.Join(dir, "probe-rs"), nil, 0o755); err != nil {
			t.Fatal(err)
		}
	}
	t.Setenv("MCX_GDB_BRIDGE_DIR", bridge)
	got := bundledProbeRs("probe-rs")
	want := filepath.Join(bridge, "..", "..", "..", "..", "..", "tools", "probe-rs", "0.32.0", "probe-rs")
	if len(got) == 0 || filepath.Clean(got[0]) != filepath.Clean(want) {
		t.Errorf("got %v, want %s first", got, want)
	}
}
