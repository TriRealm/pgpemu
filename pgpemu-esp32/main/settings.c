#include "settings.h"

#include "config_secrets.h"

#define MAX_PGP_DEVICES 4

// Runtime settings
Settings settings = {
    .mutex = NULL,

    .chosen_device = 0,
    .target_active_connections = 1,

    .autocatch = {
    true,   // Device 1
    true,   // Device 2
    true,  // Device 3
    true   // Device 4
},

.autospin = {
    true,   // Device 1
    true,  // Device 2
    true,   // Device 3
    true   // Device 4
},

    .powerbank_ping = false,

    .use_button = false,
    .use_led = false,

    .led_interactions = true,
    .led_brightness = 128,   // default ~50%
    .verbose = true,
};

void init_settings()
{
    settings.mutex = xSemaphoreCreateMutex();

    xSemaphoreTake(
        settings.mutex,
        portMAX_DELAY
    );
}

void settings_ready()
{
    xSemaphoreGive(settings.mutex);
}

bool toggle_setting(bool *var)
{
    if (!var)
    {
        return false;
    }

    if (!xSemaphoreTake(
            settings.mutex,
            10000 / portTICK_PERIOD_MS))
    {
        return false;
    }

    *var = !*var;

    xSemaphoreGive(settings.mutex);

    return true;
}

bool get_setting(bool *var)
{
    if (!var)
    {
        return false;
    }

    if (!xSemaphoreTake(
            settings.mutex,
            portMAX_DELAY))
    {
        return false;
    }

    bool result = *var;

    xSemaphoreGive(settings.mutex);

    return result;
}

uint8_t get_setting_uint8(uint8_t *var)
{
    if (!var)
    {
        return 0;
    }

    if (!xSemaphoreTake(
            settings.mutex,
            portMAX_DELAY))
    {
        return 0;
    }

    uint8_t result = *var;

    xSemaphoreGive(settings.mutex);

    return result;
}

bool get_device_autocatch(uint8_t device)
{
    if (device >= MAX_PGP_DEVICES)
    {
        return false;
    }

    return get_setting(
        &settings.autocatch[device]
    );
}

bool get_device_autospin(uint8_t device)
{
    if (device >= MAX_PGP_DEVICES)
    {
        return false;
    }

    return get_setting(
        &settings.autospin[device]
    );
}

bool set_device_autocatch(
    uint8_t device,
    bool enabled)
{
    if (device >= MAX_PGP_DEVICES)
    {
        return false;
    }

    return set_setting_bool(
        &settings.autocatch[device],
        enabled
    );
}

bool set_device_autospin(
    uint8_t device,
    bool enabled)
{
    if (device >= MAX_PGP_DEVICES)
    {
        return false;
    }

    return set_setting_bool(
        &settings.autospin[device],
        enabled
    );
}

bool set_setting_bool(bool *var, bool val)
{
    if (!var)
    {
        return false;
    }

    if (!xSemaphoreTake(
            settings.mutex,
            portMAX_DELAY))
    {
        return false;
    }

    *var = val;

    xSemaphoreGive(settings.mutex);

    return true;
}

bool set_setting_uint8(uint8_t *var, const uint8_t val)
{
    if (!var)
    {
        return false;
    }

    if (!xSemaphoreTake(
            settings.mutex,
            portMAX_DELAY))
    {
        return false;
    }

    *var = val;

    xSemaphoreGive(settings.mutex);

    return true;
}

uint8_t get_led_brightness()
{
    return get_setting_uint8(
        &settings.led_brightness
    );
}

bool set_led_brightness(uint8_t brightness)
{
    return set_setting_uint8(
        &settings.led_brightness,
        brightness
    );
}

bool set_chosen_device(uint8_t id)
{
    if (!is_valid_secrets_id(id))
    {
        return false;
    }

    if (!xSemaphoreTake(
            settings.mutex,
            portMAX_DELAY))
    {
        return false;
    }

    settings.chosen_device = id;

    xSemaphoreGive(settings.mutex);

    return true;
}