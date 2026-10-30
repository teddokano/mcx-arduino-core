package main

import (
	"bytes"
	"encoding/xml"
	"os/exec"
	"strconv"
)

// findProbePorts reads the USB devices from `ioreg` as XML. A serial port
// (IOCalloutDevice) belongs to the nearest USB device above it in the tree,
// which is the probe and not a hub it hangs from.
func findProbePorts() []probePort {
	out, err := exec.Command("ioreg", "-a", "-r", "-c", "IOUSBHostDevice", "-l").Output()
	if err != nil {
		return nil
	}
	root, err := parsePlist(out)
	if err != nil {
		return nil
	}
	var found []probePort
	var walk func(v any, dev map[string]any)
	walk = func(v any, dev map[string]any) {
		switch n := v.(type) {
		case []any:
			for _, c := range n {
				walk(c, dev)
			}
		case map[string]any:
			if n["IOObjectClass"] == "IOUSBHostDevice" {
				dev = n
			}
			if a, ok := n["IOCalloutDevice"].(string); ok && dev != nil {
				vid, _ := dev["idVendor"].(int64)
				pid, _ := dev["idProduct"].(int64)
				if vid == mculinkVID && pid == mculinkPID {
					serial, _ := dev["kUSBSerialNumberString"].(string)
					product, _ := dev["kUSBProductString"].(string)
					found = append(found, probePort{address: a, serial: serial, product: product})
				}
			}
			if c, ok := n["IORegistryEntryChildren"]; ok {
				walk(c, dev)
			}
		}
	}
	walk(root, nil)
	return found
}

// parsePlist decodes an XML property list into maps, slices, strings,
// int64s and bools. Other value types come out as nil.
func parsePlist(b []byte) (any, error) {
	dec := xml.NewDecoder(bytes.NewReader(b))
	for {
		t, err := dec.Token()
		if err != nil {
			return nil, err
		}
		if se, ok := t.(xml.StartElement); ok && se.Name.Local != "plist" {
			return plistValue(dec, se)
		}
	}
}

func plistValue(dec *xml.Decoder, se xml.StartElement) (any, error) {
	switch se.Name.Local {
	case "dict":
		m := map[string]any{}
		key := ""
		for {
			t, err := dec.Token()
			if err != nil {
				return nil, err
			}
			switch e := t.(type) {
			case xml.StartElement:
				if e.Name.Local == "key" {
					var k string
					if err := dec.DecodeElement(&k, &e); err != nil {
						return nil, err
					}
					key = k
					continue
				}
				v, err := plistValue(dec, e)
				if err != nil {
					return nil, err
				}
				m[key] = v
			case xml.EndElement:
				return m, nil
			}
		}
	case "array":
		var a []any
		for {
			t, err := dec.Token()
			if err != nil {
				return nil, err
			}
			switch e := t.(type) {
			case xml.StartElement:
				v, err := plistValue(dec, e)
				if err != nil {
					return nil, err
				}
				a = append(a, v)
			case xml.EndElement:
				return a, nil
			}
		}
	case "true", "false":
		return se.Name.Local == "true", dec.Skip()
	case "string", "integer":
		var s string
		if err := dec.DecodeElement(&s, &se); err != nil {
			return nil, err
		}
		if se.Name.Local == "integer" {
			n, err := strconv.ParseInt(s, 0, 64)
			if err != nil {
				return nil, nil
			}
			return n, nil
		}
		return s, nil
	default:
		return nil, dec.Skip()
	}
}
