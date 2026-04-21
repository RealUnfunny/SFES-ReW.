#include <Arduino.h>
#include <SoftwareSerial.h>

#include ".common/comm.h"

bool NodeMCUTransmit(Requests request, SoftwareSerial *serial, const char *payload, char *out_buffer,
                     size_t buffer_size);
