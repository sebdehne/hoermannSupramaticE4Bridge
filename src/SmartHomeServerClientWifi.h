#ifndef _SMARTHOME_CLIENT_WIFI_H
#define _SMARTHOME_CLIENT_WIFI_H

#include "logger.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "secrets.h"
#include "config.h"
#include <SPI.h>
#include <WiFiNINA.h>


enum SmartHomeServerClientWifiState {
    INIT,
    WIFI_CONNECT,
    WIFI_CONNECTING,
    READING_DATA,
    MESSAGE_RECEIVED
};

class SmartHomeServerClientWifiClass
{
    private:
    IPAddress serverIp;
    WiFiUDP Udp;
    SmartHomeServerClientWifiState currentState = INIT;
    unsigned long currentStateChangedAt = millis();
    bool currentStateTimeout(unsigned long timeoutInMs);

    public:

    uint8_t receivedMessage[255];
    size_t receivedMessageLengh;

    void scheduleReconnect();
    void run();
    bool hasMessage();
    void markMessageConsumed();
    void send(uint8_t * buf, size_t len);
};

extern SmartHomeServerClientWifiClass SmartHomeServerClientWifi;

#endif