#include "box_actions.h"

#include ".common/comm.h"

#include <Arduino.h>
#include <RF24.h>
#include <SoftwareSerial.h>
#include <stdint.h>

RF24 radio(9, 10);

box_t active_boxes[MAX_BOXES];
uint8_t active_box_count = 0;

enum Pins2
{
  FRESH_PIN = 2,
  GOING_BAD_PIN = 3,
  EXPIRED_PIN = 4,
  ALC_SENSOR_PIN = A0,
  CH3_SENSOR_PIN = A1
};

enum ThresholdPins2 // Current Testing Values
{
  ALC_THRESHOLD_BAD = 160,
  ALC_THRESHOLD_EXP = 500,
  CH3_THRESHOLD_BAD = 400,
  CH3_THRESHOLD_EXP = 600
};

FoodCondition evalLocal()
{
  int alc_reading = analogRead(Pins2::ALC_SENSOR_PIN);
  int ch3_reading = analogRead(Pins2::CH3_SENSOR_PIN);

  if (alc_reading >= ThresholdPins2::ALC_THRESHOLD_BAD || ch3_reading >= CH3_THRESHOLD_BAD)
    return FoodCondition::GOING_BAD;
  else if (alc_reading >= ThresholdPins2::ALC_THRESHOLD_EXP || ch3_reading >= CH3_THRESHOLD_EXP)
    return FoodCondition::EXPIRED;
  else
    return FoodCondition::FRESH;
}

void RadioSetup()
{
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
}

void ParseBoxes(String resp)
{
  active_boxes[0].index = "A0";
  active_boxes[0].State = FoodCondition::UNKNOWN;
  Addresser(&active_boxes[0]);

  active_box_count = 1;
  int start = 1;

  for (int i = 0; i <= resp.length(); i++)
  {
    if (resp[i] == ',' or i == resp.length())
    {
      String id = resp.substring(start, i);
      id.trim();

      if (id.length() > 0 && active_box_count < MAX_BOXES)
      {
        active_boxes[active_box_count].index = id.c_str();
        active_boxes[active_box_count].State = FoodCondition::UNKNOWN;
        Addresser(&active_boxes[active_box_count]);
        active_box_count++;
      }
      start = i + 1;
    }
  }
}

void Poller()
{
  active_boxes[0].State = evalLocal();

  for (int i = 1; i < active_box_count; i++)
  {
    radio.openWritingPipe(active_boxes[i].address);
    radio.stopListening();

    uint8_t request = 0x01;
    bool sent = radio.write(&request, sizeof(request));
    if (!sent)
    {
      d_SerialPrint("No Response from");
      d_SerialPrintV(active_boxes[i].index);
      active_boxes[i].State = FoodCondition::UNKNOWN;
      continue;
    }

    radio.openReadingPipe(1, active_boxes[i].address);
    radio.startListening();

    unsigned long start = millis();

    while (!radio.available())
    {
      if (millis() - start > TIMEOUT_MS)
      {
        d_SerialPrint("Timeout: ");
        d_SerialPrintV(active_boxes[i].index);
        break;
      }
    }
    if (radio.available())
    {
      FoodCondition condition;
      radio.read(&condition, sizeof(condition));
      active_boxes[i].State = condition;
    }

    radio.stopListening();
  }
}

void NodeMCUSender(SoftwareSerial *sender)
{
  sender->print(UpdBoxes);
  delay(50);
  String msg = sender->readStringUntil(MSG_TERMINATOR);
  if (!strcmp(msg.c_str(), "Ok."))
  {
    for (int i = 0; i < active_box_count; i++)
    {
      sender->print(active_boxes[i].index);
      sender->print(',');
      sender->print((uint8_t)active_boxes[i].State);
      sender->print(',');
    }
    sender->print(MSG_TERMINATOR);
  }
}
