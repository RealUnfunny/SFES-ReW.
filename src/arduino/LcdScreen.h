#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>

void UiSetup(SoftwareSerial *sender);
void UiLoop(SoftwareSerial *sender);

bool InventoryBootFinished();
uint16_t InventoryCount();
uint16_t InventoryWindowStart();
void InventorySetWindowStart(uint16_t start);
void InventoryMoveNext();
void InventoryMovePrev();

char KeypadGetPressedEvent(uint32_t now_ms);
