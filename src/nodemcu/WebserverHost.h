#pragma once

#define NODEMCU_FILE
#include <ESP8266WebServer.h>
/** @file WebserverHost.h
 *  @brief Sets up the AsyncWebServer on the NodeMCU
 */

/** @brief Connects all functions to
 */

void handleRoot(ESP8266WebServer *server);
void handleAdd(ESP8266WebServer *server);
void handleGetInventory(ESP8266WebServer *server);
void handleDelete(ESP8266WebServer *server);
void WebSetup(ESP8266WebServer *server);
