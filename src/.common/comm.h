#pragma once

// Debug Macros, for printing to the Serial Monitor
// only when 'DEBUG_MODE' is defined

#include <stdint.h>

#define MSG_TERMINATOR '^'
#define ReqActBox "RAB^"
#define ReqActBox2 "RAB"
#define UpdBoxes "UPD^"
#define UpdBoxes2 "UPD"
#define INVENTORY_FILE "/inventory.csv"

#define DEBUG_MODE

#ifdef DEBUG_MODE

  #define d_SerialBegin(x) Serial.begin(x)
  #ifdef NODEMCU_FILE
    #define d_SerialPrint(x) Serial.print(x)
    #define d_SerialPrintln(x) Serial.println(x)
  #else
    #define d_SerialPrint(x) Serial.print(F(x))
    #define d_SerialPrintln(x) Serial.println(F(x))
  #endif
  #define d_SerialPrintV(x) Serial.print(x)     // Print Value
  #define d_SerialPrintlnV(x) Serial.println(x) // Print Value
  #define d_SerialPrintf(...) Serial.printf(__VA_ARGS__)
  #define d_delay(x) delay(x)

#else

  #define d_SerialBegin(x)
  #define d_SerialPrint(x)
  #define d_SerialPrintV(x)
  #define d_SerialPrintln(x)
  #define d_SerialPrintlnV(x)
  #define d_SerialPrintf(...)
  #define d_delay(x)

#endif

// Eponymous.
typedef enum
{
  FRESH = 1,
  GOING_BAD = 2,
  EXPIRED = 3,
  ERROR = 4,
  UNKNOWN = 5,
} FoodCondition;

// Wifi Password Pair Type
typedef struct
{
  char ssid[20];
  char pass[20];
} wifi_pass_t;

// Boxes Type
struct box_t
{
  const char *index;
  volatile FoodCondition State;
  uint64_t address;
};

void Addresser(box_t *box);
// Nodes Type

// Request Type

typedef enum
{
  BoxInfo,
  ActiveBoxes
} Requests;
