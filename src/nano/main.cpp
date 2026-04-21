#include ".common/comm.h"

#include <Arduino.h>
#include <RF24.h>
#include <SPI.h>

enum Pins
{
  FRESH_PIN = 8,
  GOING_BAD_PIN = 7,
  EXPIRED_PIN = 6,
  ALC_SENSOR_PIN = A0,
  CH3_SENSOR_PIN = A1
};

RF24 radio(9, 10); // CE, CSN

constexpr uint64_t MAIN_ADDRESS = 0xF0F0F0AA11LL;
threshold_values thresholds;

constexpr uint64_t MakeBoxAddress(const char *index)
{
  return (0xF0F0F00000LL | (uint64_t)index[0]) | ((uint64_t)index[1] << 8);
}

constexpr uint64_t MY_ADDRESS = MakeBoxAddress("A1");

#define PKT_REQUEST 0
#define PKT_RESPONSE 1

void PrintAddress(const char *label, uint64_t addr)
{
  Serial.print(label);
  Serial.print(" 0x");
  for (int i = 4; i >= 0; i--)
  {
    uint8_t b = (addr >> (8 * i)) & 0xFF;
    if (b < 16)
      Serial.print("0");
    Serial.print(b, HEX);
  }
  Serial.println();
}

FoodCondition CurrentCondition = UNKNOWN;

void evaluateCondition()
{
  int alc_reading = analogRead(Pins::ALC_SENSOR_PIN);
  int ch3_reading = analogRead(Pins::CH3_SENSOR_PIN);

  d_SerialPrint("Alcohol Values:");
  d_SerialPrintlnV(alc_reading);
  d_SerialPrint("CH3 Values");
  d_SerialPrintlnV(ch3_reading);

  if (alc_reading >= thresholds.alc_exp || ch3_reading >= thresholds.ch3_exp)
  {
    digitalWrite(Pins::EXPIRED_PIN, HIGH);
    digitalWrite(Pins::FRESH_PIN, LOW);
    digitalWrite(Pins::GOING_BAD_PIN, LOW);
    CurrentCondition = FoodCondition::EXPIRED;
  }
  else if (alc_reading >= thresholds.alc_bad || ch3_reading >= thresholds.ch3_bad)
  {
    digitalWrite(Pins::GOING_BAD_PIN, HIGH);
    digitalWrite(Pins::FRESH_PIN, LOW);
    digitalWrite(Pins::EXPIRED_PIN, LOW);
    CurrentCondition = FoodCondition::GOING_BAD;
  }
  else
  {
    digitalWrite(Pins::FRESH_PIN, HIGH);
    digitalWrite(Pins::GOING_BAD_PIN, LOW);
    digitalWrite(Pins::EXPIRED_PIN, LOW);
    CurrentCondition = FoodCondition::FRESH;
  }
}

void setup()
{
  Serial.begin(9600);

  pinMode(A5, INPUT);
  pinMode(A6, INPUT);

  pinMode(8, OUTPUT); // Fresh Pin
  pinMode(7, OUTPUT); // Going Bad
  pinMode(6, OUTPUT); // Expired

  // digitalWrite(8, HIGH); // Fresh Pin
  // digitalWrite(7, HIGH); // Going Bad
  // digitalWrite(6, HIGH); // Expired

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.setRetries(5, 15);

  radio.openReadingPipe(1, MY_ADDRESS);
  radio.startListening();

  Serial.println("Nano ready");
  PrintAddress("Listening on:", MY_ADDRESS);
  PrintAddress("Replying to :", MAIN_ADDRESS);
  Serial.print("Packet size: ");
  Serial.println(sizeof(Packet));
}

void loop()
{
  if (!radio.available())
    return;

  Packet req;
  radio.read(&req, sizeof(req));

  uint8_t type, rseq;
  FoodCondition cond;
  bool valid, error;
  uint8_t flags;

  if (!UnpackPacket(req, type, rseq, cond, valid, error, flags))
  {
    Serial.println("Nano: bad request packet");
    return;
  }

  thresholds = profile_table[flags];
  evaluateCondition();

  if (type != PKT_REQUEST)
  {
    Serial.println("Nano: not a request");
    return;
  }

  Packet resp;
  PackPacket(resp, PKT_RESPONSE, rseq, CurrentCondition, CurrentCondition != UNKNOWN, false, 0);

  radio.stopListening();
  radio.openWritingPipe(MAIN_ADDRESS);

  delayMicroseconds(150);

  bool ok = radio.write(&resp, sizeof(resp));
  Serial.print("Nano send = ");
  Serial.println(ok ? "OK" : "FAIL");

  radio.openReadingPipe(1, MY_ADDRESS);
  radio.startListening();
}
