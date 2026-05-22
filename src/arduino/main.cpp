#define POLL_INTERVAL_MS 5000
#define REFRESH_INT_MS 30000

#include "LcdScreen.h"
#include "box_actions.h"
#include "interop_comms.h"
#include <.common/comm.h>
#include <Arduino.h>
#include <SoftwareSerial.h>

unsigned long last_poll = 0;
unsigned long last_refresh = 0;

unsigned long time = 0;
// const int SENSOR_READ_DELAY = 100;

const int status_pins[] = {Pins::FRESH_PIN, Pins::GOING_BAD_PIN, Pins::EXPIRED_PIN};

int DetermineFoodStatus();
void UpdateStatusDisplay(int Cond);

SoftwareSerial nodemcu(5, 6);

void setup()
{
  d_SerialBegin(9600);

  RadioSetup();

  pinMode(Pins::FRESH_PIN, OUTPUT);
  pinMode(Pins::GOING_BAD_PIN, OUTPUT);
  pinMode(Pins::EXPIRED_PIN, OUTPUT);

  pinMode(Pins::ALC_SENSOR_PIN, INPUT);
  pinMode(Pins::CH3_SENSOR_PIN, INPUT);

  digitalWrite(Pins::FRESH_PIN, HIGH);
  delay(1000);
  digitalWrite(Pins::FRESH_PIN, LOW);
  delay(1000);
  digitalWrite(Pins::GOING_BAD_PIN, HIGH);
  delay(1000);
  digitalWrite(Pins::GOING_BAD_PIN, LOW);
  delay(1000);
  digitalWrite(Pins::EXPIRED_PIN, HIGH);
  delay(1000);
  digitalWrite(Pins::EXPIRED_PIN, LOW);

  d_SerialPrintln("SFES Main Indicator Init'sed");

  nodemcu.begin(9600);

  char response[120];

  d_SerialPrintln("Entering Setup, sending Active Boxes request.");
  if (NodeMCUTransmit(Requests::ActiveBoxes, &nodemcu, NULL, response, sizeof(response)))
  {
    d_SerialPrint("Receieved input:");
    d_SerialPrintlnV(response);
    ParseBoxes(response);
  }
  else
  {
    d_SerialPrintln("Failed to get active boxes");
  }

  UiSetup(&nodemcu);
}

void loop()
{
  unsigned long now = millis();

  if (now - last_poll >= POLL_INTERVAL_MS)
  {
    d_SerialPrintln("Polling...");
    last_poll = now;
    Poller();
    NodeMcuUpdate(&nodemcu);
  }
  if (now - last_refresh >= REFRESH_INT_MS)
  {
    d_SerialPrintln("Refreshing...");
    last_refresh = now;
    char response[120];

    if (NodeMCUTransmit(Requests::ActiveBoxes, &nodemcu, NULL, response, sizeof(response)))
    {
      d_SerialPrint("Receieved input:");
      d_SerialPrintlnV(response);
      ParseBoxes(response);
    }
    else
      d_SerialPrintln("Failed to get active boxes");
  }
}
