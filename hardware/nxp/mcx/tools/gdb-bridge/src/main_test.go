package main

import (
	"os"
	"path/filepath"
	"runtime"
	"strings"
	"testing"
)

// The table `LinkServer probes` printed (LinkServer 26.6) with an
// FRDM-MCXA153 and an FRDM-MCXN947 plugged in.
const twoKinds = `  #  Description                                    Serial         Device    Board         Capabilities
---  ---------------------------------------------  -------------  --------  ------------  ----------------
  1  MCU-LINK FRDM-MCXA153 (r0E7) CMSIS-DAP V3.167  PZU5XXMWG442Y  MCXA153   FRDM-MCXA153  DEBUG, VCOM, SIO
  2  MCU-LINK FRDM-MCXN947 (r0E7) CMSIS-DAP V3.128  UENBVJCYVDM5J  MCXN947   FRDM-MCXN947  DEBUG, VCOM, SIO
`

// fakeLinkServer writes a stand-in for LinkServer that prints output for
// `probes`, and returns its path.
func fakeLinkServer(t *testing.T, output string) string {
	t.Helper()
	if runtime.GOOS == "windows" {
		t.Skip("the fake LinkServer is a shell script")
	}
	dir := t.TempDir()
	data := filepath.Join(dir, "probes.txt")
	if err := os.WriteFile(data, []byte(output), 0o644); err != nil {
		t.Fatal(err)
	}
	exe := filepath.Join(dir, "LinkServer")
	script := "#!/bin/sh\ncat '" + data + "'\n"
	if err := os.WriteFile(exe, []byte(script), 0o755); err != nil {
		t.Fatal(err)
	}
	return exe
}

func TestListProbes(t *testing.T) {
	got := listProbes(fakeLinkServer(t, twoKinds))
	want := []linkProbe{{"PZU5XXMWG442Y", "MCXA153"}, {"UENBVJCYVDM5J", "MCXN947"}}
	if len(got) != len(want) {
		t.Fatalf("got %v, want %v", got, want)
	}
	for i := range want {
		if got[i] != want[i] {
			t.Errorf("row %d: got %v, want %v", i, got[i], want[i])
		}
	}
}

func TestListProbesTableTwice(t *testing.T) {
	// Seen once when checking by hand: the table can come out on stdout and
	// stderr both, and CombinedOutput then has it twice.
	got := listProbes(fakeLinkServer(t, twoKinds+twoKinds))
	if len(got) != 2 {
		t.Fatalf("got %v, want the two probes once each", got)
	}
}

func TestProbeFor(t *testing.T) {
	twoSame := strings.Replace(twoKinds,
		"MCU-LINK FRDM-MCXN947 (r0E7) CMSIS-DAP V3.128  UENBVJCYVDM5J  MCXN947   FRDM-MCXN947",
		"MCU-LINK FRDM-MCXA153 (r0E7) CMSIS-DAP V3.167  ABCDEFGHIJKLM  MCXA153   FRDM-MCXA153", 1)
	unidentified := strings.Replace(twoKinds, "UENBVJCYVDM5J  MCXN947   FRDM-MCXN947",
		"UENBVJCYVDM5J                          ", 1)
	oneProbe := strings.Join(strings.Split(twoKinds, "\n")[:3], "\n") + "\n"

	cases := []struct {
		name, output, device, want string
		wantErr                    string
	}{
		{"A153 of two kinds", twoKinds, "MCXA153:FRDM-MCXA153", "PZU5XXMWG442Y", ""},
		{"N947 of two kinds", twoKinds, "MCXN947:FRDM-MCXN947", "UENBVJCYVDM5J", ""},
		{"one probe is left to LinkServer", oneProbe, "MCXN947:FRDM-MCXN947", "", ""},
		{"two of the same kind", twoSame, "MCXA153:FRDM-MCXA153", "", "2 boards with an MCXA153"},
		{"no probe on that chip", twoKinds, "MCXC444:FRDM-MCXC444", "", "none of them reports an MCXC444"},
		{"a probe with no target identified", unidentified, "MCXN947:FRDM-MCXN947", "", "none of them reports an MCXN947"},
		{"unreadable output is left to LinkServer", "Error: something else\n", "MCXA153:FRDM-MCXA153", "", ""},
	}
	for _, c := range cases {
		t.Run(c.name, func(t *testing.T) {
			got, err := probeFor(fakeLinkServer(t, c.output), c.device)
			if c.wantErr != "" {
				if err == nil || !strings.Contains(err.Error(), c.wantErr) {
					t.Fatalf("got error %v, want one containing %q", err, c.wantErr)
				}
				return
			}
			if err != nil {
				t.Fatalf("unexpected error: %v", err)
			}
			if got != c.want {
				t.Errorf("got %q, want %q", got, c.want)
			}
		})
	}
}
