#define NODEMCU_FILE

#define NOTIFY_INTERVAL_MS 60000 // 1 minute

unsigned long last_notify = 0;

#include <Arduino.h>
#include <SoftwareSerial.h>

#include ".common/comm.h"
#include "CsvOperations.h"
#include "ESP8266WebServer.h"
#include "SetupFunctions.h"
#include "WebServer.h"

// opting for .csv because it's faster to parse and write
// than a formatted json file

#define EXPIRY_INTERVL 30000ll

#define HTTP_PORT 80
#define DAYS_GOING_BAD 2
// for perishables we cannot monitor, we broadly
// assume that the item will expire ~2 days before
// the final expiry date, except for items that items
// expires in two days, somehow (???).

#define MAX_BOX_COUNT 8
// Current Testing Value, total count should be ~260 boxes

SoftwareSerial arduino_wire(D2, D1);
ESP8266WebServer server(80);

unsigned long time_tc = 0;

void setup()
{
  d_SerialBegin(9600);

  WifiSetup();
  NTPSetup();
  NTFYSetup();
  WebSetup(&server);
  SDSetup();
  BootNotify();

  arduino_wire.begin(9600);
}

void loop()
{
  unsigned long rn = millis();
  server.handleClient();
  MessageHandler(&arduino_wire);

  if (rn - time_tc >= EXPIRY_INTERVL)
  {
    time_tc = rn;
    CheckAndNotify();
  }
}
