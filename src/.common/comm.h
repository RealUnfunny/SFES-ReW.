#pragma once

// Eponymous.
enum FoodCondition
{
    FRESH,
    GOING_BAD,
    EXPIRED,
    ERROR
};

// Wifi Password Pair Type
typedef struct
{
    char ssid[20];
    char pass[20];
} wifi_pass_t;

// Boxes Type
typedef struct
{
    char index[2];
    volatile FoodCondition FoodCondition; // Why does it let me do this
} box_t;
