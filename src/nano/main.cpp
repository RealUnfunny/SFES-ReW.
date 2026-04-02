#include <Arduino.h>

#include ".common/comm.h"

#include <RF24.h>

RF24 radio(9, 10); // CE, CSN

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

uint64_t my_address; // derived from its box ID, same Addresser logic

FoodCondition evaluateCondition()
{
  int alc_reading = analogRead(ALC_SENSOR_PIN);
  int ch3_reading = analogRead(CH3_SENSOR_PIN);

  if (alc_reading >= ALC_THRESHOLD_BAD || ch3_reading >= CH3_THRESHOLD_BAD)
  {
    digitalWrite(GOING_BAD_PIN, HIGH);
    digitalWrite(FRESH_PIN, LOW);
    digitalWrite(EXPIRED_PIN, LOW);
    return FoodCondition::GOING_BAD;
  }
  else if (alc_reading >= ALC_THRESHOLD_EXP || ch3_reading >= CH3_THRESHOLD_EXP)
  {
    digitalWrite(EXPIRED_PIN, HIGH);
    digitalWrite(GOING_BAD_PIN, LOW);
    digitalWrite(FRESH_PIN, LOW);
    return FoodCondition::EXPIRED;
  }
  else
  {
    digitalWrite(FRESH_PIN, HIGH);
    digitalWrite(EXPIRED_PIN, LOW);
    digitalWrite(GOING_BAD_PIN, LOW);
    return FoodCondition::FRESH;
  }
}

void setup()
{
  // derive address same way as Uno
  my_address = 0xF0F0F0F000LL;
  my_address |= (uint64_t)'A'; // replace with actual BoxID
  my_address |= (uint64_t)'1' << 8;

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.openReadingPipe(1, my_address);
  radio.startListening();
}

void loop()
{
  if (radio.available())
  {
    uint8_t request;
    radio.read(&request, sizeof(request));

    if (request == 0x01)
    {
      FoodCondition condition = evaluateCondition();

      radio.stopListening();
      radio.openWritingPipe(my_address);
      radio.write(&condition, sizeof(condition));
      radio.startListening();
    }
  }
}
