#include ".common/comm.h"
#include "ESP8266WebServer.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

#include <SoftwareSerial.h>

#include "sensitive_data.c"
#include "wl_definitions.h"
// Contains definitions for:
//     -> NTFY_SERVER
//     -> NTFY_PORT
//     -> NTFY_TOPIC
//     -> pass_list: Array of wifi_pass_t (as defined in .common/comm.h)

#define DEBUG_MODE

#define INVENTORY_FILE "/inventory.csv"

#define SENSOR_TIMEOUT 60000l
#define EXPIRY_INTERVL 30000l

#define HTTP_PORT 80
#define DAYS_GOING_BAD 2
// for perishables we cannot monitor, we broadly
// assume that the item will expire ~2 days before
// the final expiry date, except for items that items
// expires in two days, somehow (???).

#define MAX_BOX_COUNT 8 // Current Testing Value for ze packed array

SoftwareSerial arduino(D6, D5);
WiFiClient client_server;
ESP8266WebServer server(HTTP_PORT);

void WifiSetup();

void setup()
{
}

void loop()
{
}

void WifiSetup()
{
    for (wifi_pass_t wifi_pass : pass_list)
    {
#ifdef DEBUG_MODE
        Serial.print("Connecting to...");
        Serial.print(wifi_pass.ssid);
#endif

        WiFi.begin(wifi_pass.ssid, wifi_pass.pass);

#ifdef DEBUG_MODE
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }
        Serial.println("\nWiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
#endif
        break;
    }
}
