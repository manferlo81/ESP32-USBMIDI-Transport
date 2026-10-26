#include "ESP32_USBMIDI_Transport.h"

#include <USB.h>
#include <USBMIDI.h>

ESP32_USBMIDI_Transport::ESP32_USBMIDI_Transport(const char* name) : p__USBMIDI() {
    setDeviceName(name);
}

void ESP32_USBMIDI_Transport::begin(void) {
    USB.begin();
    p__USBMIDI.begin();
}

void ESP32_USBMIDI_Transport::begin(const uint32_t) {
    begin();
}

bool ESP32_USBMIDI_Transport::beginTransmission(const uint8_t) const {
    return true;
}

void ESP32_USBMIDI_Transport::endTransmission(void) const {}

unsigned ESP32_USBMIDI_Transport::available(void) {  //

    // Declare packet to receive data
    midiEventPacket_t packet;

    // While there there is a packet
    while (p__USBMIDI.readPacket(&packet)) {  //

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

}  //

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

void ESP32_USBMIDI_Transport::write(const uint8_t b) {  //

    // Check if buffer is empty and ready to be initialized
    // This happens when p__TX_Index == 0 and p__TX_Packet_Header != MIDI_CIN_SYSEX_START
    if (p__TX_Packet_Header != MIDI_CIN_SYSEX_START && p__TX_Packet_Index == 0) {
        // Buffer is empty, ready to start receiving data

        // Set header
        p__TX_Packet_Header = (b >> 4);

        // Set expected length and/or handle SysEx
        switch (p__TX_Packet_Header) {  //

            // Three bytes messages
            case MIDI_CIN_NOTE_OFF:
            case MIDI_CIN_NOTE_ON:
            case MIDI_CIN_POLY_KEYPRESS:
            case MIDI_CIN_CONTROL_CHANGE:
            case MIDI_CIN_PITCH_BEND_CHANGE:
                // Three bytes expected
                p__TX_Packed_ExpectedLength = 3;
                break;

            // Two bytes messages
            case MIDI_CIN_PROGRAM_CHANGE:
            case MIDI_CIN_CHANNEL_PRESSURE:
                // Two bytes expected
                p__TX_Packed_ExpectedLength = 2;
                break;

            // One byte messages or SysEx
            case MIDI_CIN_1BYTE_DATA:
                if (b == MIDI_STATUS_SYSEX_START) {
                    // Start of SysEx
                    p__TX_Packet_Header = MIDI_CIN_SYSEX_START;
                    p__TX_Packed_ExpectedLength = 3;
                    break;
                } else {
                    // One byte expected
                    p__TX_Packed_ExpectedLength = 1;
                    break;
                }

            // Unknown message set expected length to 0
            // to indicate that nothing should be sent
            default:
                p__TX_Packed_ExpectedLength = 0;

        }  //

    }  //

    // Add byte to buffer and advance p__TX_Index
    p__TX_Packet_Bytes[p__TX_Packet_Index++] = b;

    // Intersect SysEx End byte, only if we are already sending a SysEx message
    // p__TX_Packet_Header == MIDI_CIN_SYSEX_START => Sending SysEx message
    // b == MIDI_STATUS_SYSEX_END => SysEx ends
    if (p__TX_Packet_Header == MIDI_CIN_SYSEX_START && b == MIDI_STATUS_SYSEX_END) {
        // SysEx ends, so we send it

        // Set SysEx End header
        // p__TX_Buffer_Index (1) + 4 = 5 : MIDI_CIN_SYSEX_END_1BYTE
        // p__TX_Buffer_Index (2) + 4 = 6 : MIDI_CIN_SYSEX_END_2BYTE
        // p__TX_Buffer_Index (3) + 4 = 7 : MIDI_CIN_SYSEX_END_3BYTE
        p__TX_Packet_Header =  p__TX_Packet_Index + 4;

        // Write SysEx End packet
        p__TX_WritePacket();

        // Clear header to indicate that we are not sending a SysEx message anymore
        p__TX_Packet_Header = 0;

        // Set index to 0 to indicate that we are ready to receive data
        p__TX_Packet_Index = 0;

    }  //

    // Check if TX buffer already full
    if (p__TX_Packet_Index >= p__TX_Packed_ExpectedLength) {
        // Buffer is full, so we send it

        // Send data if there is data to send
        if (p__TX_Packed_ExpectedLength > 0) p__TX_WritePacket();

        // Set index to 0 to indicate that we are ready to receive data
        p__TX_Packet_Index = 0;

    }  //

}  //

bool ESP32_USBMIDI_Transport::p__TX_WritePacket() {
    midiEventPacket_t packet = {
        p__TX_Packet_Header,
        p__TX_Packet_Bytes[0],
        p__TX_Packet_Index >= 2 ? p__TX_Packet_Bytes[1] : (uint8_t)0,
        p__TX_Packet_Index >= 3 ? p__TX_Packet_Bytes[2] : (uint8_t)0  //
    };
    return p__USBMIDI.writePacket(&packet);
}

void ESP32_USBMIDI_Transport::p__RX_addByteToQueue(const uint8_t value) {
    p__RX_Queue[p__RX_Tail] = value;
    p__RX_Tail = (p__RX_Tail + 1) % RX_BUFFER_SIZE;
}
