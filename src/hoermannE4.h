#ifndef _HOERMANN_E4
#define _HOERMANN_E4

#include <Arduino.h>
#include "logger.h"
#include "utils.h"

enum HoermannE4State
{
    E4_INIT,
    E4_READING

};

class HoermannE4Class
{
private:
    size_t tail_size = 2;
    size_t func23_header_size = 11;
    size_t func16_header_size = 7;
    unsigned long messageSeperatePauseMs = 4800;
    uint8_t my_bus_id = 2;

    uint8_t receiveBuffer[255];
    size_t bytesRead = 0;
    uint8_t txBuffer[255];
    size_t txLen = 0;
    uint8_t previousCounter = 0;

    void send(uint8_t *buf, size_t len);
    bool validateCrc16(size_t len);
    void restartReading();
    void handleMessage();
    void printReceiveBuffer();

    uint8_t byteCount = 0;

    HoermannE4State currentState = E4_INIT;
    unsigned long currentStateChangedAt = millis();
    unsigned long readingStartedAt = 0;
    unsigned long lastReadAt = 0;

public:
    void run();
};

extern HoermannE4Class HoermannE4;

#endif
