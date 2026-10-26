#pragma once

#include <USBMIDI.h>

class ESP32_USBMIDI_Transport {  //

private:
    static constexpr uint8_t RX_BUFFER_SIZE = 128;

private:
    uint8_t p__TX_Packet_Header;
    uint8_t p__TX_Packet_Bytes[3];
    uint8_t p__TX_Packet_Index = 0;
    uint8_t p__TX_Packed_ExpectedLength = 0;

private:
    uint8_t p__RX_Queue[RX_BUFFER_SIZE];
    uint8_t p__RX_Head = 0;
    uint8_t p__RX_Tail = 0;

private:
    USBMIDI p__USBMIDI;

public:
    bool thruActivated = false;

public:
    ESP32_USBMIDI_Transport(const char* name = nullptr);

    void begin(void);
    void begin(const uint32_t baud);

public:
    bool beginTransmission(const uint8_t type) const;
    void endTransmission(void) const;

public:
    unsigned available(void);
    uint8_t read(void);

public:
    void write(const uint8_t value);

private:
    bool p__TX_WritePacket();

private:
    void p__RX_addByteToQueue(const uint8_t value);

public:
    static void setDeviceName(const char* deviceName);

};  //
