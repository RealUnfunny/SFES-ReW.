#include ".common/comm.h"
#include <Arduino.h>
#include <SoftwareSerial.h>

enum Pins
{
    FRESH_PIN = 2,
    GOING_BAD_PIN = 3,
    EXPIRED_PIN = 4,
    ALC_SENSOR_PIN = A0,
    CH3_SENSOR_PIN = A1
};

enum ThresholdPins
{
    ALC_THRESHOLD_BAD = 160,
    ALC_THRESHOLD_EXP = 500,
    CH3_THRESHOLD_BAD = 400,
    CH3_THRESHOLD_EXP = 600
};

unsigned long time = 0;
// const int SENSOR_READ_DELAY = 100;
const int status_pins[] = {Pins::FRESH_PIN, Pins::GOING_BAD_PIN, Pins::EXPIRED_PIN};

SoftwareSerial nodemcu(5, 6);

int DetermineFoodStatus();
void UpdateStatusDisplay(int Cond);

void setup()
{
    Serial.begin(9600);
    nodemcu.begin(9600);

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
}

void loop()
{
    UpdateStatusDisplay(1);
}

void UpdateStatusDisplay(int Cond)
{
    int alc_reading = analogRead(Pins::ALC_SENSOR_PIN);
    int ch3_reading = analogRead(Pins::CH3_SENSOR_PIN);

    d_SerialPrint("Sensor Readings:\nAlcohol Readings:");
    d_SerialPrint(alc_reading);
    d_SerialPrint("| CH3 Readings: ");
    d_SerialPrint(alc_reading);

    if (alc_reading >= ThresholdPins::ALC_THRESHOLD_EXP || ch3_reading >= CH3_THRESHOLD_EXP)
        digitalWrite(Pins::EXPIRED_PIN, HIGH);
    else if (alc_reading >= ThresholdPins::ALC_THRESHOLD_BAD || ch3_reading >= CH3_THRESHOLD_BAD)
        digitalWrite(Pins::GOING_BAD_PIN, HIGH);
    else
        digitalWrite(Pins::FRESH_PIN, HIGH);
}
