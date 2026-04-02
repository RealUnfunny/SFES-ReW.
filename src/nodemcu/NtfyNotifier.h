#define NODEMCU_FILE

#pragma once

#define PRIORITY_DEFAULT "3"
#define PRIORITY_URGENT "5"

void NtfyNotify(const char *tags, const char *title, const char *message, const char *priority);
