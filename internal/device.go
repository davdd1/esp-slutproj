package internal

import (
	"sync"
)

type Device struct {
	ID          string
	Status      string
	Temperature string
}

var devices = make(map[string]*Device)
var mu sync.Mutex // Mutex to protect concurrent access to the devices map

// RegisterDevice adds a new device to the devices map if it doesn't already exist.
func RegisterDevice(id string) {
	mu.Lock()
	defer mu.Unlock()
	if _, exists := devices[id]; !exists {
		devices[id] = &Device{ID: id, Status: "Connected"}
	}
}

//Check if MAC address is already registered
func IsKnownDevice(id string) bool {
	mu.Lock()
	defer mu.Unlock()
	_, exists := devices[id]
	return exists
}

// UpdateDeviceTemperature updates the temperature of a device in the devices map.
func UpdateDeviceTemperature(id string, temp string) {
	mu.Lock()
	defer mu.Unlock()
	if device, exists := devices[id]; exists {
		device.Temperature = temp
	}
}

// UnregisterDevice removes a device from the devices map. Currently unused in
func UnregisterDevice(id string) {
	mu.Lock()
	defer mu.Unlock()
	delete(devices, id)
}

// GetDevice returns a device from the devices map based on the provided ID.
func GetDevice(id string) *Device {
	mu.Lock()
	defer mu.Unlock()
	return devices[id]
}
