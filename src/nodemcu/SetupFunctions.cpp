#include "SetupFunctions.h"

#include "nodemcu/NtfyNotifier.h"
#include "wl_definitions.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <IPAddress.h>

#include ".common/comm.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <SD.h>

#include "sensitive_data.cpp"
// Contains defines for:
//     -> NTFY_SERVER
//     -> NTFY_PORT
//     -> NTFY_TOPIC
//     -> pass_list: Array of wifi_pass_t (as defined in .common/comm.h)

#include "SetupFunctions.h"

WiFiUDP ntp_udp;
NTPClient time_client(ntp_udp, "pool.ntp.org", 0, 0);

void WifiSetup()
{
  for (wifi_pass_t wifi_pass : pass_list)
  {
    d_SerialPrint("Connecting to...");
    d_SerialPrintV(wifi_pass.ssid);

    WiFi.begin(wifi_pass.ssid, wifi_pass.pass);

    while ((WiFi.status() != WL_CONNECTED) and (WiFi.status() != WL_WRONG_PASSWORD))
    {
      d_delay(500);
      d_SerialPrint(".");
    }

    switch (WiFi.status()) // Using a switch so that other cases can be handled in the future
    {
      case WL_CONNECTED:
        d_SerialPrintln("\nWiFi connected");
        d_SerialPrint("IP address: ");
        d_SerialPrintlnV(WiFi.localIP());
        return;
        // Kill the function on connecting
      case WL_WRONG_PASSWORD:
        d_SerialPrintf("\nWrong Password for \"%s\"!", wifi_pass.ssid);
        continue;
      default:
        continue;
    }
  }
}

void NTPSetup()
{
  time_client.begin();

  d_SerialPrintln("Waiting for NTP...");
  while (!time_client.update())
  {
    d_SerialPrint(".");
    delay(500);
  }

  time_t epochTime = time_client.getEpochTime();
  timeval tv = {epochTime, 0};
  settimeofday(&tv, nullptr);
  d_SerialPrintf("Current Unix time: %lu\n", time(nullptr));
}

void NTFYSetup()
{
  IPAddress ntfy_ip;

  d_SerialPrintf("Resolving host :%s\n", NTFY_SERVER);
  if (WiFi.hostByName(NTFY_SERVER, ntfy_ip) == 0)
  {
    d_SerialPrintln("Fatal: DNS Resolution failed for ntfy.sh!");
    return;
  }
}

void SDSetup()
{
  time_client.update();

  if (!SD.begin(D8))
  {
    d_SerialPrintln("SD Init Failed!");
    return;
  }

  d_SerialPrintln("SD Card Init'ized.");

  if (!SD.exists(INVENTORY_FILE))
  {
    d_SerialPrintln("File not found! Creating...");
    File f = SD.open(INVENTORY_FILE, FILE_WRITE);
    if (!f)
      return;
    f.println("serial_no,box_id,item_name,reg_date,expiry_date,physical");
    f.close();
    d_SerialPrintln("File Created.");
  }
  else
    d_SerialPrintln("File Exists. Continuing...");
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
