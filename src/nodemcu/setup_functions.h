#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "ESP8266WebServer.h"
#include "Esp.h"
#include "WiFiClient.h"
#include "wl_definitions.h"

#include ".common/comm.h"
#include "sensitive_data.c"
// Contains defines for:
//     -> NTFY_SERVER
//     -> NTFY_PORT
//     -> NTFY_TOPIC
//     -> pass_list: Array of wifi_pass_t (as defined in .common/comm.h)

#define HTTP_PORT 80

void WifiSetup();
void NTPSetup();

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
