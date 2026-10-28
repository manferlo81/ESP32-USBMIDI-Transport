#include "ESP32_USBMIDI_Transport.h"

#include <USB.h>
#include <USBMIDI.h>

ESP32_USBMIDI_Transport::ESP32_USBMIDI_Transport(const char* deviceName) : p__USBMIDI() {
    // Use library setDeviceName method instead of passing the name to USBMIDI Constructor
    setDeviceName(deviceName);
}

void ESP32_USBMIDI_Transport::begin(const uint32_t) {  //

    // Ensure USB is initialized
    // If it's already initialized this call has no effect
    USB.begin();

    // Initialize USBMIDI
    p__USBMIDI.begin();

}  //

bool ESP32_USBMIDI_Transport::beginTransmission(const uint8_t) const {
    return true;
}

void ESP32_USBMIDI_Transport::endTransmission(void) const {}

static uint8_t s__LengthFromCIN(const uint8_t cin) {  //

    switch (cin) {  //

        // Unsupported CIN
        case MIDI_CIN_MISC:         //
        case MIDI_CIN_CABLE_EVENT:  //
            return 0;

        // Three bytes messages
        case MIDI_CIN_SYSCOM_3BYTE:       // 3 bytes System Common message
        case MIDI_CIN_SYSEX_START:        // SysEx Start/Continue
        case MIDI_CIN_SYSEX_END_3BYTE:    // 3 bytes SysEx End message
        case MIDI_CIN_NOTE_OFF:           // Note On
        case MIDI_CIN_NOTE_ON:            // Note Off
        case MIDI_CIN_POLY_KEYPRESS:      //
        case MIDI_CIN_CONTROL_CHANGE:     // Control Change
        case MIDI_CIN_PITCH_BEND_CHANGE:  // Pitch Bend
            return 3;

        // Two bytes messages
        case MIDI_CIN_SYSCOM_2BYTE:      // 2 bytes System Common message
        case MIDI_CIN_SYSEX_END_2BYTE:   // 2 bytes SysEx End message
        case MIDI_CIN_PROGRAM_CHANGE:    // Program Change
        case MIDI_CIN_CHANNEL_PRESSURE:  //
            return 2;

        // One byte messages or SysEx
        case MIDI_CIN_SYSEX_END_1BYTE:  // 1 byte SysEx End message
        case MIDI_CIN_1BYTE_DATA:       // 1 byte message
            return 1;

    }  //

    // Unknown CIN
    return 0;

}  //

void ESP32_USBMIDI_Transport::p__RX_addByteToQueue(const uint8_t value) {  //

    // Add byte to RX Queue
    p__RX_Queue[p__RX_Tail] = value;

    // Advance RX Queue tail index
    p__RX_Tail = (p__RX_Tail + 1) % RX_BUFFER_SIZE;

}  //

unsigned ESP32_USBMIDI_Transport::available(void) {  //

    // Declare packet to receive data
    midiEventPacket_t packet;

    // While there is a packet...
    while (p__USBMIDI.readPacket(&packet)) {  //

        // Determine length
        const uint8_t cin = packet.header & 0x0F;
        uint8_t len = s__LengthFromCIN(cin);

        // Add byte 1 to queue if length is 1 or more
        if (len > 0) p__RX_addByteToQueue(packet.byte1);

        // Add byte 2 to queue if length is 2 or more
        if (len > 1) p__RX_addByteToQueue(packet.byte2);

        // Add byte 3 to queue if length is 3
        if (len > 2) p__RX_addByteToQueue(packet.byte3);

    }  //

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

void ESP32_USBMIDI_Transport::p__TX_WritePacket() {  //

    // Exit if there is nothing to send
    if (!p__TX_Packet_Index) return;

    // Constant Expression
    static constexpr uint8_t uint8_zero = 0;

    // Create packet
    midiEventPacket_t packet = {
        p__TX_Packet_Header,
        p__TX_Packet_Bytes[0],
        p__TX_Packet_Index > 1 ? p__TX_Packet_Bytes[1] : uint8_zero,
        p__TX_Packet_Index > 2 ? p__TX_Packet_Bytes[2] : uint8_zero  //
    };

    // Write packet
    p__USBMIDI.writePacket(&packet);

    // Set index to 0 to indicate that we are ready to receive data
    p__TX_Packet_Index = 0;

}  //

void ESP32_USBMIDI_Transport::write(const uint8_t midiByte) {  //

    // Check if buffer is empty and ready to be initialized
    // p__TX_Packet_Index == 0 => This will be the first byte in the buffer
    // p__TX_Packet_Header != MIDI_CIN_SYSEX_START => We are not currently receiving SysEx message
    if (p__TX_Packet_Index == 0 && p__TX_Packet_Header != MIDI_CIN_SYSEX_START) {
        // Buffer is empty, ready to start receiving data

        // Set header
        p__TX_Packet_Header = (midiByte >> 4);

        // Determine expected length
        p__TX_Packed_ExpectedLength = s__LengthFromCIN(p__TX_Packet_Header);

        // Modify Header and length if a SysEx Start received
        if (p__TX_Packed_ExpectedLength == 1 && p__TX_Packet_Header == MIDI_CIN_1BYTE_DATA && midiByte == MIDI_STATUS_SYSEX_START) {  //

            // Set header to MIDI_CIN_SYSEX_START
            // indicating we are receiving a SysEx message
            p__TX_Packet_Header = MIDI_CIN_SYSEX_START;

            // Set length to maximum (3) to allow for full SysEx message
            p__TX_Packed_ExpectedLength = 3;

        }  //

    }  //

    // Add byte to buffer and advance buffer index
    p__TX_Packet_Bytes[p__TX_Packet_Index++] = midiByte;

    // Intersect SysEx End byte, only if we are already receiving a SysEx message
    // p__TX_Packet_Header == MIDI_CIN_SYSEX_START => Receiving SysEx message
    // b == MIDI_STATUS_SYSEX_END => SysEx End
    if (p__TX_Packet_Header == MIDI_CIN_SYSEX_START && midiByte == MIDI_STATUS_SYSEX_END) {
        // SysEx ended, so we send it

        // Set SysEx End header
        // p__TX_Buffer_Index (1) + 4 = MIDI_CIN_SYSEX_END_1BYTE (5)
        // p__TX_Buffer_Index (2) + 4 = MIDI_CIN_SYSEX_END_2BYTE (6)
        // p__TX_Buffer_Index (3) + 4 = MIDI_CIN_SYSEX_END_3BYTE (7)
        p__TX_Packet_Header = p__TX_Packet_Index + 4;

        // Send packet and reset index
        p__TX_WritePacket();

        // Clear header to indicate that we are not receiving a SysEx message anymore
        p__TX_Packet_Header = 0;

        // Exit to prevent the write routine to continue
        // Message already been sent, so we are done
        return;

    }  //

    // Check if TX buffer already full
    if (p__TX_Packet_Index >= p__TX_Packed_ExpectedLength) {
        // Buffer is full, so we send it

        // Send packet and reset index
        p__TX_WritePacket();

    }  //

}  //
