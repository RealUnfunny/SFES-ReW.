#include "LcdScreen.h"

#include <.common/comm.h>
#include <Wire.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>

static constexpr uint8_t PCF8575_ADDR = 0x20;
static constexpr uint8_t LCD_COLS = 16;
static constexpr uint32_t INTRO_DURATION_MS = 3000UL;
static constexpr uint32_t BOOT_ANIM_INTERVAL_MS = 220UL;
static constexpr uint32_t LOADING_ANIM_INTERVAL_MS = 180UL;

enum RefereshPhase : uint8_t
{
  REFRESH_IDLE,
  REFRESH_WAIT_COUNT,
  REFRESH_WAIT_WINDOW
};

namespace KeypadPins
{
constexpr uint8_t R1 = 8;
constexpr uint8_t R2 = 9;
constexpr uint8_t R3 = 10;
constexpr uint8_t R4 = 11;
constexpr uint8_t C1 = 12;
constexpr uint8_t C2 = 13;
constexpr uint8_t C3 = 14;
constexpr uint8_t C4 = 15;
} // namespace KeypadPins

static const char KEYMAP[4][4] = {
    {'1', '2', '3', 'A'}, {'4', '5', '6', 'B'}, {'7', '8', '9', 'C'}, {'*', '0', '#', 'D'}};

namespace Pins
{
constexpr uint8_t LCD_RS = 0;
constexpr uint8_t LCD_E = 1;
constexpr uint8_t LCD_D4 = 2;
constexpr uint8_t LCD_D5 = 3;
constexpr uint8_t LCD_D6 = 4;
constexpr uint8_t LCD_D7 = 5;
constexpr uint8_t LCD_BL = 6;
} // namespace Pins

enum UiScreen : uint8_t
{
  SCREEN_INTRO,
  SCREEN_BOOTING,
  SCREEN_INVENTORY
};

static UiScreen g_screen = SCREEN_INTRO;
static bool g_dirty = true;

static uint32_t g_last_loading_anim_ms = 0;
static uint8_t g_loading_dots = 0;
static uint8_t g_loading_spinner_idx = 0;
static uint8_t g_loading_emote_index = 0;

static uint32_t g_screen_started_ms = 0;
static uint32_t g_last_boot_anim_ms = 0;
static uint8_t g_boot_emote_index = 0;
static uint8_t g_boot_punct_phase = 0;

static bool g_inventory_boot_started = false;
static bool g_inventory_ready = false;
static bool g_inventory_window_pending = false;

static uint16_t g_inventory_count = 0;
static uint16_t g_inventory_window_start = 0;
static uint16_t g_current_window_start = 0xFFFFu;

static char g_row0_name[17] = {0};
static char g_row0_condition[12] = {0};
static char g_row0_box[3] = {0};
static bool g_row0_valid = false;

static char g_row1_name[17] = {0};
static char g_row1_condition[12] = {0};
static char g_row1_box[3] = {0};
static bool g_row1_valid = false;

static char g_held_key = 0;
static uint32_t g_hold_start_ms = 0;
static uint32_t g_last_repeat_ms = 0;

static uint32_t g_last_ivc_req_ms = 0;
static const uint32_t IVC_RETRY_MS = 500;
static bool g_boot_request_started = false;

static RefereshPhase g_refresh_phase = REFRESH_IDLE;
static uint32_t g_last_inventory_refresh_ms = 0;
static const uint32_t INVENTORY_REFRESH_MS = 3000;

static uint16_t g_pcf_shadow = 0xFFFFu;

static const char E0[] PROGMEM = "(^_^)";
static const char E1[] PROGMEM = "(-_-)";
static const char E2[] PROGMEM = "(o_o)";
static const char E3[] PROGMEM = "(>_<)";
static const char E4[] PROGMEM = "(^o^)/";
static const char E5[] PROGMEM = "(._.)";
static const char E6[] PROGMEM = "\\(^_^)/";
static const char E7[] PROGMEM = "('u')";
static const char E8[] PROGMEM = "(^.^)";
static const char E9[] PROGMEM = "(^_~)";

static const char *const BOOT_EMOTES[] PROGMEM = {E0, E1, E2, E3, E4, E5, E6, E7, E8, E9};
static constexpr uint8_t BOOT_EMOTE_COUNT = static_cast<uint8_t>(sizeof(BOOT_EMOTES) / sizeof(BOOT_EMOTES[0]));
static const char LOADING_SPINNER[4] = {'|', '/', '-', '\\'};

static void KeypadReleaseAll();
static void InventoryLoadVisibleWindow(SoftwareSerial *sender, uint16_t start);

static uint16_t InventoryMaxWindowStart()
{
  if (g_inventory_count == 0)
    return 0;

  return static_cast<uint16_t>(g_inventory_count - 1);
}

static uint16_t GetRepeatInterval(uint32_t held_ms)
{
  if (held_ms < 300)
    return 200;
  if (held_ms < 1000)
    return 120;
  if (held_ms < 2000)
    return 70;
  return 40;
}

static void PcfWrite16(uint16_t value)
{
  g_pcf_shadow = value;

  Wire.beginTransmission(PCF8575_ADDR);
  Wire.write(static_cast<uint8_t>(value & 0xFFu));
  Wire.write(static_cast<uint8_t>((value >> 8) & 0xFFu));
  Wire.endTransmission();
}

static uint16_t PcfRead16()
{
  Wire.requestFrom(static_cast<int>(PCF8575_ADDR), 2);

  uint8_t low = 0xFF;
  uint8_t high = 0xFF;

  if (Wire.available())
    low = static_cast<uint8_t>(Wire.read());
  if (Wire.available())
    high = static_cast<uint8_t>(Wire.read());

  return static_cast<uint16_t>(low | (static_cast<uint16_t>(high) << 8));
}

static uint16_t PcfShadow()
{
  return g_pcf_shadow;
}

static void PcfWriteMasked(uint16_t mask, uint16_t value)
{
  g_pcf_shadow = static_cast<uint16_t>((g_pcf_shadow & ~mask) | (value & mask));
  PcfWrite16(g_pcf_shadow);
}

static void PcfSetPin(uint8_t pin, bool high)
{
  const uint16_t mask = static_cast<uint16_t>(1u << pin);

  if (high)
    g_pcf_shadow |= mask;
  else
    g_pcf_shadow &= static_cast<uint16_t>(~mask);

  PcfWrite16(g_pcf_shadow);
}

static uint16_t LcdMask()
{
  return static_cast<uint16_t>((1u << Pins::LCD_RS) | (1u << Pins::LCD_E) | (1u << Pins::LCD_D4) |
                               (1u << Pins::LCD_D5) | (1u << Pins::LCD_D6) | (1u << Pins::LCD_D7) |
                               (1u << Pins::LCD_BL));
}

static void LcdPulseEnable(uint16_t base)
{
  PcfWriteMasked(LcdMask(), static_cast<uint16_t>(base | (1u << Pins::LCD_E)));
  delayMicroseconds(2);
  PcfWriteMasked(LcdMask(), base);
  delayMicroseconds(80);
}

static void LcdSendNibble(uint8_t nibble, bool rs)
{
  uint16_t out = 0;

  if (rs)
    out |= static_cast<uint16_t>(1u << Pins::LCD_RS);
  if (PcfShadow() & (1u << Pins::LCD_BL))
    out |= static_cast<uint16_t>(1u << Pins::LCD_BL);

  if (nibble & 0x01u)
    out |= static_cast<uint16_t>(1u << Pins::LCD_D4);
  if (nibble & 0x02u)
    out |= static_cast<uint16_t>(1u << Pins::LCD_D5);
  if (nibble & 0x04u)
    out |= static_cast<uint16_t>(1u << Pins::LCD_D6);
  if (nibble & 0x08u)
    out |= static_cast<uint16_t>(1u << Pins::LCD_D7);

  PcfWriteMasked(LcdMask(), out);
  LcdPulseEnable(out);
}

static void LcdSendByte(uint8_t value, bool rs)
{
  LcdSendNibble(static_cast<uint8_t>((value >> 4) & 0x0F), rs);
  LcdSendNibble(static_cast<uint8_t>(value & 0x0F), rs);
  delayMicroseconds(40);
}

static void LcdCommand(uint8_t cmd)
{
  LcdSendByte(cmd, false);
}

static void LcdWriteChar(char c)
{
  LcdSendByte(static_cast<uint8_t>(c), true);
}

static void LcdSetBacklight(bool on)
{
  PcfSetPin(Pins::LCD_BL, on);
}

static void LcdBegin()
{
  delay(50);
  LcdSetBacklight(true);

  const uint16_t init_mask = LcdMask();
  const uint16_t init_val = static_cast<uint16_t>(1u << Pins::LCD_BL);
  PcfWriteMasked(init_mask, init_val);

  delay(5);

  LcdSendNibble(0x03, false);
  delay(5);
  LcdSendNibble(0x03, false);
  delayMicroseconds(150);
  LcdSendNibble(0x03, false);
  LcdSendNibble(0x02, false);

  LcdCommand(0x28);
  LcdCommand(0x0C);
  LcdCommand(0x06);
  LcdCommand(0x01);
  delay(3);
}

static void LcdSetCursor(uint8_t col, uint8_t row)
{
  static constexpr uint8_t ROW_OFFSETS[2] = {0x00, 0x40};
  LcdCommand(static_cast<uint8_t>(0x80 | (ROW_OFFSETS[row] + col)));
}

static void LcdPrintLine(uint8_t row, const char *text)
{
  LcdSetCursor(0, row);

  uint8_t i = 0;
  while (text[i] != '\0' && i < LCD_COLS)
  {
    LcdWriteChar(text[i]);
    ++i;
  }

  while (i < LCD_COLS)
  {
    LcdWriteChar(' ');
    ++i;
  }
}

static void LcdPrintCentered(uint8_t row, const char *text)
{
  char centered[LCD_COLS + 1];
  uint8_t len = 0;

  while (text[len] != '\0' && len < LCD_COLS)
  {
    centered[len] = text[len];
    ++len;
  }

  const uint8_t left_pad = static_cast<uint8_t>((LCD_COLS - len) / 2U);
  uint8_t i = 0;

  for (; i < left_pad; ++i)
    centered[i] = ' ';
  for (uint8_t j = 0; j < len && i < LCD_COLS; ++j, ++i)
    centered[i] = text[j];
  while (i < LCD_COLS)
    centered[i++] = ' ';

  centered[LCD_COLS] = '\0';
  LcdPrintLine(row, centered);
}

static void LcdDisplayControl(bool display_on, bool cursor_on, bool blink_on)
{
  uint8_t cmd = 0x08;
  if (display_on)
    cmd |= 0x04;
  if (cursor_on)
    cmd |= 0x02;
  if (blink_on)
    cmd |= 0x01;
  LcdCommand(cmd);
}

static void LcdCursorOff()
{
  LcdDisplayControl(true, false, false);
}

static void SetDirty()
{
  g_dirty = true;
}

static void InventoryClearWindow()
{
  g_row0_name[0] = '\0';
  g_row0_condition[0] = '\0';
  g_row0_box[0] = '\0';
  g_row0_valid = false;

  g_row1_name[0] = '\0';
  g_row1_condition[0] = '\0';
  g_row1_box[0] = '\0';
  g_row1_valid = false;
}

static void CopyField(char *dst, size_t dst_size, const char *src)
{
  if (dst_size == 0)
    return;

  strncpy(dst, src ? src : "", dst_size - 1);
  dst[dst_size - 1] = '\0';
}

static PGM_P BootEmoteAt(uint8_t index)
{
  return (PGM_P)pgm_read_ptr(&BOOT_EMOTES[index % BOOT_EMOTE_COUNT]);
}

static void CopyProgmemString(char *dest, size_t dest_size, PGM_P src)
{
  if (dest_size == 0)
    return;

  strncpy_P(dest, src, dest_size - 1);
  dest[dest_size - 1] = '\0';
}

static void InventoryBeginBoot(SoftwareSerial *sender)
{
  if (sender == nullptr)
    return;

  g_inventory_boot_started = true;
  g_inventory_ready = false;
  g_inventory_window_pending = false;
  g_inventory_count = 0;
  g_current_window_start = 0xFFFFu;
  g_last_ivc_req_ms = 0;
  InventoryClearWindow();
}

static void InventoryRequestCount(SoftwareSerial *sender)
{
  if (sender == nullptr)
    return;
  sender->print("IVC");
  sender->print(MSG_TERMINATOR);
}

static void InventoryPollBoot(SoftwareSerial *sender)
{
  static char buffer[64];
  static size_t idx = 0;

  const bool waiting_for_count = (!g_inventory_ready) || (g_refresh_phase == REFRESH_WAIT_COUNT);

  if (sender == nullptr || !g_inventory_boot_started || !waiting_for_count)
    return;

  const uint32_t now = millis();

  if (!g_inventory_ready && (now - g_last_ivc_req_ms >= IVC_RETRY_MS))
  {
    g_last_ivc_req_ms = now;
    InventoryRequestCount(sender);
    d_SerialPrintln("Retrying IVC...");
  }

  while (sender->available())
  {
    const char c = static_cast<char>(sender->read());

    if (c == MSG_TERMINATOR)
    {
      buffer[idx] = '\0';

      d_SerialPrintln("Boot_RX:");
      d_SerialPrintlnV(buffer);

      if (strncmp(buffer, "IVC,", 4) == 0)
      {
        const uint16_t new_count = static_cast<uint16_t>(atoi(buffer + 4));
        g_inventory_count = new_count;

        if (g_inventory_window_start > InventoryMaxWindowStart())
          g_inventory_window_start = InventoryMaxWindowStart();

        g_inventory_ready = true;
        SetDirty();
        idx = 0;
        return;
      }

      // Not an IVC message — discard and keep reading; don't return so
      // InventoryPollWindow gets a chance to see it next tick.
      idx = 0;
    }

    if (idx < sizeof(buffer) - 1)
      buffer[idx++] = c;
    else
      idx = 0;
  }
}
static void InventoryLoadVisibleWindow(SoftwareSerial *sender, uint16_t start)
{
  if (sender == nullptr)
    return;

  if (start > InventoryMaxWindowStart())
    start = InventoryMaxWindowStart();

  g_inventory_window_start = start;
  g_inventory_window_pending = true;
  InventoryClearWindow();

  sender->print("IVW,");
  sender->print(start);
  sender->print(",2");
  sender->print(MSG_TERMINATOR);
}

static void InventoryPollWindow(SoftwareSerial *sender)
{
  static char buffer[160];
  static size_t idx = 0;

  if (sender == nullptr || !g_inventory_window_pending)
    return;

  while (sender->available())
  {
    const char c = static_cast<char>(sender->read());

    if (c == MSG_TERMINATOR)
    {
      buffer[idx] = '\0';

      // Ignore anything that is not an inventory window reply;
      // do NOT clear pending/phase flags — InventoryPollBoot may still need this message.
      if (strncmp(buffer, "IVW,", 4) != 0)
      {
        idx = 0;
        return;
      }

      InventoryClearWindow();

      char *payload = buffer + 4;
      char *save_line = nullptr;
      char *line = strtok_r(payload, "\n", &save_line);
      uint8_t line_no = 0;

      while (line != nullptr && line_no < 2)
      {
        char *save_field = nullptr;

        char *name_tok = strtok_r(line, "|", &save_field);
        char *condition_tok = strtok_r(nullptr, "|", &save_field);
        char *box_tok = strtok_r(nullptr, "|", &save_field);

        if (name_tok && condition_tok && box_tok)
        {
          if (line_no == 0)
          {
            CopyField(g_row0_name, sizeof(g_row0_name), name_tok);
            CopyField(g_row0_condition, sizeof(g_row0_condition), condition_tok);
            CopyField(g_row0_box, sizeof(g_row0_box), box_tok);
            g_row0_valid = true;
          }
          else
          {
            CopyField(g_row1_name, sizeof(g_row1_name), name_tok);
            CopyField(g_row1_condition, sizeof(g_row1_condition), condition_tok);
            CopyField(g_row1_box, sizeof(g_row1_box), box_tok);
            g_row1_valid = true;
          }
        }

        line = strtok_r(nullptr, "\n", &save_line);
        ++line_no;
      }

      // Even if no rows were parsed, this still counts as a valid
      // inventory-window response, so stop showing "Loading..."
      g_inventory_window_pending = false;
      g_current_window_start = g_inventory_window_start;
      g_refresh_phase = REFRESH_IDLE;
      SetDirty();

      idx = 0;
      return;
    }

    if (idx < sizeof(buffer) - 1)
      buffer[idx++] = c;
    else
      idx = 0;
  }
}

static void DrawIntroScreen()
{
  LcdPrintCentered(0, "sfes! v0.3");
  LcdPrintCentered(1, "by aryan");
}

static void DrawBootingScreen()
{
  char emote_buf[LCD_COLS + 1] = {0};
  char boot_line[LCD_COLS + 1] = {0};
  char suffix[9] = "........";

  for (uint8_t i = 0; i < g_boot_punct_phase && i < 8; ++i)
    suffix[i] = ',';

  snprintf(boot_line, sizeof(boot_line), "Booting%s", suffix);
  CopyProgmemString(emote_buf, sizeof(emote_buf), BootEmoteAt(g_boot_emote_index));

  LcdPrintLine(0, boot_line);
  LcdPrintCentered(1, emote_buf);
}

static const char *CondToStr(uint8_t cond)
{
  switch (cond)
  {
    case 0:
      return "Fr"; // FRESH
    case 1:
      return "Bd "; // GOING_BAD
    case 2:
      return "Ex "; // EXPIRED
    default:
      return "Un "; // UNKNOWN
  }
}

static void FormatName8(char *out, size_t out_size, const char *name)
{
  if (out_size == 0)
    return;

  size_t len = strlen(name);

  if (len <= 8)
  {
    strncpy(out, name, out_size - 1);
    out[out_size - 1] = '\0';
    return;
  }

  // 5 chars + "..."
  strncpy(out, name, 5);
  out[5] = '.';
  out[6] = '.';
  out[7] = '.';
  out[8] = '\0';
}

static void FormatInventoryName(const char *src, char *dst, size_t dst_size)
{
  if (dst_size == 0)
    return;

  dst[0] = '\0';

  if (src == nullptr || src[0] == '\0')
    return;

  const size_t len = strlen(src);

  if (len <= 8)
  {
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
    return;
  }

  if (dst_size >= 9)
  {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = '.';
    dst[3] = '.';
    dst[4] = '.';
    dst[5] = src[len - 3];
    dst[6] = src[len - 2];
    dst[7] = src[len - 1];
    dst[8] = '\0';
  }
}

static void DrawInventoryScreen()
{
  char line0[17] = {0};
  char line1[17] = {0};

  // Blinking toggle (~500ms)
  const bool blink_on = ((millis() / 500) % 2) == 0;

  if (g_inventory_window_pending)
  {
    char loading_line[17] = {0};
    char emote_buf[17] = {0};

    snprintf(loading_line, sizeof(loading_line), "Loading%.*s %c", g_loading_dots, "....",
             LOADING_SPINNER[g_loading_spinner_idx % 4]);

    CopyProgmemString(emote_buf, sizeof(emote_buf), BootEmoteAt(g_loading_emote_index));

    LcdPrintCentered(0, loading_line);
    LcdPrintCentered(1, emote_buf);
    return;
  }

  if (g_row0_valid)
  {
    char name0[9] = {0};
    char cond_buf[3] = {0};
    const uint8_t cond0 = static_cast<uint8_t>(atoi(g_row0_condition));

    FormatInventoryName(g_row0_name, name0, sizeof(name0));
    strncpy(cond_buf, CondToStr(cond0), 2);
    cond_buf[2] = '\0';

    // Blink for bad/expired
    if ((cond0 == 1 || cond0 == 2) && !blink_on)
      strcpy(cond_buf, "  ");

    snprintf(line0, sizeof(line0), "%-8s %2.2s|%2.2s", name0, g_row0_box, cond_buf);
  }
  else
  {
    if (g_inventory_window_start >= g_inventory_count)
      snprintf(line0, sizeof(line0), "EOL");
    else
      snprintf(line0, sizeof(line0), "--");
  }

  if (g_row1_valid)
  {
    char name1[9] = {0};
    char cond_buf[3] = {0};
    const uint8_t cond1 = static_cast<uint8_t>(atoi(g_row1_condition));

    FormatInventoryName(g_row1_name, name1, sizeof(name1));
    strncpy(cond_buf, CondToStr(cond1), 2);
    cond_buf[2] = '\0';

    if ((cond1 == 1 || cond1 == 2) && !blink_on)
      strcpy(cond_buf, "  ");

    snprintf(line1, sizeof(line1), "%-8s %2.2s|%2.2s", name1, g_row1_box, cond_buf);
  }
  else
  {
    if ((g_inventory_window_start + 1) >= g_inventory_count)
      snprintf(line1, sizeof(line1), "EOL");
    else
      snprintf(line1, sizeof(line1), "--");
  }

  LcdPrintLine(0, line0);
  LcdPrintLine(1, line1);
}

static void DrawUi()
{
  if (!g_dirty)
    return;

  LcdCursorOff();

  switch (g_screen)
  {
    case SCREEN_INTRO:
      DrawIntroScreen();
      break;
    case SCREEN_BOOTING:
      DrawBootingScreen();
      break;
    case SCREEN_INVENTORY:
      DrawInventoryScreen();
      break;
  }

  g_dirty = false;
}

static void UpdateBootState(uint32_t now_ms, SoftwareSerial *sender)
{
  switch (g_screen)
  {
    case SCREEN_INTRO:
      if ((now_ms - g_screen_started_ms) >= INTRO_DURATION_MS)
      {
        g_screen = SCREEN_BOOTING;
        g_screen_started_ms = now_ms;
        g_last_boot_anim_ms = now_ms;
        SetDirty();
      }
      break;

    case SCREEN_BOOTING:
      if ((now_ms - g_last_boot_anim_ms) >= BOOT_ANIM_INTERVAL_MS)
      {
        g_last_boot_anim_ms = now_ms;
        g_boot_emote_index = static_cast<uint8_t>((g_boot_emote_index + 1U) % BOOT_EMOTE_COUNT);
        g_boot_punct_phase = static_cast<uint8_t>((g_boot_punct_phase + 1U) % 9U);
        SetDirty();
      }

      if (InventoryBootFinished())
      {
        g_screen = SCREEN_INVENTORY;
        g_screen_started_ms = now_ms;
        g_inventory_window_start = 0;
        g_current_window_start = 0xFFFFu;
        InventoryLoadVisibleWindow(sender, 0);
        SetDirty();
      }
      break;

    case SCREEN_INVENTORY:
      break;
  }
}

static void I2CRecoverBus()
{
  // Toggle SCL 9 times to unlock any stuck I2C slave before Wire.begin()
  pinMode(SDA, OUTPUT);
  digitalWrite(SDA, HIGH);
  pinMode(SCL, OUTPUT);

  for (uint8_t i = 0; i < 9; ++i)
  {
    digitalWrite(SCL, LOW);
    delayMicroseconds(5);
    digitalWrite(SCL, HIGH);
    delayMicroseconds(5);
  }

  // Issue a STOP condition
  digitalWrite(SDA, LOW);
  delayMicroseconds(5);
  digitalWrite(SDA, HIGH);
  delayMicroseconds(5);
}

void UiSetup(SoftwareSerial *sender)
{
  (void)sender;

  I2CRecoverBus();
  Wire.begin();
  Wire.setClock(50000);

  g_last_inventory_refresh_ms = millis();

  PcfWrite16(g_pcf_shadow);
  LcdBegin();

  g_screen = SCREEN_INTRO;
  g_dirty = true;
  g_screen_started_ms = millis();
  g_last_boot_anim_ms = g_screen_started_ms;
  g_boot_emote_index = 0;
  g_boot_punct_phase = 0;

  g_inventory_boot_started = false;
  g_inventory_ready = false;
  g_inventory_window_pending = false;
  g_inventory_count = 0;
  g_inventory_window_start = 0;
  g_current_window_start = 0xFFFFu;
  g_boot_request_started = false;
  g_last_loading_anim_ms = g_screen_started_ms;
  g_loading_dots = 0;
  g_loading_spinner_idx = 0;
  g_loading_emote_index = 0;

  InventoryClearWindow();
  DrawUi();
}

static void KeypadReleaseAll()
{
  PcfSetPin(KeypadPins::R1, true);
  PcfSetPin(KeypadPins::R2, true);
  PcfSetPin(KeypadPins::R3, true);
  PcfSetPin(KeypadPins::R4, true);
  PcfSetPin(KeypadPins::C1, true);
  PcfSetPin(KeypadPins::C2, true);
  PcfSetPin(KeypadPins::C3, true);
  PcfSetPin(KeypadPins::C4, true);
}

static char KeypadScanRaw()
{
  const uint8_t rows[4] = {KeypadPins::R1, KeypadPins::R2, KeypadPins::R3, KeypadPins::R4};
  const uint8_t cols[4] = {KeypadPins::C1, KeypadPins::C2, KeypadPins::C3, KeypadPins::C4};

  for (uint8_t r = 0; r < 4; ++r)
  {
    KeypadReleaseAll();
    PcfSetPin(rows[r], false);
    delayMicroseconds(20);

    const uint16_t state = PcfRead16();

    for (uint8_t c = 0; c < 4; ++c)
    {
      if ((state & (1u << cols[c])) == 0)
      {
        KeypadReleaseAll();
        return KEYMAP[r][c];
      }
    }
  }

  KeypadReleaseAll();
  return 0;
}

char KeypadGetPressedEvent(uint32_t now_ms)
{
  static char last_raw = 0;
  static char stable_key = 0;
  static uint32_t last_change_time = 0;
  static constexpr uint32_t DEBOUNCE_MS = 30;

  const char raw = KeypadScanRaw();

  if (raw != last_raw)
  {
    last_raw = raw;
    last_change_time = now_ms;
  }

  if ((now_ms - last_change_time) >= DEBOUNCE_MS)
  {
    if (stable_key != raw)
    {
      stable_key = raw;
      if (stable_key != 0)
        return stable_key;
    }
  }

  return 0;
}

void UiLoop(SoftwareSerial *sender)
{
  const uint32_t now_ms = millis();

  if (!g_boot_request_started)
  {
    InventoryBeginBoot(sender);
    g_boot_request_started = true;
  }

  if (!InventoryBootFinished())
  {
    InventoryPollBoot(sender);
  }
  else
  {
    char press = KeypadGetPressedEvent(now_ms);
    char raw = KeypadScanRaw();

    // Input only when NOT waiting for window
    if (g_refresh_phase == REFRESH_IDLE)
    {
      if (press)
      {
        g_held_key = press;
        g_hold_start_ms = now_ms;
        g_last_repeat_ms = now_ms;

        if (press == '2')
        {
          if (g_inventory_window_start > 0)
          {
            g_inventory_window_start--;
            SetDirty();
          }
        }
        else if (press == '8')
        {
          if (g_inventory_window_start < InventoryMaxWindowStart())
          {
            g_inventory_window_start++;
            SetDirty();
          }
        }
      }
      else if (g_held_key != 0 && raw == g_held_key)
      {
        const uint32_t held_ms = now_ms - g_hold_start_ms;
        const uint16_t interval = GetRepeatInterval(held_ms);

        if (now_ms - g_last_repeat_ms >= interval)
        {
          g_last_repeat_ms = now_ms;

          if (g_held_key == '2')
          {
            if (g_inventory_window_start > 0)
            {
              g_inventory_window_start--;
              SetDirty();
            }
          }
          else if (g_held_key == '8')
          {
            if (g_inventory_window_start < InventoryMaxWindowStart())
            {
              g_inventory_window_start++;
              SetDirty();
            }
          }
        }
      }

      // Trigger refresh cycle
      if (now_ms - g_last_inventory_refresh_ms >= INVENTORY_REFRESH_MS)
      {
        g_last_inventory_refresh_ms = now_ms;

        sender->print("IVC");
        sender->print(MSG_TERMINATOR);

        g_refresh_phase = REFRESH_WAIT_COUNT;
      }
    }

    if (raw == 0)
      g_held_key = 0;

    // Poll boot/IVC first; only poll window when boot is done and a window is pending
    InventoryPollBoot(sender);
    if (g_inventory_ready && g_inventory_window_pending)
      InventoryPollWindow(sender);
  }

  static uint32_t last_blink_update = 0;
  if (now_ms - last_blink_update >= 200)
  {
    last_blink_update = now_ms;
    SetDirty();
  }

  UpdateBootState(now_ms, sender);
  DrawUi();
}

bool InventoryBootFinished()
{
  return g_inventory_ready;
}

uint16_t InventoryCount()
{
  return g_inventory_count;
}

uint16_t InventoryWindowStart()
{
  return g_inventory_window_start;
}

void InventorySetWindowStart(uint16_t start)
{
  if (!g_inventory_ready)
    return;

  if (start > InventoryMaxWindowStart())
    start = InventoryMaxWindowStart();

  g_inventory_window_start = start;
}

void InventoryMoveNext()
{
  if (!g_inventory_ready || g_inventory_count == 0)
    return;

  if (g_inventory_window_start < InventoryMaxWindowStart())
    ++g_inventory_window_start;
}

void InventoryMovePrev()
{
  if (!g_inventory_ready || g_inventory_count == 0)
    return;

  if (g_inventory_window_start > 0)
    --g_inventory_window_start;
}
