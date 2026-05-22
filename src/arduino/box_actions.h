#include <.common/comm.h>
#include <Arduino.h>
#include <SoftwareSerial.h>

#define MAX_BOXES 2
#define TIMEOUT_MS 500
#define TIME_TO_WAIT_FOR_RESPONSE 250

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

FoodCondition evalLocal();
void RadioSetup();
void ParseBoxes(char *resp);

void Poller();

void NodeMcuUpdate(SoftwareSerial *sender);
