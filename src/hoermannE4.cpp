#include "hoermannE4.h"
#include "crc16.h"

bool HoermannE4Class::validateCrc16(size_t len)
{

    uint16_t crc = 0;
    crc = receiveBuffer[len + 1];
    crc <<= 8;
    crc = crc + receiveBuffer[len];

    uint16_t calcCrc16 = crc16(receiveBuffer, len);

    return calcCrc16 == crc;
}

void HoermannE4Class::printReceiveBuffer()
{
    char hexBuf[sizeof(receiveBuffer) * 2];
    char logBuf[sizeof(hexBuf) + 100];

    toHex(receiveBuffer, bytesRead, hexBuf);

    sniprintf(logBuf, sizeof(logBuf), "Received: %s", hexBuf);

    Log.log(logBuf);
}

void HoermannE4Class::restartReading()
{
    bytesRead = 0;
    currentState = E4_READING;
    currentStateChangedAt = millis();
}

void HoermannE4Class::send(uint8_t *buf, size_t len)
{
    uint16_t crc = crc16(buf, len);
    uint8_t sendBuf[len + 2];
    memcpy(sendBuf, buf, len);

    sendBuf[len + 0] = (crc) & 0xff;
    sendBuf[len + 1] = (crc >> 8) & 0xff;

    char logbuf[1024];
    char hexBuf[len * 2];
    toHex(sendBuf, sizeof(sendBuf), hexBuf);
    sniprintf(logbuf, sizeof(logbuf), "Sending: %s", hexBuf);
    Log.log(logbuf);

    Serial1.write(sendBuf, sizeof(sendBuf));
}

void HoermannE4Class::handleMessage()
{
    txLen = 0;
    uint8_t func = receiveBuffer[1];
    if (func != 0x17 && func != 0x10)
    {
        char buf[255];
        sniprintf(buf, sizeof(buf), "Invalid func code %u received", func);
        Log.log(buf);
        return;
    }

    if (func == 0x10)
    {
        uint16_t writeOffset = receiveBuffer[2];
        writeOffset <<= 8;
        writeOffset = writeOffset + receiveBuffer[3];
        uint16_t writeLen = receiveBuffer[4];
        writeLen <<= 8;
        writeLen = writeLen + receiveBuffer[5];
        byteCount = receiveBuffer[6];

        bool crcIsValid = validateCrc16(func16_header_size + byteCount);
        if (!crcIsValid)
        {
            Log.log("Func16: Invalid CRC");
            restartReading();
            return;
        }

        // broadcast 00109D31000912430000004060000000000000100000010000720D
        if (receiveBuffer[0] == 0x00 && writeOffset == 0x9d31 && writeLen == 9 && byteCount == 18)
        {
            uint8_t doorTargetPosition = receiveBuffer[9];
            uint8_t doorCurrentPosition = receiveBuffer[10];
            uint8_t doorState = receiveBuffer[11];

            // 14 .. from docs (a indicator for automatic state maby?)
            // 10 .. on after turn on
            // 04 .. shut down after inactivy
            // 00 .. off after turn off
            uint8_t lampOn = receiveBuffer[20];

            // Log.log("Boradcast OK");
        }
        else
        {
            Log.log("Unknown Boradcast");
        }
    }

    if (func == 0x17 && receiveBuffer[0] == my_bus_id)
    {
        uint16_t readOffset = receiveBuffer[2];
        readOffset <<= 8;
        readOffset = readOffset + receiveBuffer[3];
        uint16_t readLen = receiveBuffer[4];
        readLen <<= 8;
        readLen = readLen + receiveBuffer[5];
        uint16_t writeOffset = receiveBuffer[6];
        writeOffset <<= 8;
        writeOffset = writeOffset + receiveBuffer[7];
        uint16_t writeLen = receiveBuffer[8];
        writeLen <<= 8;
        writeLen = writeLen + receiveBuffer[9];
        byteCount = receiveBuffer[10];

        // See https://blog.dupas.be/posts/hoermann-uap-hcp1/

        bool crcIsValid = validateCrc16(func23_header_size + byteCount);
        if (!crcIsValid)
        {
            Log.log("Func23: Invalid CRC");
            restartReading();
            return;
        }

        // bus-scan
        if (readOffset == 0x9cb9 && readLen == 5 && writeOffset == 0x9c41 && writeLen == 3 && byteCount == 6)
        {
            Log.log("Handling bus-scan");
            uint8_t cnt = receiveBuffer[11];
            uint8_t cmd = receiveBuffer[12];

            int i = 0;
            txBuffer[i++] = receiveBuffer[0];
            txBuffer[i++] = 0x17;
            txBuffer[i++] = 10; // byteCount
            txBuffer[i++] = cnt;
            txBuffer[i++] = 0;
            txBuffer[i++] = cmd;
            txBuffer[i++] = 0x05;
            writeUint16(0x0430, txBuffer, i++);
            i++;
            writeUint16(0x10ff, txBuffer, i++);
            i++;
            writeUint16(0xa845, txBuffer, i++);
            i++;
            txLen = i;
        }

        // Command request
        else if (readOffset == 0x9cb9 && readLen == 8 && writeOffset == 0x9c41 && writeLen == 2 && byteCount == 4)
        {
            // Log.log("Handling Command request");
            uint8_t cnt = receiveBuffer[11];
            uint8_t cmd = receiveBuffer[12];
            int i = 0;
            txBuffer[i++] = receiveBuffer[0];
            txBuffer[i++] = 0x17;
            txBuffer[i++] = readLen * 2; // byteCount (16)
            txBuffer[i++] = cnt;
            txBuffer[i++] = 0x00;
            txBuffer[i++] = cmd;
            txBuffer[i++] = 0x01;
            txBuffer[i++] = 0;    // TODO 7
            txBuffer[i++] = 0;    // TODO 8
            txBuffer[i++] = 0;    // TODO 9
            txBuffer[i++] = 0x00; // 10
            txBuffer[i++] = 0x00; // 11
            txBuffer[i++] = 0x00; // 12
            txBuffer[i++] = 0x00; // 13
            txBuffer[i++] = 0x00; // 14
            txBuffer[i++] = 0x00; // 15
            txBuffer[i++] = 0x00; // 16
            txBuffer[i++] = 0x00; // 17
            txBuffer[i++] = 0x00; // 18
            txLen = i;

            
        }

        else
        {
            char buf[1024];
            Log.log(buf);
            sniprintf(buf, sizeof(buf), "Unsupported fun23-msg received: readOffset=%u, readLen=%u, writeOffset=%u, writeLen=%u", readOffset, readLen, writeOffset, writeLen);
            Log.log(buf);
        }
    }
}

void HoermannE4Class::run()
{
    switch (currentState)
    {
    case E4_INIT:
    {
        Serial1.begin(57600, SERIAL_8E1);
        restartReading();
        break;
    }
    case E4_READING:
    {
        bool readSomething = false;
        size_t bytesReadThisTime = 0;
        while (Serial1.available())
        {
            int read = Serial1.read();

            if (read < 0)
            {
                break;
            }

            if (bytesRead == sizeof(receiveBuffer))
            {
                Log.log("Read buffer full, flushing");
                restartReading();
                break;
            }
            receiveBuffer[bytesRead++] = read;
            bytesReadThisTime++;
            readSomething = true;
        }

        if (readSomething)
        {
            lastReadAt = micros();
        }
        else if (!readSomething && bytesRead > 0 && (micros() - lastReadAt) > messageSeperatePauseMs)
        {
            printReceiveBuffer();
            handleMessage();

            if (txLen > 0)
            {
                send(txBuffer, txLen);
            }

            restartReading();
        }

        break;
    }

    default:
        break;
    }
}

HoermannE4Class HoermannE4;
