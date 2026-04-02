#include "CsvOperations.h"

#include "NtfyNotifier.h"
#include <Arduino.h>
#include <CSV_Parser.h> // screw you clang, ignoring blatant error
#include <FS.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <cstdint>
#include <sys/select.h>
#include <sys/types.h>

#include ".common/comm.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

int record_count = 0;

CSV_Parser parser("ucssuLuLcc", true, ',');
// The .csv will have data stored in the following format:
// ->   serial_no, box_id, item_name, reg_date, expiry_date, physical, condition;
// The first is an unsigned 8 bit int(s), next two are strings(s), next
// two are unsigned long(s), while physical is a boolean, so it can be
// interpret as a character
CSV_Parser physical_parser("-s---uc-", true, ',');
// '-' means to ignore that column, thus saving memory.
CSV_Parser serial_parser("uc------", true, ',');
CSV_Parser cp("-ssuLuLcc", true, ',');

void LoadPhysicals()
{
  d_SerialPrintln("Data requested! 2");
  if (physical_parser.readSDfile(INVENTORY_FILE))
  {
    d_SerialPrintln("File read!");
  }
  else
  {
    d_SerialPrintln("Whoops! File does not exist!");
  }
}

void WritePhysicals(SoftwareSerial *sender)
{
  char **box_id = (char **)physical_parser["box_id"];
  uint8_t *physical = (uint8_t *)physical_parser["physical"];
  record_count = physical_parser.getRowsCount();
  d_SerialPrintf("Record count: %d\n", record_count);

  for (int i = 0; i < record_count; i++)
  {
    d_SerialPrintf("Record %d: %s, physical: %d\n", i, box_id[i], physical[i]);
    if (physical[i])
    {
      sender->print(box_id[i]);
      sender->print(",");
    }
  }
  sender->print(MSG_TERMINATOR);
  d_SerialPrintln("Done sending!");
}

void SaveInventory(InventoryRecordLine *records)
{
  SD.remove(INVENTORY_FILE);
  File f = SD.open(INVENTORY_FILE, FILE_WRITE);
  if (!f)
    return;

  f.println("serial_no,box_id,item_name,reg_date,expiry_date,physical");

  for (int i = 0; i < record_count; i++)
  {
    f.print(i + 1);
    f.print(',');
    f.print(records[i].box_id);
    f.print(',');
    f.print(records[i].item_name);
    f.print(',');
    f.print(records[i].reg_date);
    f.print(',');
    f.print(records[i].expiry_date);
    f.print(',');
    f.println(records[i].physical ? "1" : "0");
  }
  f.close();
}

void AddRecord(InventoryRecordLine *record)
{
  if (!serial_parser.readSDfile(INVENTORY_FILE))
    return;

  uint8_t *serials = (uint8_t *)serial_parser["serial_no"];
  int count = serial_parser.getRowsCount();
  uint8_t new_serial = (count > 0) ? serials[count - 1] + 1 : 0;

  File f = SD.open(INVENTORY_FILE, FILE_WRITE);
  if (!f)
    return;

  f.print(new_serial);
  f.print(',');
  f.print(record->box_id);
  f.print(',');
  f.print(record->item_name);
  f.print(',');
  f.print(record->reg_date);
  f.print(',');
  f.print(record->expiry_date);
  f.print(',');
  f.print(record->physical ? 1 : 0);
  f.print(',');
  f.println(record->condition);
  f.close();

  String msg = String("**") + record->item_name + "** has been added to box **" + record->box_id + "**.";
  NtfyNotify("white_check_mark,package", "Item Added", msg.c_str(), "default");
}

void DeleteRecord(uint8_t serial_no)
{
  bool offset = 0;
  if (!cp.readSDfile(INVENTORY_FILE))
    return;

  uint8_t *serials = (uint8_t *)cp["serial_no"];
  char **box_id = (char **)cp["box_id"];
  char **item_name = (char **)cp["item_name"];
  uint32_t *reg_date = (uint32_t *)cp["reg_date"];
  uint32_t *expiry_date = (uint32_t *)cp["expiry_date"];
  char *physical = (char *)cp["physical"];
  char *condition = (char *)cp["condition"];
  int count = cp.getRowsCount();

  SD.remove(INVENTORY_FILE);
  File f = SD.open(INVENTORY_FILE, FILE_WRITE);
  if (!f)
    return;

  f.println("serial_no,box_id,item_name,reg_date,expiry_date,physical,condition");

  for (int i = 0; i < count; i++)
  {
    if (serials[i] == serial_no)
    {
      offset = 1;
      continue; // skip this record
    }
    f.print(i - offset);
    f.print(',');
    f.print(box_id[i]);
    f.print(',');
    f.print(item_name[i]);
    f.print(',');
    f.print(reg_date[i]);
    f.print(',');
    f.print(expiry_date[i]);
    f.print(',');
    f.print(physical[i] ? "1" : "0");
    f.print(',');
    f.println(condition[i]);
  }
  f.close();

  String msg = String("Item with serial **") + serial_no + "** has been deleted from inventory.";
  NtfyNotify("wastebasket", "Item Deleted", msg.c_str(), "default");
}

void CleanCSV()
{
  if (!cp.readSDfile(INVENTORY_FILE))
    return;

  char **ids = (char **)cp["box_id"];
  char **items = (char **)cp["item_name"];
  uint32_t *dateRegs = (uint32_t *)cp["reg_date"];
  uint32_t *expiries = (uint32_t *)cp["expiry_date"];
  uint8_t *physicals = (uint8_t *)cp["physical"];
  uint8_t *condition = (uint8_t *)cp["condition"];
  int count = cp.getRowsCount();

  SD.remove(INVENTORY_FILE);
  File f = SD.open(INVENTORY_FILE, FILE_WRITE);
  if (!f)
    return;

  f.println("serial_no,box_id,item_name,reg_date,expiry_date,physical");

  for (int i = 0; i < count; i++)
  {
    f.print(i + 1);
    f.print(',');
    f.print(ids[i]);
    f.print(',');
    f.print(items[i]);
    f.print(',');
    f.print(dateRegs[i]);
    f.print(',');
    f.print(expiries[i]);
    f.print(',');
    f.println(physicals[i] ? "1" : "0");
  }
  f.close();
}

void CSVUpdate(String csv_data)
{
  CSV_Parser cp(/*format*/ "ucssuLuLcc");
  if (!cp.readSDfile(INVENTORY_FILE))
    return;

  uint8_t *serials = (uint8_t *)cp["serial_no"];
  char **ids = (char **)cp["box_id"];
  char **items = (char **)cp["item_name"];
  uint32_t *dateRegs = (uint32_t *)cp["reg_date"];
  uint32_t *expiries = (uint32_t *)cp["expiry_date"];
  char *physicals = (char *)cp["physical"];
  char *conditions = (char *)cp["condition"];
  int count = cp.getRowsCount();

  // parse incoming csv_data
  CSV_Parser update_cp(/*format*/ "sc", false, ',');
  update_cp << csv_data.c_str();
  update_cp.parseLeftover();
  char **update_ids = (char **)update_cp[0];
  char *update_conditions = (char *)update_cp[1];
  int update_count = update_cp.getRowsCount();

  SD.remove(INVENTORY_FILE);
  File f = SD.open(INVENTORY_FILE, FILE_WRITE);
  if (!f)
    return;

  f.println("serial_no,box_id,item_name,reg_date,expiry_date,physical,condition");

  for (int i = 0; i < count; i++)
  {
    char condition = conditions[i]; // default to existing

    // only bother looking up update if it has a physical unit
    if (physicals[i])
    {
      for (int j = 0; j < update_count; j++)
      {
        if (strcmp(ids[i], update_ids[j]) == 0)
        {
          condition = update_conditions[j];
          break;
        }
      }
    }

    f.print(serials[i]);
    f.print(',');
    f.print(ids[i]);
    f.print(',');
    f.print(items[i]);
    f.print(',');
    f.print(dateRegs[i]);
    f.print(',');
    f.print(expiries[i]);
    f.print(',');
    f.print(physicals[i] ? "1" : "0");
    f.print(',');
    f.println(condition);
  }
  f.close();
}

void CheckAndNotify()
{
  cp = CSV_Parser(/*format*/ "-ss-uLcc");
  if (!cp.readSDfile(INVENTORY_FILE))
    return;

  char **ids = (char **)cp["box_id"];
  char **items = (char **)cp["item_name"];
  uint32_t *expiries = (uint32_t *)cp["expiry_date"];
  char *physicals = (char *)cp["physical"];
  char *conditions = (char *)cp["condition"];
  int count = cp.getRowsCount();

  uint32_t now = time(nullptr);

  for (int i = 0; i < count; i++)
  {
    if (!physicals[i])
      continue;

    if (expiries[i] < now)
    {
      String msg = String("**") + items[i] + "** in box **" + ids[i] + "** has expired!";
      NtfyNotify("warning,skull", "Item Expired!", msg.c_str(), "urgent");
      continue;
    }

    switch (conditions[i])
    {
      case 2:
        {
          String msg = String("**") + items[i] + "** in box **" + ids[i] + "** is expiring soon.";
          NtfyNotify("warning", "Item Expiring", msg.c_str(), "high");
        }
        break;
      case 3:
        {
          String msg = String("**") + items[i] + "** in box **" + ids[i] + "** is rotting!";
          NtfyNotify("warning,skull,biohazard", "Item Rotting!", msg.c_str(), "urgent");
        }
        break;
      case 4:
        {
          String msg = String("Sensor error for box **") + ids[i] + "**.";
          NtfyNotify("warning,x", "Sensor Error", msg.c_str(), "default");
        }
        break;
      default:
        break;
    }
  }
}
