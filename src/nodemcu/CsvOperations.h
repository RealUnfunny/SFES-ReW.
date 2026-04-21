#pragma once

#define NODEMCU_FILE

#define INVENTORY_FILE_TEMP "/temp.csv"

#define BUFFER_LIMIT 64
// The total buffer size of a completely
// packed csv line would actually be 48
// bytes, but 64 allows a safe margin and
// is a round number

#include ".common/comm.h"
#include "SoftwareSerial.h"
#include <CSV_Parser.h>
#include <stdint.h>

#include <SD.h>
#include <SPI.h>

struct InventoryRecordLine
{
  uint32_t reg_date;
  uint32_t expiry_date;
  char item_name[16];
  char box_id[3];
  uint8_t notified;
  bool physical;
  bool supported;
  FoodCondition condition;
  ProfileID preset;
};

void WritePhysicals(SoftwareSerial *sender);
void SaveInventory(InventoryRecordLine *records);
void AddRecord(const InventoryRecordLine *record);
void DeleteRecord(const char *target_box_id);
void CleanCSV();
void UpdateCSV(const char *csv_input);
void CheckAndNotify();
