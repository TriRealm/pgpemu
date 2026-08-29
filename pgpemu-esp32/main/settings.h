#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define WIFI_PASSWORD_MAX_LEN 64

#define MAX_PGP_DEVICES 4

typedef struct
{
    // Any read/write must lock this
    SemaphoreHandle_t mutex;

    // Set which PGP device preset stored in NVS should be cloned
    uint8_t chosen_device;

    // Set how many client connections are allowed at the same time
    uint8_t target_active_connections;

    // Per-device Gotcha functions
    bool autocatch[MAX_PGP_DEVICES];
    bool autospin[MAX_PGP_DEVICES];

    // Waste a bit of power to keep your powerbank from turning us off
    bool powerbank_ping;

    // Do you have an input button? Only checked on boot
    bool use_button;

    // Do you have an LED? Only checked on boot
    bool use_led;

    // Show pokestop/pokemon interactions using the LED
    bool led_interactions;

    // LED brightness (0-255)
    uint8_t led_brightness;

    // Wi-Fi configuration AP password
    char wifi_password[WIFI_PASSWORD_MAX_LEN];

    // Verbose log messages
    bool verbose;

} Settings;

extern Settings settings;

void init_settings();
void settings_ready();

bool toggle_setting(bool *var);
bool get_setting(bool *var);

uint8_t get_setting_uint8(uint8_t *var);

bool set_setting_bool(bool *var, bool val);
bool set_setting_uint8(uint8_t *var, const uint8_t val);

bool get_device_autocatch(uint8_t device);
bool get_device_autospin(uint8_t device);

bool set_device_autocatch(
    uint8_t device,
    bool enabled
);

uint8_t get_led_brightness();
bool set_led_brightness(uint8_t brightness);

bool get_wifi_password(char *out, size_t out_size);
bool set_wifi_password(const char *new_password);

bool set_device_autospin(
    uint8_t device,
    bool enabled
);

bool set_chosen_device(uint8_t id);

#endif /* SETTINGS_H */