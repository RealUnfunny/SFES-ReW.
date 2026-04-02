#include <.common/comm.h>
#include <Arduino.h>
#include <SoftwareSerial.h>

#define MAX_BOXES 2
#define TIMEOUT_MS 500

FoodCondition evalLocal();
void RadioSetup();
void ParseBoxes(String resp);

void Poller();
void NodeMCUSender(SoftwareSerial *sender);
