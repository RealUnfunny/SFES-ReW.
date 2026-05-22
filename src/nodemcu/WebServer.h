#define NODEMCU_FILE

#include <ESP8266WebServer.h>

void handleRoot(ESP8266WebServer *server);
void handleGetInventory(ESP8266WebServer *server);
void handleAdd(ESP8266WebServer *server);
void handleDelete(ESP8266WebServer *server);
void handleClear(ESP8266WebServer *server);
void WebSetup(ESP8266WebServer *server);
