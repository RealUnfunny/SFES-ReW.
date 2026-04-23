#pragma once

// Debug Macros, for printing to the Serial Monitor
// only when 'DEBUG_MODE' is defined

#include <stdint.h>

#define MSG_TERMINATOR '^'
#define ReqActBox "RAB"
#define UpdBoxes "UPD"
#define ReqInvCount "IVC"
#define ReqInvWin "IVW"

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
enum FoodCondition : uint8_t
{
  FRESH = 0,
  GOING_BAD = 1,
  EXPIRED = 2,
  UNKNOWN = 3,
};

struct threshold_values
{
  uint16_t alc_bad;
  uint16_t alc_exp;
  uint16_t ch3_bad;
  uint16_t ch3_exp;
};

enum ProfileID : uint8_t
{
  PROFILE_UNKNOWN = 0,
  FRUIT_FAST = 1,
  FRUIT_MEDIUM = 2,
  FRUIT_SLOW = 3,
  EXPERIMENTAL = 4,
  TIME_ONLY = 5
};

// To turn a sensor 'off', simply set the threshold value to 9999.
const threshold_values profile_table[] = {
    {9999, 9999, 9999, 9999}, // UNKNOWN
    {180, 320, 9999, 9999},   // FRUIT_FAST
    {220, 380, 9999, 9999},   // FRUIT_MEDIUM
    {260, 440, 9999, 9999},   // FRUIT_SLOW
    {220, 380, 500, 700},     // EXPERIMENTAL
    {9999, 9999, 9999, 9999}  // TIME_ONLY
};

// Wifi Password Pair Type
typedef struct
{
  char ssid[20];
  char pass[20];
} wifi_pass_t;

// Boxes Type
struct box_t
{
  char index[3];
  volatile FoodCondition State;
  volatile ProfileID p_id;
  uint64_t address;
};

inline void Addresser(box_t *box)
{
  box->address = (0xF0F0F00000LL | (uint64_t)box->index[0]) | ((uint64_t)box->index[1] << 8);
}

typedef enum
{
  BoxInfo,
  ActiveBoxes,
  UpdateBoxes
} Requests;

#define PROTO_VERSION 0x01

enum PacketType : uint8_t
{
  PKT_REQUEST = 0,
  PKT_RESPONSE = 1
};

struct Packet
{
  uint8_t header;
  uint8_t payload;
  uint8_t crc;
};

inline uint8_t MakeHeader(uint8_t type, uint8_t seq)
{
  return ((PROTO_VERSION & 0x01) << 7) | ((type & 0x03) << 5) | (seq & 0x0F);
}

inline uint8_t MakePayload(FoodCondition cond, bool valid, bool error, uint8_t flags)
{
  return ((cond & 0x03) << 6) | ((valid ? 1 : 0) << 5) | ((error ? 1 : 0) << 4) | (flags & 0x0F);
}

inline uint8_t ComputeCRC(uint8_t header, uint8_t payload)
{
  return header ^ payload;
}

inline void PackPacket(Packet &pkt, uint8_t type, uint8_t seq, FoodCondition cond, bool valid, bool error,
                       uint8_t flags)
{
  pkt.header = MakeHeader(type, seq);
  pkt.payload = MakePayload(cond, valid, error, flags);
  pkt.crc = ComputeCRC(pkt.header, pkt.payload);
}

inline bool UnpackPacket(const Packet &pkt, uint8_t &type, uint8_t &seq, FoodCondition &cond, bool &valid, bool &error,
                         uint8_t &flags)
{
  if (ComputeCRC(pkt.header, pkt.payload) != pkt.crc)
    return false;

  uint8_t version = (pkt.header >> 7) & 0x01;
  if (version != PROTO_VERSION)
    return false;

  type = (pkt.header >> 5) & 0x03;
  seq = pkt.header & 0x0F;

  cond = (FoodCondition)((pkt.payload >> 6) & 0x03);
  valid = ((pkt.payload >> 5) & 0x01) != 0;
  error = ((pkt.payload >> 4) & 0x01) != 0;
  flags = pkt.payload & 0x0F;

  return true;
}

inline constexpr uint64_t CalcAddress(const char *index)
{
  return (0xF0F0F0F000LL | (uint64_t)index[0]) | ((uint64_t)index[1] << 8);
}
