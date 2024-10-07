#include "SmartHomeServerClientWifi.h"

void SmartHomeServerClientWifiClass::scheduleReconnect()
{
    currentState = WIFI_CONNECT;
    currentStateChangedAt = millis();
}

bool SmartHomeServerClientWifiClass::hasMessage()
{
    return currentState == MESSAGE_RECEIVED;
}

void SmartHomeServerClientWifiClass::markMessageConsumed()
{
    currentState = READING_DATA;
    currentStateChangedAt = millis();
}

void SmartHomeServerClientWifiClass::send(uint8_t *buf, size_t len)
{
    if (currentState != WIFI_CONNECTING)
    {
        if (!Udp.beginPacket(REMOTE_UDP_IP, REMOTE_UDP_PORT))
        {
            Log.log("SmartHomeServerClientWifi.send() - error beginPacket()");
            return;
        };
        Udp.write(buf, len);
        if (!Udp.endPacket())
        {
            Log.log("SmartHomeServerClientWifi.send() - error endPacket()");
            return;
        }
    }
}

void SmartHomeServerClientWifiClass::run()
{

    switch (currentState)
    {
    case INIT:
    {

        serverIp.fromString(REMOTE_UDP_IP);

        // pinMode(NINA_RESETN, OUTPUT);
        // digitalWrite(NINA_RESETN, 1);
        // delay(100);
        // digitalWrite(NINA_RESETN, 0);
        // delay(100);

        if (WiFi.status() == WL_NO_MODULE)
        {
            Log.log("No wifi module detected");
            delay(10000);
            return;
        }

        static const char *expectedFirmeware = WIFI_FIRMWARE_LATEST_VERSION;
        static const char *actualFirmware = WiFi.firmwareVersion();

        if (strcmp(expectedFirmeware, actualFirmware) != 0)
        {
            char buf[1024];
            snprintf(buf, sizeof(buf), "Firmware upgrade needed: expected: %s, actual: %s", expectedFirmeware, actualFirmware);
            Log.log(buf);
            delay(10000);
            return;
        }

        currentState = WIFI_CONNECT;
        currentStateChangedAt = millis();

        break;
    }
    case WIFI_CONNECT:
    {
        WiFi.disconnect();
        WiFi.wifiSetPassphrase(WIFI_SSID, WIFI_PASS);
        currentState = WIFI_CONNECTING;
        currentStateChangedAt = millis();
        break;
    }
    case WIFI_CONNECTING:
        if (currentStateTimeout(WIFI_TIMEOUT_MS))
        {
            Log.log("SmartHomeServerClientWifi - wifi connect failed, resetting");
            currentState = WIFI_CONNECT;
            currentStateChangedAt = millis();
            break;
        }

        if (WiFi.begin() == WL_CONNECTED)
        {
            if (Udp.begin(LOCAL_UDP_PORT))
            {
                IPAddress localIP = WiFi.localIP();
                char buf[255];
                sniprintf(buf, sizeof(buf), "SmartHomeServerClientWifi - wifi connected. Listening on: %u.%u.%u.%u:%u", localIP[0], localIP[1], localIP[2], localIP[3], LOCAL_UDP_PORT);
                Log.log(buf);
                currentState = READING_DATA;
                currentStateChangedAt = millis();
            }
            else
            {
                Log.log("Udp.begin() - error");
            }
        }

        break;
    case MESSAGE_RECEIVED:
        // TODO
        markMessageConsumed();

        break;
    case READING_DATA:
        if (Udp.parsePacket())
        {
            IPAddress remoteIp = Udp.remoteIP();
            uint16_t remotePort = Udp.remotePort();
            char buf[255];
            sniprintf(buf, sizeof(buf), "Received msg from: %u.%u.%u.%u:%u", remoteIp[0], remoteIp[1], remoteIp[2], remoteIp[3], remotePort);
            Log.log(buf);

            receivedMessageLengh = Udp.read(receivedMessage, sizeof(receivedMessage));
            if (remoteIp == serverIp)
            {
                currentState = MESSAGE_RECEIVED;
                currentStateChangedAt = millis();
            }
            else
            {
                Log.log("Ignoring message");
            }
        }
        break;

    default:
        break;
    }
}

bool SmartHomeServerClientWifiClass::currentStateTimeout(unsigned long timeoutInMs)
{
    unsigned long now = millis();
    unsigned long elapsed = now - currentStateChangedAt;
    return elapsed > timeoutInMs;
}

SmartHomeServerClientWifiClass SmartHomeServerClientWifi;
