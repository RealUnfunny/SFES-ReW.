#include "oled_graphics.h"
#include ".common/comm.h"
#include "Arduino.h"

Adafruit_SSD1306 disp(SCR_WDH, SCR_HGT, &Wire, OLED_RESET);

void OledScreenInit()
{
    if (!disp.begin(SSD1306_SWITCHCAPVCC, SCR_ADDR))
    {
        d_SerialPrint("Allocation Failed!");
        while (1)
            ;
    }

    disp.clearDisplay();
    disp.setTextColor(SSD1306_WHITE);
    disp.cp437(true);
}

void LoadingScreen(bool EndEvent)
{
    if (EndEvent == true)
        return;
    disp.clearDisplay();
    disp.setCursor(0, 0);

    disp.setTextSize(2);
    disp.write("  Loading");

    disp.setCursor(56, 32);
    disp.setTextSize(3);
    disp.write(millis());
    disp.display();
    // Not really recommended to do,
    // since millis() can get really big
    // which would cause a lot of long-to-char
    // conversion overhead, so hopefully loading
    // is only done at the start of the system,
    // when millis is small and managable.
}
