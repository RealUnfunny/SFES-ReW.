#include "NtfyNotifier.h"

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
