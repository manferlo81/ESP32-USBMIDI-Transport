#include "ESP32_USBMIDI_Transport.h"

#include <USB.h>
#include <USBMIDI.h>

static inline USBMIDI create_USBMIDI_instance(const char* name) {
    return (name == nullptr) ? USBMIDI() : USBMIDI(name);
}

ESP32_USBMIDI_Transport::ESP32_USBMIDI_Transport(const char* name) : p__usbMIDI(create_USBMIDI_instance(name)) {}

void ESP32_USBMIDI_Transport::begin(void) {
    USB.begin();
    p__usbMIDI.begin();
}

void ESP32_USBMIDI_Transport::begin(const uint32_t) {
    begin();
}

bool ESP32_USBMIDI_Transport::beginTransmission(const uint8_t) {
    return true;
}

void ESP32_USBMIDI_Transport::endTransmission(void) {}

uint8_t ESP32_USBMIDI_Transport::read(void) {  //

    // Return 0 if queue is empty
    if (p__RX_Head == p__RX_Tail) return 0;

    // Get byte from queue
    uint8_t b = p__RX_Queue[p__RX_Head];

    // Advance queue head
    p__RX_Head = (p__RX_Head + 1) % RX_BUFFER_SIZE;

    // Return byte
    return b;

}  //

void ESP32_USBMIDI_Transport::write(const uint8_t value) {  //

    // Add byte to buffer
    p__TX_Packet_Buffer[p__TX_Index++] = value;

    // Declare buffer length if this is the first byte
    if (p__TX_Index == 1) {
        uint8_t status = p__TX_Packet_Buffer[0] & 0xF0;
        // p__TX_Buffer_Length = 2 - Program Change, Channel Pressure
        // p__TX_Buffer_Length = 3 - Note On/Off, Control Change, Pitch Bend, etc.
        p__TX_Buffer_Length = (status == 0xC0 || status == 0xD0) ? 2 : 3;
    }

    // If buffer is full...
    if (p__TX_Index >= p__TX_Buffer_Length) {  //

        // Declare packet to send
        midiEventPacket_t packet;
        packet.header = (p__TX_Packet_Buffer[0] >> 4) & 0x0F;
        packet.byte1 = p__TX_Packet_Buffer[0];
        packet.byte2 = (p__TX_Buffer_Length > 1) ? p__TX_Packet_Buffer[1] : 0;
        packet.byte3 = (p__TX_Buffer_Length > 2) ? p__TX_Packet_Buffer[2] : 0;

        // Send packet
        p__usbMIDI.writePacket(&packet);

        // Reset buffer
        p__TX_Index = 0;
        p__TX_Buffer_Length = 0;

    }  //

}  //

unsigned ESP32_USBMIDI_Transport::available(void) {  //

    // Declare packet to receive data
    midiEventPacket_t packet;

    // While there there is a packet
    while (p__usbMIDI.readPacket(&packet)) {  //

        // Determine length
        uint8_t len = 3;
        uint8_t cin = packet.header & 0x0F;
        if (cin == 0x0C || cin == 0x0D)
            len = 2;

        // Add byte 1 to queue
        p__RX_addByteToQueue(packet.byte1);

        if (len > 1) {
            // Add byte 2 to queue
            p__RX_addByteToQueue(packet.byte2);
        }

        if (len > 2) {
            // Add byte 3 to queue
            p__RX_addByteToQueue(packet.byte3);
        }
    }

    // Return number of bytes in queue
    if (p__RX_Tail < p__RX_Head) return RX_BUFFER_SIZE - p__RX_Head + p__RX_Tail;
    return p__RX_Tail - p__RX_Head;
}

void ESP32_USBMIDI_Transport::p__RX_addByteToQueue(const uint8_t value) {
    p__RX_Queue[p__RX_Tail] = value;
    p__RX_Tail = (p__RX_Tail + 1) % RX_BUFFER_SIZE;
}
