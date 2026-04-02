#define POLL_INTERVAL_MS 5000
#define REFRESH_INT_MS 30000

#include "box_actions.h"
#include "interop_comms.h"
#include <.common/comm.h>
#include <Arduino.h>
#include <SoftwareSerial.h>

unsigned long last_poll = 0;
unsigned long last_refresh = 0;

enum Pins
{
  FRESH_PIN = 2,
  GOING_BAD_PIN = 3,
  EXPIRED_PIN = 4,
  ALC_SENSOR_PIN = A0,
  CH3_SENSOR_PIN = A1
};

enum ThresholdPins // Current Testing Values
{
  ALC_THRESHOLD_BAD = 160,
  ALC_THRESHOLD_EXP = 500,
  CH3_THRESHOLD_BAD = 400,
  CH3_THRESHOLD_EXP = 600
};

unsigned long time = 0;
// const int SENSOR_READ_DELAY = 100;

const int status_pins[] = {Pins::FRESH_PIN, Pins::GOING_BAD_PIN, Pins::EXPIRED_PIN};

int DetermineFoodStatus();
void UpdateStatusDisplay(int Cond);

SoftwareSerial nodemcu(5, 6);

void setup()
{
  Serial.begin(9600);

  for (int pins : status_pins)
    digitalWrite(pins, HIGH);

  delay(1000); // Arbitrary

  for (int pins : status_pins)
    digitalWrite(pins, LOW);

  d_SerialPrintln("SFES Main Indicator Init'sed");

  for (int pins : status_pins)
    pinMode(pins, OUTPUT);

  pinMode(Pins::ALC_SENSOR_PIN, INPUT);
  pinMode(Pins::CH3_SENSOR_PIN, INPUT);

  delay(10000);

  nodemcu.begin(9600);
  RadioSetup();

  String response = "";
  while (!strcmp(response.c_str(), ""))
  {
    d_SerialPrint("Asking for file...");
    response = NodeMCUTransmit(Requests::ActiveBoxes, &nodemcu);
    delay(50);
  }
  ParseBoxes(response);
}

void loop()
{
  unsigned long now = millis();

  if (now - last_poll >= POLL_INTERVAL_MS)
  {
    last_poll = now;
    Poller();
    NodeMCUSender(&nodemcu);
  }
  if (now - last_refresh >= REFRESH_INT_MS)
  {
    last_refresh = now;
    String response = NodeMCUTransmit(Requests::ActiveBoxes, &nodemcu);
    ParseBoxes(response);
  }
}

void UpdateLocalReadings()
{
  int alc_reading = analogRead(Pins::ALC_SENSOR_PIN);
  int ch3_reading = analogRead(Pins::CH3_SENSOR_PIN);

  d_SerialPrint("Sensor Readings:\nAlcohol Readings:");
  d_SerialPrintV(alc_reading);
  d_SerialPrint("| CH3 Readings: ");
  d_SerialPrintV(alc_reading);

  if (alc_reading >= ThresholdPins::ALC_THRESHOLD_EXP || ch3_reading >= CH3_THRESHOLD_EXP)
    digitalWrite(Pins::EXPIRED_PIN, HIGH);
  else if (alc_reading >= ThresholdPins::ALC_THRESHOLD_BAD || ch3_reading >= CH3_THRESHOLD_BAD)
    digitalWrite(Pins::GOING_BAD_PIN, HIGH);
  else
    digitalWrite(Pins::FRESH_PIN, HIGH);
}
