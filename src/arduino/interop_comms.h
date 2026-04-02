#include <Arduino.h>
#include <SoftwareSerial.h>

#include ".common/comm.h"

String NodeMCUTransmit(Requests request, SoftwareSerial *sender);
