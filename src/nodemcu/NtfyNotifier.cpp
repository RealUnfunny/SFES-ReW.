#include "NtfyNotifier.h"

#include "CsvOperations.h"
#include "ESP8266HTTPClient.h"
#include "IPAddress.h"
#include "WString.h"
#include "WiFiClient.h"

#include ".common/comm.h"
#include "sensitive_data.cpp"

void NtfyNotify(const char *tags, const char *title, const char *message, const char *priority)
{
  const String topicPath = String("/") + String(NTFY_TOPIC);

  HTTPClient http;
  IPAddress ntfy_ip;
  WiFiClient esp_client;

  if (!http.begin(esp_client, String(NTFY_SERVER), NTFY_PORT, topicPath))
  {
    d_SerialPrintln("Fatal: HTTP connect failed.");
    http.end();
    return;
  }

  http.addHeader("X-Title", title);
  http.addHeader("X-Priority", priority);
  http.addHeader("X-Tags", tags);
  http.addHeader("Markdown", "yes");

  int http_response = http.POST(message);

  if (http_response > 0)
    d_SerialPrintf("NTFY Post success. Code %d\n", http_response);
  else
    d_SerialPrintf("NTFY Post Failed. Error %s(%d)\n", http.errorToString(http_response).c_str(), http_response);

  http.end();
  yield();
}

void NotifyItemAdded(const InventoryRecordLine *record)
{
  String msg = String("**") + record->item_name + "** has been added to box **" + record->box_id + "**.";
  NtfyNotify("white_check_mark,package", "Item Added", msg.c_str(), "default");
}

void NotifyItemDeleted(const char *box_id)
{
  String msg = String("Item in box **") + box_id + "** has been deleted from inventory.";
  NtfyNotify("wastebasket", "Item Deleted", msg.c_str(), "default");
}

void BootNotify()
{
  time_t now = time(nullptr);
  struct tm *t = localtime(&now);
  char time_str[32];
  strftime(time_str, sizeof(time_str), "%d/%m/%Y %H:%M:%S", t);
  String msg = String("System booted successfully at **") + time_str + "**. Ready to monitor inventory.";
  NtfyNotify("white_check_mark,rocket", "System Ready", msg.c_str(), "default");
}
