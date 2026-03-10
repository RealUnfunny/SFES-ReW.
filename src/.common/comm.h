#pragma once

// Debug Macros, for printing to the Serial Monitor
// only when 'DEBUG_MODE' is defined
#ifdef DEBUG_MODE

#define d_SerialBegin(x) Serial.begin(x)
#define d_SerialPrint(x) Serial.print(F(x))
#define d_SerialPrintln(x) Serial.println(F(x))
#define d_SerialPrintf(...) Serial.printf(__VA_ARGS__)
#define d_delay(x) delay(x)

#else

#define d_SerialBegin(x)
#define d_SerialPrint(x)
#define d_SerialPrintln(x)
#define d_SerialPrintf(...)
#define d_delay(x)

#endif

// Eponymous.
typedef enum
{
    FRESH,
    GOING_BAD,
    EXPIRED,
    ERROR
} FoodCondition;

// Wifi Password Pair Type
typedef struct
{
    char ssid[20];
    char pass[20];
} wifi_pass_t;

// Boxes Type
typedef struct
{
    char index[2];
    volatile FoodCondition State;
} box_t;
