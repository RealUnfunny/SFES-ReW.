#define NODEMCU_FILE

#pragma once

#define PRIORITY_DEFAULT "3"
#define PRIORITY_URGENT "5"

#include "CsvOperations.h"

void NtfyNotify(const char *tags, const char *title, const char *message, const char *priority);
void NotifyItemAdded(const InventoryRecordLine *record);
void NotifyItemDeleted(const char *box_id);
