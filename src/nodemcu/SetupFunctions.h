#pragma once

#include "SoftwareSerial.h"
#define NODEMCU_FILE

#include <stdint.h>

/** @file SetupFunctions.h
 *  @brief Sets Up WiFi and other networking features
 */

#pragma once

#define HTTP_PORT 80

/**
 * @brief Sets up the WiFi on the NodeMCU.
 *
 * @details Iterates through "pass_list" (defined in sensitive_data.c)
 *
 * @note It only goes to the first successful connection, and not best
 *       connection, as in the current build the webserver's size isn't
 *       really known yet. [09 Mar 2026]
 */
void WifiSetup();

/**
 * @brief Sets up the connection to the NTP time pool.
 *
 * @note We need to do this, since there was an initial bug where new
 *       logged items where set to be registered from the unix time epoch
 */
void NTPSetup();

/**
 * @brief Sets up the connection to the ntfy.sh topic.
 */
void NTFYSetup();

/**
 * @brief Ensures that the inventory file, and all related files are available on the SD card
 *        at the time of boot.
 */
void SDSetup();

/**
 * @brief Entry notification that is sent on the booting of the system.
 */
void BootNotify();

/**
 * @brief Handles all the communication with the Arduino Uno unit.
 */
void MessageHandler(SoftwareSerial *serial);
