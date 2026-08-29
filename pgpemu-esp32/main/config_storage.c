#include "config_storage.h"

#include <string.h>

#include "config_secrets.h"
#include "log_tags.h"
#include "nvs_helper.h"
#include "settings.h"

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char KEY_AUTOCATCH[] = "catch";
static const char KEY_AUTOSPIN[] = "spin";

static const char KEY_AUTOCATCH_0[] = "catch0";
static const char KEY_AUTOCATCH_1[] = "catch1";
static const char KEY_AUTOCATCH_2[] = "catch2";
static const char KEY_AUTOCATCH_3[] = "catch3";

static const char KEY_AUTOSPIN_0[] = "spin0";
static const char KEY_AUTOSPIN_1[] = "spin1";
static const char KEY_AUTOSPIN_2[] = "spin2";
static const char KEY_AUTOSPIN_3[] = "spin3";

static const char KEY_POWERBANK_PING[] = "ping";
static const char KEY_CHOSEN_DEVICE[] = "device";
static const char KEY_CONNECTION_COUNT[] = "conns";
static const char KEY_USE_BUTTON[] = "usebut";
static const char KEY_USE_LED[] = "useled";
static const char KEY_LED_BRIGHTNESS[] = "ledbright";
static const char KEY_SHOW_LED_INTERACTIONS[] = "ledinter";
static const char KEY_WIFI_SSID[] = "wifissid";
static const char KEY_WIFI_PASSWORD[] = "wifipass";
static const char KEY_VERBOSE[] = "verbose";

#define MAX_PGP_DEVICES 4



void init_config_storage()
{
    esp_err_t err;

    err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());

        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);
}



void read_stored_settings(bool use_mutex)
{
    esp_err_t err;

    int8_t autocatch[MAX_PGP_DEVICES] = {0};
    int8_t autospin[MAX_PGP_DEVICES] = {0};

    int8_t legacy_autocatch = 0;
    int8_t legacy_autospin = 0;

    int8_t powerbank_ping = 0;
    int8_t use_button = 0;
    int8_t use_led = 0;
    int8_t led_interactions = 0;
    int8_t verbose = 0;

    uint8_t chosen_device = 0;
    uint8_t connection_count = 0;

    bool has_new_autocatch = false;
    bool has_new_autospin = false;

    if (use_mutex)
    {
        if (!xSemaphoreTake(
                settings.mutex,
                portMAX_DELAY))
        {
            ESP_LOGE(
                CONFIG_STORAGE_TAG,
                "cannot get settings mutex"
            );

            return;
        }
    }

    nvs_handle_t user_settings_handle;

    err = nvs_open(
        "user_settings",
        NVS_READONLY,
        &user_settings_handle
    );

    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGW(
                CONFIG_STORAGE_TAG,
                "user settings partition doesn't exist, using default settings"
            );
        }
        else
        {
            ESP_ERROR_CHECK(err);
        }

        if (use_mutex)
        {
            xSemaphoreGive(settings.mutex);
        }

        return;
    }

    
    // Per-device Autocatch
    

    const char *autocatch_keys[MAX_PGP_DEVICES] = {
        KEY_AUTOCATCH_0,
        KEY_AUTOCATCH_1,
        KEY_AUTOCATCH_2,
        KEY_AUTOCATCH_3
    };

    for (uint8_t i = 0; i < MAX_PGP_DEVICES; i++)
    {
        err = nvs_get_i8(
            user_settings_handle,
            autocatch_keys[i],
            &autocatch[i]
        );

        if (err == ESP_OK)
        {
            has_new_autocatch = true;

            settings.autocatch[i] =
                (bool)autocatch[i];
        }
        else if (err != ESP_ERR_NVS_NOT_FOUND)
        {
            nvs_read_check(
                CONFIG_STORAGE_TAG,
                err,
                autocatch_keys[i]
            );
        }
    }

    
    // Per-device Autospin
    

    const char *autospin_keys[MAX_PGP_DEVICES] = {
        KEY_AUTOSPIN_0,
        KEY_AUTOSPIN_1,
        KEY_AUTOSPIN_2,
        KEY_AUTOSPIN_3
    };

    for (uint8_t i = 0; i < MAX_PGP_DEVICES; i++)
    {
        err = nvs_get_i8(
            user_settings_handle,
            autospin_keys[i],
            &autospin[i]
        );

        if (err == ESP_OK)
        {
            has_new_autospin = true;

            settings.autospin[i] =
                (bool)autospin[i];
        }
        else if (err != ESP_ERR_NVS_NOT_FOUND)
        {
            nvs_read_check(
                CONFIG_STORAGE_TAG,
                err,
                autospin_keys[i]
            );
        }
    }

    
    // Legacy Autocatch migration
    

    if (!has_new_autocatch)
    {
        err = nvs_get_i8(
            user_settings_handle,
            KEY_AUTOCATCH,
            &legacy_autocatch
        );

        if (err == ESP_OK)
        {
            ESP_LOGI(
                CONFIG_STORAGE_TAG,
                "migrating legacy autocatch setting to all devices: %d",
                legacy_autocatch
            );

            for (uint8_t i = 0; i < MAX_PGP_DEVICES; i++)
            {
                settings.autocatch[i] =
                    (bool)legacy_autocatch;
            }
        }
        else if (err != ESP_ERR_NVS_NOT_FOUND)
        {
            nvs_read_check(
                CONFIG_STORAGE_TAG,
                err,
                KEY_AUTOCATCH
            );
        }
    }

    
    // Legacy Autospin migration
    

    if (!has_new_autospin)
    {
        err = nvs_get_i8(
            user_settings_handle,
            KEY_AUTOSPIN,
            &legacy_autospin
        );

        if (err == ESP_OK)
        {
            ESP_LOGI(
                CONFIG_STORAGE_TAG,
                "migrating legacy autospin setting to all devices: %d",
                legacy_autospin
            );

            for (uint8_t i = 0; i < MAX_PGP_DEVICES; i++)
            {
                settings.autospin[i] =
                    (bool)legacy_autospin;
            }
        }
        else if (err != ESP_ERR_NVS_NOT_FOUND)
        {
            nvs_read_check(
                CONFIG_STORAGE_TAG,
                err,
                KEY_AUTOSPIN
            );
        }
    }

    
    // Global Boolean settings
    

    err = nvs_get_i8(
        user_settings_handle,
        KEY_POWERBANK_PING,
        &powerbank_ping
    );

    if (nvs_read_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_POWERBANK_PING))
    {
        settings.powerbank_ping =
            (bool)powerbank_ping;
    }

    err = nvs_get_i8(
        user_settings_handle,
        KEY_USE_BUTTON,
        &use_button
    );

    if (nvs_read_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_USE_BUTTON))
    {
        settings.use_button =
            (bool)use_button;
    }

    err = nvs_get_i8(
        user_settings_handle,
        KEY_USE_LED,
        &use_led
    );

    if (nvs_read_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_USE_LED))
    {
        settings.use_led =
            (bool)use_led;
    }

    err = nvs_get_i8(
        user_settings_handle,
        KEY_SHOW_LED_INTERACTIONS,
        &led_interactions
    );

    if (nvs_read_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_SHOW_LED_INTERACTIONS))
    {
        settings.led_interactions =
            (bool)led_interactions;
    }

    uint8_t led_brightness = 128;

    err = nvs_get_u8(
        user_settings_handle,
        KEY_LED_BRIGHTNESS,
        &led_brightness
    );

    if (nvs_read_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_LED_BRIGHTNESS))
    {
        settings.led_brightness =
            led_brightness;
    }

    err = nvs_get_i8(
        user_settings_handle,
        KEY_VERBOSE,
        &verbose
    );

    if (nvs_read_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_VERBOSE))
    {
        settings.verbose =
            (bool)verbose;
    }

    // Wi-Fi SSID
    char stored_wifi_ssid[WIFI_SSID_MAX_LEN] = {0};
    size_t wifi_ssid_len = sizeof(stored_wifi_ssid);

    err = nvs_get_str(
        user_settings_handle,
        KEY_WIFI_SSID,
        stored_wifi_ssid,
        &wifi_ssid_len
    );

    if (err == ESP_OK)
    {
        strlcpy(settings.wifi_ssid, stored_wifi_ssid, sizeof(settings.wifi_ssid));
    }
    else if (err != ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_read_check(CONFIG_STORAGE_TAG, err, KEY_WIFI_SSID);
    }

    // Wi-Fi Password
    char stored_wifi_password[WIFI_PASSWORD_MAX_LEN] = {0};
    size_t wifi_password_len = sizeof(stored_wifi_password);

    err = nvs_get_str(
        user_settings_handle,
        KEY_WIFI_PASSWORD,
        stored_wifi_password,
        &wifi_password_len
    );

    if (err == ESP_OK)
    {
        strlcpy(settings.wifi_password, stored_wifi_password, sizeof(settings.wifi_password));
    }
    else if (err != ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_read_check(CONFIG_STORAGE_TAG, err, KEY_WIFI_PASSWORD);
    }
    
    // Chosen device
    

    err = nvs_get_u8(
        user_settings_handle,
        KEY_CHOSEN_DEVICE,
        &chosen_device
    );

    if (nvs_read_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_CHOSEN_DEVICE))
    {
        if (is_valid_secrets_id(chosen_device))
        {
            settings.chosen_device =
                chosen_device;
        }
        else
        {
            ESP_LOGE(
                CONFIG_STORAGE_TAG,
                "invalid chosen device %d",
                chosen_device
            );
        }
    }

    
    // Target connection count
    

    err = nvs_get_u8(
        user_settings_handle,
        KEY_CONNECTION_COUNT,
        &connection_count
    );

    if (nvs_read_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_CONNECTION_COUNT))
    {
        if (connection_count <= CONFIG_BT_ACL_CONNECTIONS &&
            connection_count > 0)
        {
            settings.target_active_connections =
                connection_count;
        }
        else
        {
            ESP_LOGE(
                CONFIG_STORAGE_TAG,
                "invalid target active connections: %d (1-%d allowed)",
                connection_count,
                CONFIG_BT_ACL_CONNECTIONS
            );
        }
    }

    

    nvs_close(user_settings_handle);

    if (use_mutex)
    {
        xSemaphoreGive(settings.mutex);
    }

    
    // Log loaded device settings
    

    ESP_LOGI(
        CONFIG_STORAGE_TAG,
        "Autocatch: D1=%d D2=%d D3=%d D4=%d",
        settings.autocatch[0],
        settings.autocatch[1],
        settings.autocatch[2],
        settings.autocatch[3]
    );

    ESP_LOGI(
        CONFIG_STORAGE_TAG,
        "Autospin: D1=%d D2=%d D3=%d D4=%d",
        settings.autospin[0],
        settings.autospin[1],
        settings.autospin[2],
        settings.autospin[3]
    );

    ESP_LOGI(
        CONFIG_STORAGE_TAG,
        "user settings read from nvs"
    );
}


// WRITE CONFIG STORAGE


bool write_config_storage()
{
    esp_err_t err;

    if (!xSemaphoreTake(
            settings.mutex,
            portMAX_DELAY))
    {
        ESP_LOGE(
            CONFIG_STORAGE_TAG,
            "cannot get settings mutex"
        );

        return false;
    }

    nvs_handle_t user_settings_handle;

    err = nvs_open(
        "user_settings",
        NVS_READWRITE,
        &user_settings_handle
    );

    if (err != ESP_OK)
    {
        xSemaphoreGive(settings.mutex);

        ESP_LOGE(
            CONFIG_STORAGE_TAG,
            "failed to open user settings"
        );

        return false;
    }

    bool all_ok = true;

    
    // Per-device Autocatch
    

    const char *autocatch_keys[MAX_PGP_DEVICES] = {
        KEY_AUTOCATCH_0,
        KEY_AUTOCATCH_1,
        KEY_AUTOCATCH_2,
        KEY_AUTOCATCH_3
    };

    for (uint8_t i = 0; i < MAX_PGP_DEVICES; i++)
    {
        err = nvs_set_i8(
            user_settings_handle,
            autocatch_keys[i],
            settings.autocatch[i] ? 1 : 0
        );

        all_ok =
            all_ok &&
            nvs_write_check(
                CONFIG_STORAGE_TAG,
                err,
                autocatch_keys[i]
            );
    }


    // Per-device Autospin


    const char *autospin_keys[MAX_PGP_DEVICES] = {
        KEY_AUTOSPIN_0,
        KEY_AUTOSPIN_1,
        KEY_AUTOSPIN_2,
        KEY_AUTOSPIN_3
    };

    for (uint8_t i = 0; i < MAX_PGP_DEVICES; i++)
    {
        err = nvs_set_i8(
            user_settings_handle,
            autospin_keys[i],
            settings.autospin[i] ? 1 : 0
        );

        all_ok =
            all_ok &&
            nvs_write_check(
                CONFIG_STORAGE_TAG,
                err,
                autospin_keys[i]
            );
    }


    // Global Boolean settings


    err = nvs_set_i8(
        user_settings_handle,
        KEY_POWERBANK_PING,
        settings.powerbank_ping
    );

    all_ok =
        all_ok &&
        nvs_write_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_POWERBANK_PING
        );

    err = nvs_set_i8(
        user_settings_handle,
        KEY_USE_BUTTON,
        settings.use_button
    );

    all_ok =
        all_ok &&
        nvs_write_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_USE_BUTTON
        );

    err = nvs_set_i8(
        user_settings_handle,
        KEY_USE_LED,
        settings.use_led
    );

    all_ok =
        all_ok &&
        nvs_write_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_USE_LED
        );

    err = nvs_set_i8(
        user_settings_handle,
        KEY_SHOW_LED_INTERACTIONS,
        settings.led_interactions
    );

    all_ok =
        all_ok &&
        nvs_write_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_SHOW_LED_INTERACTIONS
        );

        err = nvs_set_u8(
        user_settings_handle,
        KEY_LED_BRIGHTNESS,
        settings.led_brightness
    );

    all_ok =
        all_ok &&
        nvs_write_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_LED_BRIGHTNESS
        );

        // Wi-Fi SSID
        err = nvs_set_str(
        user_settings_handle,
        KEY_WIFI_SSID,
        settings.wifi_ssid
    );

    all_ok =
        all_ok &&
        nvs_write_check(CONFIG_STORAGE_TAG, err, KEY_WIFI_SSID);

        // Wi-Fi Password

        err = nvs_set_str(
        user_settings_handle,
        KEY_WIFI_PASSWORD,
        settings.wifi_password
    );

    all_ok =
        all_ok &&
        nvs_write_check(CONFIG_STORAGE_TAG, err, KEY_WIFI_PASSWORD);

    err = nvs_set_i8(
        user_settings_handle,
        KEY_VERBOSE,
        settings.verbose
    );

    all_ok =
        all_ok &&
        nvs_write_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_VERBOSE
        );


    // uint8 settings


    err = nvs_set_u8(
        user_settings_handle,
        KEY_CHOSEN_DEVICE,
        settings.chosen_device
    );

    all_ok =
        all_ok &&
        nvs_write_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_CHOSEN_DEVICE
        );

    err = nvs_set_u8(
        user_settings_handle,
        KEY_CONNECTION_COUNT,
        settings.target_active_connections
    );

    all_ok =
        all_ok &&
        nvs_write_check(
            CONFIG_STORAGE_TAG,
            err,
            KEY_CONNECTION_COUNT
        );


    // Release settings mutex


    xSemaphoreGive(settings.mutex);


    // Commit


    err = nvs_commit(
        user_settings_handle
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            CONFIG_STORAGE_TAG,
            "commit failed"
        );

        nvs_close(
            user_settings_handle
        );

        return false;
    }

    nvs_close(
        user_settings_handle
    );

    // Log saved device settings


    ESP_LOGI(
        CONFIG_STORAGE_TAG,
        "Autocatch saved: D1=%d D2=%d D3=%d D4=%d",
        settings.autocatch[0],
        settings.autocatch[1],
        settings.autocatch[2],
        settings.autocatch[3]
    );

    ESP_LOGI(
        CONFIG_STORAGE_TAG,
        "Autospin saved: D1=%d D2=%d D3=%d D4=%d",
        settings.autospin[0],
        settings.autospin[1],
        settings.autospin[2],
        settings.autospin[3]
    );

    return all_ok;
}