#include <SoftwareSerial.h>
#include <ctime>

#include ".common/comm.h"
#include "setup_functions.h"

#define INVENTORY_FILE "/inventory.csv"
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

SoftwareSerial arduino(D6, D5);
WiFiClient client_server;
ESP8266WebServer server(HTTP_PORT);

void WifiSetup();
void NTPSetup();

void setup()
{
    WifiSetup();

    d_SerialBegin(115200);
    arduino.begin(9600);
}

void loop()
{
}
