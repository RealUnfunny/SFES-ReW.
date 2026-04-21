#include "box_actions.h"

#include ".common/comm.h"

#include "interop_comms.h"
#include <Arduino.h>
#include <RF24.h>
#include <SoftwareSerial.h>
#include <stdint.h>

RF24 radio(9, 10);

box_t active_boxes[MAX_BOXES];
uint8_t active_box_count = 0;
threshold_values thresholds;

inline void PrintAddress(const char *label, uint64_t addr)
{
  d_SerialPrintV(label);
  d_SerialPrintV(" 0x");

  for (int i = 4; i >= 0; i--) // print 5 bytes (LSB first in RF24)
  {
    uint8_t byte = (addr >> (8 * i)) & 0xFF;
    if (byte < 16)
      d_SerialPrintV("0");
    d_SerialPrintV(byte);
    d_SerialPrintV(HEX);
  }

  d_SerialPrintlnV("");
}

FoodCondition evalLocal()
{
  int alc_reading = analogRead(Pins::ALC_SENSOR_PIN);
  int ch3_reading = analogRead(Pins::CH3_SENSOR_PIN);

  d_SerialPrint("Alcohol Values:");
  d_SerialPrintlnV(alc_reading);
  d_SerialPrint("CH3 Values");
  d_SerialPrintlnV(ch3_reading);

  if (alc_reading >= thresholds.alc_exp || ch3_reading >= thresholds.ch3_exp)
  {
    digitalWrite(Pins::EXPIRED_PIN, HIGH);
    digitalWrite(Pins::FRESH_PIN, LOW);
    digitalWrite(Pins::GOING_BAD_PIN, LOW);
    return FoodCondition::EXPIRED;
  }
  else if (alc_reading >= thresholds.alc_bad || ch3_reading >= thresholds.ch3_bad)
  {
    digitalWrite(Pins::GOING_BAD_PIN, HIGH);
    digitalWrite(Pins::FRESH_PIN, LOW);
    digitalWrite(Pins::EXPIRED_PIN, LOW);
    return FoodCondition::GOING_BAD;
  }
  else
  {
    digitalWrite(Pins::FRESH_PIN, HIGH);
    digitalWrite(Pins::GOING_BAD_PIN, LOW);
    digitalWrite(Pins::EXPIRED_PIN, LOW);
    return FoodCondition::FRESH;
  }
}

constexpr uint64_t MAIN_ADDRESS = 0xF0F0F0AA11LL;

void RadioSetup()
{
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.setRetries(5, 15);

  radio.openReadingPipe(1, MAIN_ADDRESS);
  radio.startListening();

  d_SerialPrintln("Radio initialized (UNO)");
  PrintAddress("Listening on: ", MAIN_ADDRESS);
}

void ParseBoxes(char *resp)
{
  active_box_count = 0;

  char *token = strtok(resp, ";");

  while (token != NULL && active_box_count < MAX_BOXES)
  {
    while (*token == ' ')
      token++;

    size_t len = strlen(token);
    while (len > 0 && (token[len - 1] == ' ' || token[len - 1] == '\r' || token[len - 1] == '\n'))
    {
      token[len - 1] = '\0';
      len--;
    }

    if (len >= 4)
    {
      char *comma = strchr(token, ',');
      if (comma != NULL)
      {
        *comma = '\0';
        char *valueStr = comma + 1;

        uint8_t value = (uint8_t)atoi(valueStr);

        // fill struct
        active_boxes[active_box_count].index[0] = token[0];
        active_boxes[active_box_count].index[1] = token[1];
        active_boxes[active_box_count].index[2] = '\0';

        active_boxes[active_box_count].p_id = (ProfileID)value;

        // set state AFTER p_id is known
        if (token[0] == 'A' && token[1] == '0')
        {
          thresholds = profile_table[value];
          active_boxes[active_box_count].State = evalLocal();
        }
        else
          active_boxes[active_box_count].State = FoodCondition::UNKNOWN;

        Addresser(&active_boxes[active_box_count]);
        active_box_count++;
      }
    }

    token = strtok(NULL, ";");
  }
}

void ClearRadioRX()
{
  while (radio.available())
  {
    Packet junk;
    radio.read(&junk, sizeof(junk));
  }
}

void Poller()
{
  static int current_box = 1;
  static uint8_t seq = 0;

  if (active_box_count <= 1)
    return;

  if (current_box >= active_box_count)
    current_box = 1;

  uint8_t my_seq = seq;

  Packet req;
  PackPacket(req, PKT_REQUEST, my_seq, UNKNOWN, false, false, active_boxes[current_box].p_id);

  d_SerialPrint("Polling box: ");
  d_SerialPrintlnV(active_boxes[current_box].index);

  radio.stopListening();
  ClearRadioRX();
  radio.openWritingPipe(active_boxes[current_box].address);

  bool ok = radio.write(&req, sizeof(req));
  d_SerialPrint("Poller write = ");
  d_SerialPrintlnV(ok ? "OK" : "FAIL");

  radio.openReadingPipe(1, MAIN_ADDRESS);
  radio.startListening();

  unsigned long start = millis();
  bool got_matching_response = false;

  while (millis() - start < 250)
  {
    if (!radio.available())
      continue;

    Packet resp;
    radio.read(&resp, sizeof(resp));

    uint8_t type, rseq;
    FoodCondition cond;
    bool valid, error;
    uint8_t flags;

    if (!UnpackPacket(resp, type, rseq, cond, valid, error, flags))
    {
      d_SerialPrintln("Poller: bad response packet");
      continue;
    }

    if (type != PKT_RESPONSE)
    {
      d_SerialPrintln("Poller: wrong response type");
      continue;
    }

    if (rseq != my_seq)
    {
      d_SerialPrint("Poller: seq mismatch, got ");
      d_SerialPrintV(rseq);
      d_SerialPrint(" expected ");
      d_SerialPrintlnV(my_seq);
      continue;
    }

    if (!valid || error)
    {
      d_SerialPrintln("Poller: response invalid/error");
      active_boxes[current_box].State = FoodCondition::UNKNOWN;
    }
    else
    {
      d_SerialPrintln("Poller: valid response");
      active_boxes[current_box].State = cond;
    }

    got_matching_response = true;
    break;
  }

  if (!got_matching_response)
  {
    d_SerialPrintln("Poller: response timeout");
    active_boxes[current_box].State = FoodCondition::UNKNOWN;
  }

  radio.stopListening();
  radio.openReadingPipe(1, MAIN_ADDRESS);
  radio.startListening();

  current_box++;
  seq = (seq + 1) & 0x0F;
}

void NodeMcuUpdate(SoftwareSerial *sender)
{
  char csv_data[120];
  size_t idx = 0;

  for (int i = 0; i < active_box_count; i++)
  {
    const char *index = active_boxes[i].index;
    uint8_t state = (uint8_t)active_boxes[i].State;

    int written = snprintf(csv_data + idx, sizeof(csv_data) - idx, "%s,%d\n", index, state);

    if (written < 0)
      break;

    idx += (size_t)written;

    if (idx >= sizeof(csv_data))
    {
      idx = sizeof(csv_data) - 1;
      break;
    }
  }

  csv_data[idx] = '\0';

  char dummy[16] = {0};

  if (!NodeMCUTransmit(Requests::UpdateBoxes, sender, csv_data, dummy, sizeof(dummy)))
  {
    d_SerialPrintln("Update failed");
  }
}
