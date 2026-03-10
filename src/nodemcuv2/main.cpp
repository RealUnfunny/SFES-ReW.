/* Function Docs
 *
 * WifiSetup()
 *     iterates through "pass_list" (defined in sensitive_data.c),
 *     only goes to the first successful connection, and not best
 *     connection, as in the current build the webserver's size isn't
 *     really known yet. [09 Mar 2026]
 *
 * NTPSetup()
 *     connects to the NTP internet cluster for the current time,
 *     because there was a problem with new items added on the
 *     WebServer set from relative to the unix time epoch (1st
 *     January 1970); Currently only set an offset to IST, and
 *     not for other regions as of yet [09 Mar 2026]
 * */

#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

#include <SoftwareSerial.h>
#include <ctime>

#include "Esp.h"
#include "WiFiClient.h"

#include ".common/comm.h"
#include "sensitive_data.c"
// Contains defines for:
//     -> NTFY_SERVER
//     -> NTFY_PORT
//     -> NTFY_TOPIC
//     -> pass_list: Array of wifi_pass_t (as defined in .common/comm.h)

#include "wl_definitions.h"

// #define DEBUG_MODE
//
// Above macro makes sure that the Serial Monitor (and such)
// are only active when I need it, elsewise it is excluded

#define INVENTORY_FILE "/inventory.csv"
// opting for .csv because it's faster to parse and write
// than a formatted json file

#define SENSOR_TIMEOUT 60000l
#define EXPIRY_INTERVL 30000l

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

void WifiSetup()
{
    for (wifi_pass_t wifi_pass : pass_list)
    {
        d_SerialPrint("Connecting to...");
        d_SerialPrint(wifi_pass.ssid);

        WiFi.begin(wifi_pass.ssid, wifi_pass.pass);

        while ((WiFi.status() != WL_CONNECTED) or (WiFi.status() != WL_CONNECT_FAILED))
        {
            d_delay(500);
            d_SerialPrint(".");
        }

        switch (WiFi.status())
        {
        case WL_CONNECTED:
            d_SerialPrintln("\nWiFi connected");
            d_SerialPrint("IP address: ");
            d_SerialPrintln(WiFi.localIP());
            return;
            // Kill the function on connecting
        default:
            continue;
        }
    }
}

void NTPSetup()
{
    time_t system_time = time(nullptr);
    uint8_t retries = 0;

    do
    {
        d_SerialPrintln("Configuring NTP...");
        configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
        d_SerialPrint("Waiting for NTP...");

        for (int attempts = 0; system_time == 0 and attempts < 20; attempts++)
        {
            delay(500);
            system_time = time(nullptr);
            d_SerialPrint(".");
        }

        if (system_time != 0)
        {
            d_SerialPrintln("SUCCESS!");
            d_SerialPrintf("Current Unix time: %lu\n", system_time);
            return;
        }

        d_SerialPrintln("Warning: NTP Sync failed. Retrying...");

        retries++;

        if (retries >= 3)
        {
            d_SerialPrintln("Too many retries, restarting...");
            delay(1000);
            ESP.restart();
            // Assuming that if the system has failed to the get the
            // NTP Connection for nearly ~60 times, there must be
            // wrong with the initial setup process
        }
    } while (system_time == 0);
}
