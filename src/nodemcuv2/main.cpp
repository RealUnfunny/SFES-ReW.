#include "ESP8266WebServer.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

#include <SoftwareSerial.h>

#include "sensitive_data.c"
// Contains definitions for:
//     -> NTFY_SERVER
//     -> NTFY_PORT
//     -> NTFY_TOPIC
//     -> pass_list: Array of wifi_pass_t (as defined in common.h)

#define HTTP_PORT 80
#define DAYS_GOING_BAD 2
// for perishables we cannot monitor, we broadly
// assume that the item will expire ~2 days before
// the final expiry date, except for items that items
// expires in two days, somehow.

SoftwareSerial arduino(D6, D5);
WiFiClient client_server;
ESP8266WebServer server(HTTP_PORT);

void setup()
{
}

void loop()
{
}
