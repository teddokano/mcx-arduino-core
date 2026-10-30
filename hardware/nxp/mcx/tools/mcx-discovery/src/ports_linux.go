package main

import (
	"os"
	"path/filepath"
	"strconv"
	"strings"
)

// findProbePorts reads sysfs. /sys/class/tty/ttyACM0/device is the probe's
// USB interface; the USB device, with idVendor, idProduct, serial and
// product, is the nearest directory above it that has idVendor.
func findProbePorts() []probePort {
	ttys, _ := filepath.Glob("/sys/class/tty/*/device")
	var found []probePort
	for _, t := range ttys {
		dir, err := filepath.EvalSymlinks(t)
		if err != nil {
			continue
		}
		for ; dir != "/" && dir != "."; dir = filepath.Dir(dir) {
			if _, err := os.Stat(filepath.Join(dir, "idVendor")); err == nil {
				break
			}
		}
		if dir == "/" || dir == "." {
			continue
		}
		if hexFile(dir, "idVendor") != mculinkVID || hexFile(dir, "idProduct") != mculinkPID {
			continue
		}
		found = append(found, probePort{
			address: "/dev/" + filepath.Base(filepath.Dir(t)),
			serial:  textFile(dir, "serial"),
			product: textFile(dir, "product"),
		})
	}
	return found
}

func textFile(dir, name string) string {
	b, _ := os.ReadFile(filepath.Join(dir, name))
	return strings.TrimSpace(string(b))
}

func hexFile(dir, name string) int64 {
	n, _ := strconv.ParseInt(textFile(dir, name), 16, 64)
	return n
}
