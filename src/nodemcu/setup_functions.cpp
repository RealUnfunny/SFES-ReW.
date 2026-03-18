#include "setup_functions.h"

void WifiSetup()
{
    for (wifi_pass_t wifi_pass : pass_list)
    {
        d_SerialPrint("Connecting to...");
        d_SerialPrintV(wifi_pass.ssid);

        WiFi.begin(wifi_pass.ssid, wifi_pass.pass);

        while ((WiFi.status() != WL_CONNECTED) or (WiFi.status() != WL_CONNECT_FAILED))
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
        default:
            continue;
        }
    }
}

void NTPSetup()
{
    time_t system_time = time(nullptr);
    uint8_t retries = 0;

    do
    {
        d_SerialPrintln("Configuring NTP...");
        configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
        d_SerialPrint("Waiting for NTP...");

        for (int attempts = 0; system_time == 0 and attempts < 20; attempts++)
        {
            delay(500);
            system_time = time(nullptr);
            d_SerialPrint(".");
        }

        if (system_time != 0)
        {
            d_SerialPrintln("SUCCESS!");
            d_SerialPrintf("Current Unix time: %lu\n", system_time);
            return;
        }

        d_SerialPrintln("Warning: NTP Sync failed. Retrying...");

        retries++;

        if (retries >= 3)
        {
            d_SerialPrintln("Too many retries, restarting...");
            delay(1000);
            ESP.restart();
            // Assuming that if the system has failed to the get the
            // NTP Connection for nearly ~60 times, there must be
            // wrong with the initial setup process
        }
    } while (system_time == 0);
}
