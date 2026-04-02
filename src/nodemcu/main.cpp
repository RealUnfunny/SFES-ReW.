#define NODEMCU_FILE

#define NOTIFY_INTERVAL_MS 60000 // 1 minute

unsigned long last_notify = 0;

#include <Arduino.h>
#include <SoftwareSerial.h>

#include ".common/comm.h"
#include "CsvOperations.h"
#include "SetupFunctions.h"
#include "WebserverHost.h"

// opting for .csv because it's faster to parse and write
// than a formatted json file

#define SENSOR_TIMEOUT 60000ll
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

void setup()
{
  d_SerialBegin(115200);

  WifiSetup();
  NTPSetup();
  NTFYSetup();
  SDSetup();

  LoadPhysicals();
  arduino_wire.begin(9600);

  WebSetup(&server);
}

void loop()
{
  server.handleClient();
  if (arduino_wire.available())
  {
    delay(50);
    String msg = arduino_wire.readStringUntil(MSG_TERMINATOR);
    msg.trim();
    d_SerialPrintln(msg); // debug, see what's actually arriving
    if (!strcmp(msg.c_str(), ReqActBox2))
    {
      d_SerialPrintln("Data requested!");
      WritePhysicals(&arduino_wire);
    }
    if (!strcmp(msg.c_str(), UpdBoxes))
    {
      d_SerialPrintln("Expecting Boxes.");
      arduino_wire.print("Ok.^");
      delay(50);
      String msg = arduino_wire.readStringUntil(MSG_TERMINATOR);
      delay(50);
      CSVUpdate(msg);
    }
  }
  else
  {
    d_SerialPrintln("Read Nothing?");
    delay(1000);
  }

  unsigned long now = millis();

  if (now - last_notify >= NOTIFY_INTERVAL_MS)
  {
    last_notify = now;
    CheckAndNotify();
  }
}
