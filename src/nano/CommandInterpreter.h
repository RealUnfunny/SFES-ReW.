#include ".common/comm.h"

#include <stdint.h>

#define BOX_ID "A1"

constexpr uint64_t MAIN_ADDRESS = 0xF0F0F0AA11LL;
constexpr uint64_t MY_ADDRESS = CalcAddress(BOX_ID);

#include "RF24.h"
#include <Arduino.h>

enum Nano_Pins
{
  FRESH_PIN = 8,
  GOING_BAD_PIN = 7,
  EXPIRED_PIN = 6,
  ALC_SENSOR_PIN = A0,
  CH3_SENSOR_PIN = A1
};

// Prints given address in proper base64 format.
void PrintAddress(uint64_t addr);

// Similar to the UNO, just sets global values on the Nano according to the thresholds.
void EvaluateCondition();

// Simple caller function to set thresholds.
void NanoInit(uint8_t unit_profile);

// Code meant to be called every loop.
void NanoLoop();

// Function to write back to the UNO.
void NanoNotify(RF24 *radio);

// Function meant to handle all radio input.
void CommandInterpreter(RF24 *radio);
