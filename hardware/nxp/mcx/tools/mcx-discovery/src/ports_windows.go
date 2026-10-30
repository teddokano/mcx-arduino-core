package main

import (
	"strings"
	"syscall"
	"unsafe"
)

// findProbePorts asks SetupAPI for the present devices of the Ports class.
// A COM port's own device is one interface of the probe
// (USB\VID_1FC9&PID_0143&MI_xx\...); the probe itself is the parent above it
// whose ID is USB\VID_1FC9&PID_0143\<serial number>, and Windows keeps the
// probe's USB product string as its bus-reported device description.
func findProbePorts() []probePort {
	set, _, _ := procSetupDiGetClassDevsW.Call(uintptr(unsafe.Pointer(&guidDevclassPorts)), 0, 0, digcfPresent)
	if set == invalidHandle {
		return nil
	}
	defer procSetupDiDestroyDeviceInfoList.Call(set)

	var found []probePort
	for i := uint32(0); ; i++ {
		var info spDevinfoData
		info.cbSize = uint32(unsafe.Sizeof(info))
		if ok, _, _ := procSetupDiEnumDeviceInfo.Call(set, uintptr(i), uintptr(unsafe.Pointer(&info))); ok == 0 {
			break
		}
		probe, serial, ok := probeAbove(info.devInst)
		if !ok {
			continue
		}
		name := portName(set, &info)
		if name == "" {
			continue
		}
		found = append(found, probePort{address: name, serial: serial, product: busReportedDesc(probe)})
	}
	return found
}

const probeIDPrefix = `USB\VID_1FC9&PID_0143\`

// probeAbove walks up from a COM port's device to the probe's USB device.
func probeAbove(inst uint32) (probe uint32, serial string, ok bool) {
	for depth := 0; depth < 4; depth++ {
		var parent uint32
		if r, _, _ := procCMGetParent.Call(uintptr(unsafe.Pointer(&parent)), uintptr(inst), 0); r != crSuccess {
			return 0, "", false
		}
		inst = parent
		id := deviceID(inst)
		if strings.HasPrefix(strings.ToUpper(id), probeIDPrefix) {
			return inst, id[len(probeIDPrefix):], true
		}
		if !strings.HasPrefix(strings.ToUpper(id), `USB\VID_1FC9&PID_0143&`) {
			return 0, "", false
		}
	}
	return 0, "", false
}

func deviceID(inst uint32) string {
	buf := make([]uint16, 512)
	if r, _, _ := procCMGetDeviceIDW.Call(uintptr(inst), uintptr(unsafe.Pointer(&buf[0])), uintptr(len(buf)), 0); r != crSuccess {
		return ""
	}
	return syscall.UTF16ToString(buf)
}

func busReportedDesc(inst uint32) string {
	buf := make([]uint16, 256)
	size := uint32(len(buf) * 2)
	var typ uint32
	r, _, _ := procCMGetDevNodePropertyW.Call(uintptr(inst), uintptr(unsafe.Pointer(&devpkeyBusReportedDeviceDesc)),
		uintptr(unsafe.Pointer(&typ)), uintptr(unsafe.Pointer(&buf[0])), uintptr(unsafe.Pointer(&size)), 0)
	if r != crSuccess || typ != devpropTypeString {
		return ""
	}
	return syscall.UTF16ToString(buf)
}

// portName reads "COMn" from the device's registry key.
func portName(set uintptr, info *spDevinfoData) string {
	key, _, _ := procSetupDiOpenDevRegKey.Call(set, uintptr(unsafe.Pointer(info)), dicsFlagGlobal, 0, diregDev, syscall.KEY_READ)
	if key == invalidHandle {
		return ""
	}
	defer syscall.RegCloseKey(syscall.Handle(key))
	buf := make([]uint16, 64)
	size := uint32(len(buf) * 2)
	var typ uint32
	name, _ := syscall.UTF16PtrFromString("PortName")
	if syscall.RegQueryValueEx(syscall.Handle(key), name, nil, &typ, (*byte)(unsafe.Pointer(&buf[0])), &size) != nil ||
		typ != syscall.REG_SZ {
		return ""
	}
	return syscall.UTF16ToString(buf)
}

type guid struct {
	data1 uint32
	data2 uint16
	data3 uint16
	data4 [8]byte
}

type spDevinfoData struct {
	cbSize    uint32
	classGUID guid
	devInst   uint32
	reserved  uintptr
}

type devpropkey struct {
	fmtid guid
	pid   uint32
}

const (
	digcfPresent      = 0x2
	dicsFlagGlobal    = 0x1
	diregDev          = 0x1
	crSuccess         = 0
	devpropTypeString = 0x12
	invalidHandle     = ^uintptr(0)
)

var (
	guidDevclassPorts            = guid{0x4d36e978, 0xe325, 0x11ce, [8]byte{0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}}
	devpkeyBusReportedDeviceDesc = devpropkey{guid{0x540b947e, 0x8b40, 0x45bc, [8]byte{0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2}}, 4}

	setupapi                         = syscall.NewLazyDLL("setupapi.dll")
	procSetupDiGetClassDevsW         = setupapi.NewProc("SetupDiGetClassDevsW")
	procSetupDiEnumDeviceInfo        = setupapi.NewProc("SetupDiEnumDeviceInfo")
	procSetupDiOpenDevRegKey         = setupapi.NewProc("SetupDiOpenDevRegKey")
	procSetupDiDestroyDeviceInfoList = setupapi.NewProc("SetupDiDestroyDeviceInfoList")

	cfgmgr32                  = syscall.NewLazyDLL("cfgmgr32.dll")
	procCMGetParent           = cfgmgr32.NewProc("CM_Get_Parent")
	procCMGetDeviceIDW        = cfgmgr32.NewProc("CM_Get_Device_IDW")
	procCMGetDevNodePropertyW = cfgmgr32.NewProc("CM_Get_DevNode_PropertyW")
)
