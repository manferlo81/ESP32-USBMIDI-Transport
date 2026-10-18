#include "ESP32_USBMIDI_Transport.h"

#include <Arduino.h>

// Static variable to hold default device name
static const char* p__DefaultDeviceName = nullptr;

// Override weak definition of getUSBMIDIDefaultDeviceName to provide a default name
SET_USB_MIDI_DEVICE_NAME(p__DefaultDeviceName)

void ESP32_USBMIDI_Transport::setDeviceName(const char* deviceName) {
    p__DefaultDeviceName = deviceName;
}
