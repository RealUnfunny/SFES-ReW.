#define NODEMCU_FILE

#include ".common/comm.h"
#include "SoftwareSerial.h"
#include <CSV_Parser.h>
#include <stdint.h>

struct InventoryRecordLine
{
  uint32_t reg_date;
  uint32_t expiry_date;
  char item_name[32];
  char box_id[2];
  uint8_t Serial_no;
  bool physical;
  FoodCondition condition;
};

void SDSetup();
void LoadPhysicals();
void WritePhysicals(SoftwareSerial *sender);
void SaveInventory(InventoryRecordLine *records);
void AddRecord(InventoryRecordLine *record);
void DeleteRecord(uint8_t serial_no);
void CleanCSV();
void CSVUpdate(String csv_file);
void CheckAndNotify();
