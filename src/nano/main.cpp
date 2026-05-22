#include ".common/comm.h"
#include "CommandInterpreter.h"

#include <Arduino.h>
#include <RF24.h>
#include <SPI.h>

RF24 radio(9, 10); // CE, CSN

void setup()
{
  d_SerialBegin(9600);

  pinMode(Nano_Pins::ALC_SENSOR_PIN, INPUT);
  pinMode(Nano_Pins::CH3_SENSOR_PIN, INPUT);

  pinMode(Nano_Pins::FRESH_PIN, OUTPUT);
  pinMode(Nano_Pins::EXPIRED_PIN, OUTPUT);
  pinMode(Nano_Pins::GOING_BAD_PIN, OUTPUT);

  delay(1000);
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.setRetries(5, 15);
  radio.openReadingPipe(1, MY_ADDRESS);
  radio.startListening();

  d_SerialPrintGroupln(radio.isChipConnected() ? "Radio initialized." : "Radio Uninitiailized. Restart!");
  d_SerialPrintln("Nano ready!");

  d_SerialPrint("My Address:");
  PrintAddress(MY_ADDRESS);
  d_SerialPrint("Main Address:");
  PrintAddress(MAIN_ADDRESS);
  d_SerialPrint("Packet size: ");
  d_SerialPrintlnV(sizeof(Packet));
  delay(1000);

  NanoInit(5); // Time Only, until we get an update from the UNO
}

void loop()
{
  NanoLoop();

  CommandInterpreter(&radio);
}
