#include <Arduino.h>
#include "config.h"
#include "SmartHomeServerClientWifi.h"
#include "hoermannE4.h"

void setup()
{
#ifdef DEBUG
  // setup Serial
  Serial.begin(115200);
  while (!Serial)
  {
    ;
  }
  Serial.println("OK");
#endif
}

void loop()
{

  SmartHomeServerClientWifi.run();
  HoermannE4.run();
}
