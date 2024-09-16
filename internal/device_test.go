package internal

import (
	"testing"
)

func TestRegisterDevice(t *testing.T) {
	RegisterDevice("00:11:22:33:44:55")
	device := GetDevice("00:11:22:33:44:55")
	if device == nil {
		t.Fatalf("Device not found")
	}
	if device.Status != "Connected" {
		t.Fatalf("Expected status 'Connected', got '%s'", device.Status)
	}
}

func TestUnregisterDevice(t *testing.T) {
	RegisterDevice("00:11:22:33:44:55")
	UnregisterDevice("00:11:22:33:44:55")
	device := GetDevice("00:11:22:33:44:55")
	if device != nil {
		t.Fatalf("Expected device to be nil")
	}
}
