#include <Arduino.h>
#include <SoftwareSerial.h>

#include ".common/comm.h"
#include "interop_comms.h"

String NodeMCUTransmit(Requests request, SoftwareSerial *sender)
{
  switch (request)
  {
    case Requests::ActiveBoxes:
      if (!sender->print(ReqActBox))
        return "";
      delay(1000);
      return sender->readStringUntil(MSG_TERMINATOR);
    default:
      return "No Response";
  }
}
