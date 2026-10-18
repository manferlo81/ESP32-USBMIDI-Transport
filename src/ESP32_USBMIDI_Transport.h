#pragma once

#include <USBMIDI.h>

class ESP32_USBMIDI_Transport {  //

    private:
        static constexpr uint8_t RX_BUFFER_SIZE = 128;

    private:
        uint8_t p__TX_Packet_Buffer[3];
        uint8_t p__TX_Index = 0;
        uint8_t p__TX_Buffer_Length = 0;

        uint8_t p__RX_Queue[RX_BUFFER_SIZE];
        uint8_t p__RX_Head = 0;
        uint8_t p__RX_Tail = 0;

    private:
        USBMIDI p__USBMIDI;

    public:
        bool thruActivated = false;

        ESP32_USBMIDI_Transport(const char* name = nullptr);

        void begin(void);
        void begin(const uint32_t baud);

        bool beginTransmission(const uint8_t type) const;
        void endTransmission(void) const;

        uint8_t read(void);
        void write(const uint8_t value);

        unsigned available(void);

    private:
        void p__RX_addByteToQueue(const uint8_t value);

    public:
        static void setDeviceName(const char* deviceName);

};  //
