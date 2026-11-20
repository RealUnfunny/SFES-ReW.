# Smart Food Expiry System V2 (or, SFES 2)

College project I made that was formed as an improvement on the original [Smart Food Expiry System Project](https://github.com/RealUnfunny/SFES-Origin).

![Prototype photo](extras/pictures/Prototype%20Photo%201.jpeg)

No endorsements towards "fresho!"... or Snoopy.

## Contents

- [Smart Food Expiry System V2 (or, SFES 2)](#smart-food-expiry-system-v2-or-sfes-2)
- [Contents](#contents)
- [Features](#features)
- [Dependencies](#dependencies)
- [Build](#build)
  - [Hardware](#hardware)
    - [Parts List](#parts-list)
    - [Connections](#connections)
      - [Uno](#uno)
      - [NodeMCU](#nodemcu)
      - [Nano](#nano)
  - [Software](#software)
    - [Flashing it](#flashing-it)
      - [Configuring `sensitive_data.cpp`](#configuring-sensitive_datacpp)
    - [Adding Onto The Project](#adding-onto-the-project)
- [Notes / Q&A](#notes--qa)
  - [Main Unit](#main-unit)
- [Closing Remarks](#closing-remarks)

## Features

The Smart Food Expiry System (SFES) has the following features in a very basic form.

- Monitoring of a select group of perishable food items within a 'specialised' storage container and making a basic guess on the current quality, by measuring the amount of various emitted gases' concentration. Here, we assume that a given type of food category stored will follow a general range of emission of selected gases at a given age.

- Current states are displayed in-person on the modified storage container by three LED bulbs of colors green (good), blue (rotting) and red (expired). Since an owner cannot be expected to monitor all the stored items at all times, the system also connects to the local WiFi and can send messages through [ntfy.sh](https://ntfy.sh).

- Since the system is connected to the WiFi, it also hosts a webserver that enables the owner to log and store new items and tell the system of any changes, provided they are on the same wifi, obviously. On here, they can watch over the specifics of each item, such as how much time is expected to be left before the food finally expires.

- Additionally, since there will be always be items the project can't monitor, both inside or outside some container of the the system, there is also the ability to be able to set a 'faux box' and then set the expected expiry date for that item. Then, the system will also begin monitoring this and provide notifications regarding rotting and decay.

## Dependencies

Tools and software required to build, flash, and develop this project:

- [PlatformIO Core](https://github.com/platformio/platformio-core) ([website](https://platformio.org/))
- [Node.js](https://nodejs.org/) and [npm](https://www.npmjs.com/) 
- [esbuild](https://github.com/evanw/esbuild) ([website](https://esbuild.github.io/)) 
- [Python](https://www.python.org/)

Everything else, i.e., the microcontroller frameworks/firmware libraries (Arduino AVR/ESP8266 cores, RF24, ESPAsyncWebServer, etc.) are declared in `platformio.ini` and installed automatically by PlatformIO.

## Build

### Hardware

#### Parts List

The system has the following parts:

1. **Main Unit**
   1. Arduino UNO R3
      - Three status LEDs
      - Gas sensors (MQ-3 and MQ-4)
      - nRF24L01(+) radio module*
   2. ESP8266-12E (development board, e.g. NodeMCU)
      - Micro-SD card reader
      - Micro SD card (32 GB or less; anything over ~2 GB is plenty)
2. **Remote Unit(s)** (e.g. Arduino Nano)
   - Gas sensors (MQ-3 and MQ-4)
   - nRF24L01(+) radio module
   - Three status LEDs

Other things include a **5V ⇄ 3.3V level shifter** (for the Uno–NodeMCU serial link and any 5 V SD reader), and **decoupling capacitors** on each nRF24L01(+) module to protect it from power-supply dips (a common cause of flaky radios). A separate regulated **3.3 V supply** for each radio module and a power-supply module for each unit also help. *It is often cheaper and easier to source these parts from a local electronics/microcontroller store than from large online marketplaces.*

> \*You don't need the '+' (nRF24L01+) variant for this project: the packets are small and short-range enough for the base nRF24L01 module, which is what I used. The '+' variant adds features/capabilities you likely won't need here.

#### Connections

##### Uno

| Pin | Function                  | Connects                 | Notes                                |
|:---:| ------------------------- |:------------------------:| ------------------------------------ |
| D2  | Fresh Status LED          | LED + Resistor to GND    | called 'FRESH_PIN' in code           |
| D3  | Going Bad Status LED      | "                        | called "GOING_BAD_PIN" in code       |
| D4  | Expired status LED        | "                        | called "EXPIRED_PIN" in code         |
| D5  | SoftwareSerial `RX`       | NodeMCU `D1 (GPIO5, TX)` | -                                    |
| D6  | SoftwareSerial `TX`       | NodeMCU `D2 (GPIO4, RX)` | You should use the level shifter.    |
| D9  | nRF24 `CE`                | Radio `CE`               | Radio needs a regulated 3.3V supply. |
| D10 | nRF24 `CSN`               | Radio `CSN`              | -                                    |
| D11 | nRF24 `MOSI`              | Radio `MOSI`             | -                                    |
| D12 | nRF24 `MISO`              | Radio `MISO`             | -                                    |
| D13 | nRF24 `SCK`               | Radio `SCK`              | -                                    |
| A0  | Alcohol Gas Sensor (MQ-3) | Sensor `AO`              | called 'ALC_SENSOR_PIN' in code      |
| A1  | CH3 Gas Sensor (MQ-4)     | "                        | called 'CH3_SENSOR_PIN' in code      |

##### NodeMCU

| Pin         | Function            | Connects         | Notes                                                        |
|:-----------:| ------------------- |:----------------:| ------------------------------------------------------------ |
| D1 (GPIO5)  | SoftwareSerial `TX` | Uno `D5 (RX)`    | -                                                            |
| D2 (GPIO4)  | SoftwareSerial `RX` | Uno `D6 (TX)`    | You should use the level shifter.                            |
| D5 (GPIO14) | SD Card `SCK`       | SD reader `SCK`  | -                                                            |
| D6 (GPIO12) | SD Card `MISO`      | SD reader `MISO` | -                                                            |
| D7 (GPIO13) | SD Card `MOSI`      | SD reader `MOSI` | Use a Level shifter if the reader is 5V. Usually wouldn't be |
| D8 (GPIO15) | SD Card `CS`        | SD reader `CS`   | Passed to `SD.begin(D8)` in code                             |

Remember that the **NodeMCU and Arduino UNO also need to be connected to the same ground** such that they can understand each other.

##### Nano

| Pin | Function                  | Connects              | Notes                                |
|:---:| ------------------------- |:---------------------:| ------------------------------------ |
| D6  | Expired Status LED        | LED + Resistor to GND | -                                    |
| D7  | Going Bad Status LED      | "                     | -                                    |
| D8  | Fresh Status LED          | "                     | -                                    |
| D9  | nRF24 `CE`                | Radio `CE`            | Radio needs a regulated 3.3V supply. |
| D10 | nRF24 `CSN`               | Radio `CSN`           | -                                    |
| D11 | nRF24 `MOSI`              | Radio `MOSI`          | -                                    |
| D12 | nRF24 `MISO`              | Radio `MISO`          | -                                    |
| D13 | nRF24 `SCK`               | Radio `SCK`           | -                                    |
| A0  | Alcohol Gas Sensor (MQ-3) | Sensor `AO`           | -                                    |
| A1  | CH3 Gas Sensor (MQ-4)     | Sensor `AO`           | -                                    |

### Software

#### Flashing it

You need [PlatformIO Core](https://github.com/platformio/platformio-core) - something that is x10^18 (if you don't know how big that is, it's like 12 zeroes) times better than the Arduino IDE - installed. It also has [extensions for VS Code](https://github.com/platformio/platformio-vscode-ide), but the steps below use the CLI.

1. Clone the repo into the folder you want.

   ```shell
   git clone "https://github.com/RealUnfunny/SFES-ReW." ./.
   ```

2. In `platformio.ini`, set the upload port for each device based on the output of `pio device list` (each device's environment is defined in an `[env:device_name]` block). This is mainly needed if you're working with two or more boards at the same time.

   ```ini
   [env:device_name]
   ...
   upload_port = <port_connected_to_device_name>
   ```

3. Configure [sensitive_data.cpp](#configuring-sensitive_datacpp).

4. To set up the web UI, you need [Node.js](https://nodejs.org/) installed (it comes with npm). The UI source lives in `src/nodemcu/tim`. From the repo root:

   ```bash
   cd src/nodemcu/tim
   npm install          # installs esbuild, the only dependency
   node build.mjs       # bundles everything into dist/index.html
   ```

   `build.mjs` should inline the CSS and JS into a single self-contained page. **Copy `dist/index.html` to the *root of the microSD card*** as `index.html`. The separate `dist/app.js`, `dist/styles.css` and `dist/index.html.gz` files are build byproducts which you can safely ignore. `inventory.csv` is created automatically on the card at first boot, or read if it already exists.

   > For your own convenience, it is possible to add on to the NodeMCU's Python extra script (`src/python/py_nodemcu.py`) to automatically build the UI and copy `dist/index.html` to a connected SD card reader on your computer whenever building/flashing the main software. You can ask why I didn't just do this - short answer short is I was lazy, sorgy

5. Set up each remote Nano (if you are using remote units). Each Nano needs its own **2-character box label** so the main unit can identify and poll it (for example `A1`, `B2`). `A0` is reserved for the main/local unit on the Arduino UNO.

   Before flashing a Nano, open `src/nano/CommandInterpreter.h` and set the macro`BOX_ID` to what you want that board's label to be.

   ```cpp
   #define BOX_ID "A1"
   ```

   You can then flash that Nano (with the correct Nano connected and upload port selected).

   ```bash
   pio run -e nano -t upload
   ```

   Repeat for every remote unit, changing `BOX_ID` to a different label each time and uploading again. Make sure the same label is used in the active-box/inventory configuration that the UNO receives.

   > `MY_ADDRESS` is computed from `BOX_ID` via `CalcAddress()` in `src/.common/comm.h`:
   >
   > ```cpp
   > inline constexpr uint64_t CalcAddress(const char *index)
   > {
   >   return (0xF0F0F0F000LL | (uint64_t)index[0]) | ((uint64_t)index[1] << 8);
   > }
   > ```
   >
   > For the sake of an explaination, all address starts from a fixed base (`0xF0F0F0F000`) and embeds the ASCII codes of the two label characters into the low bytes, producing a unique 5-byte nRF24 address per label (ex. `"A1"` is `0xF0F0F0F3141`, from `'A'` = `0x41` and `'1'` = `0x31`). The Nano listens on this address too. Both sides use the main unit's fixed address (`0xF0F0F0AA11LL`) to send data back to the UNO. This is quick and cheap, helping in getting addresses on the fly rather than retriving it through a lookup table, which trades memory for computation time. I mean at this scale it wouldn't matter, but I thought of using it if the scale was supposed to be really large.

6. When connected to all the boards at the same time, run:

   ```bash
   pio run -t upload
   ```

   For a specific device defined in the `platformio.ini` block as `device_name`,

   ```bash
   pio run -t upload -e <device_name>
   ```

##### Configuring `sensitive_data.cpp`

This is a sensitive file stored in `src/nodemcu` that you should keep **private**. It must be created before flashing the NodeMCU firmware, since the NodeMCU needs these details to connect and send notifications. As per the current `.gitignore`, this file is ignored and not tracked by git.

1. Create the file and define the following include and macros:

   ```cpp
   #pragma once
   #include ".common/comm.h"
   
   #define NTFY_SERVER "ntfy.sh"
   #define NTFY_PORT 80
   #define NTFY_TOPIC "a-long-random-topic-of-your-own"
   ```

   > Use an ntfy topic whose name sort of acts as a password. I used the [1Password](https://1password.com/password-generator) password generator, set to **15 characters with symbols on**; your ntfy topic should look something like `t_Wk^eYzyd-hh6V`.

2. Define a `const wifi pass_list` variable with your own WiFi networks, stored in `<wifi_name, wifi_password>` pairs. The NodeMCU will try them (in order of declaration) until one connects.

   ```cpp
   const wifi_pass_t pass_list[] = {
       {"YourNetwork", "YourPassword"},
       {"YourNetwor2", "YourPas2word"},
   };
   ```

3. After every change, remember to flash the board (again).

   ```bash
   pio run -e nodemcuv2 -t upload
   ```

#### Adding Onto The Project

The project comes with python scripts that automatically create `compilation_commands.json` in the directories for each device in `src/<device_name>`. These are automatically run and created when flashing the cards or running `pio run -t compiledb`. Any cool additions can be mentioned through a issue, if you care that much about it.

## Notes / Q&A

This section is just going to be about decisions I made while making the project, and why I made them. Think of it as a diary of sorts.

### Main Unit

The main unit consists of the Arduino UNO and the NodeMCU ESP8266-12E dev board.

- Why the UNO? 

  Not for any specific reason, really. If you knew your way around you could probably replace it with another Nano here and be A-OK, to the point where the Nano might actually be more efficient with power consumption in a computational sense. It was just convenient to develop with due to having a reset button, rather than a Nano where I'd have to yank the cable out.

- Why the dev board?

  It had more running memory and, obviously, is more accessible than a bare ESP8266 module, while also being more powerful. Moreover, completely unrelated to the previous points, I couldn't get the regular ESP8266 to work 😬.

- `.csv` instead of `.json` for storage?

  The previous version of the project, i.e., [SFES Origin](https://github.com/RealUnfunny/SFES-Origin), used `.json`, with the **ArduinoJson** library.

  Worst mistake of my life.

  The board kept running out of memory, and it became kind of problematic since I kept having memory errors on the day of my presentation. I think I was able to get it working then, for the most part, but since this project has a bigger scope, it was just a better idea in the end to write up a quick `.csv` parser/writer. This actually ended up being a great decision due to it being much, much faster than the `.json` libraries, and also much lower in memory usage.

- Accessing more than one box at a time?

  For anyone wondering: the nRF24L01(+) has 6 receive pipes, which people often (roughly) treat as a small limit on simultaneous connections. You can sidestep that, however, by polling remote units one-by-one instead of keeping active links open.

  In principle, this polling approach can scale well beyond six units, but in the current firmware the number of active boxes is capped by `MAX_BOXES` (set to 2 in `src/arduino/box_actions.h`), so you'd need to raise that (and adjust related buffers/handling) to support more. RF24's link-layer is generally reliable, since there's a retries/auto-ACK behavior and the request/response sequence check. Blah Blah Blah project

## Closing Remarks

There are a couple of things I couldn't implement, for which dead code still exists somewhere in the project, unfortunately.

- A 16×2 LCD screen and a 12-key keypad for complete offline viewing and adding of the inventory, connected to the Arduino UNO. You would use the keys based on context, and then type out specific names like on an old multi-tap Nokia. I did have this semi-working, but setup was tedious since the Arduino UNO had run out of ports, so there was more backend work regarding adding a port expander, the complications of which led to the whole shebang being finally dropped. This alleged plan also just wastes space by downloading the Adafruit display libraries onto your computer for no reason 😁.
- The website could probably be written by someone who actually knows HTML, JavaScript, and CSS, since by nature it's one file served by this poor internet module. I didn't really research a way to fix this problem.
- There was a plan to have the inventory manager be hosted on `tim.local` so that it would be easier to type, but I couldn't get the mDNS part figured out, and I think that it would only work if the system was confined to one place, i.e., to one network — which it wasn't, considering it was a college project and... it had to be at college. I could also do it through configuring the DNS settings on my computer, but I ultimately decided that most would want a more seamless scan-and-forget through a QR code of the IP, rather than the former case.
- One could also add more sensors to the units, such as a nitrogen sensor which may allow for detecting spoilage of meat products. Each food group emits its own gases, which can be picked up by a good sensor, and then probably used to extrapolate a result. Experimentally, this project includes an experimental avocado preset/profile, but avocados are too expensive for me to leave to the crutches of science!

## Licensing

do whatever you want bro🥀
