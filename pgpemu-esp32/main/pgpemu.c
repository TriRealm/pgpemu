#include "esp_system.h"
#include "esp_log.h"

#include "button_input.h"
#include "config_secrets.h"
#include "config_storage.h"
#include "led_output.h"
#include "log_tags.h"
#include "pgp_autobutton.h"
#include "pgp_bluetooth.h"
#include "pgp_gap.h"
#include "pgp_gatts.h"
#include "powerbank.h"
#include "secrets.h"
#include "settings.h"
#include "stats.h"
#include "uart.h"
#include "wifi_config.h"
#include "wifi.h"
#include "driver/gpio.h"


    void app_main()
{

    // RECOVERY MODE CHECK
    //
    // If pin 14 is held LOW for ~3s at boot, force Wi-Fi
    // configuration mode regardless of the use_button setting.
    // This guarantees a recovery path even if the button
    // was disabled in settings or was never enabled.
    //

    {
        gpio_config_t recovery_io_conf = {};
        recovery_io_conf.intr_type = GPIO_INTR_DISABLE;
        recovery_io_conf.pin_bit_mask = (1ULL << GPIO_NUM_14);
        recovery_io_conf.mode = GPIO_MODE_INPUT;
        recovery_io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        recovery_io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_config(&recovery_io_conf);

        if (gpio_get_level(GPIO_NUM_14) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(3000));

            if (gpio_get_level(GPIO_NUM_14) == 0)
            {
                ESP_LOGW(PGPEMU_TAG, "recovery: pin held at boot, forcing Wi-Fi config mode");

                init_config_storage();

                if (!init_wifi())
                {
                    ESP_LOGE(PGPEMU_TAG, "Wi-Fi initialization failed");
                }

                init_settings();
                read_stored_settings(false);
                settings_ready();

                if (!wifi_config_start())
                {
                    ESP_LOGE(PGPEMU_TAG, "failed to force-start Wi-Fi config mode");
                }

                while (wifi_config_is_active())
                {
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }

                esp_restart();
            }
        }
    }
    
    // uart menu. put it first because it purges all logs
    init_uart();

    // set log levels which let init msgs through
    log_levels_init();

    // check reset reason
    esp_reset_reason_t reset_reason = esp_reset_reason();
    ESP_LOGI(PGPEMU_TAG, "reset reason: %d", reset_reason);
    if (reset_reason == ESP_RST_BROWNOUT)
    {
        // keep it from bootlooping too quick when powering from low battery
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }

    // init nvs storage
    init_config_storage();

    if (!init_wifi())
{
    ESP_LOGE(
        PGPEMU_TAG,
        "Wi-Fi initialization failed"
    );
}

    // init settings mutex
    init_settings();
    // restore saved settings from nvs
    read_stored_settings(false);

    // initilise Wi-Fi
    //init_wifi();

    // restore log levels
    if (settings.verbose)
    {
        ESP_LOGI(PGPEMU_TAG, "log levels verbose");
        log_levels_max();
    }
    else
    {
        ESP_LOGI(PGPEMU_TAG, "log levels default");
        log_levels_min();
    }

    // rgb led
        // rgb led
    if (settings.use_led)
    {
        init_led_output();

        // pre-check: flash all LEDs white briefly to confirm they all work
        ESP_LOGI(PGPEMU_TAG, "LED self-test: flashing all LEDs");

        for (int i = 0; i < MAX_DEVICE_LEDS; i++)
        {
            show_rgb_event(i, true, true, true, 500);
        }

        vTaskDelay(pdMS_TO_TICKS(600));

        // show red
        show_rgb_event(0, true, false, false, 0);
    }
    else
    {
        ESP_LOGI(PGPEMU_TAG, "output led disabled");
    }

    // push button
    if (settings.use_button)
    {
        init_button_input();
    }
    else
    {
        ESP_LOGI(PGPEMU_TAG, "input button disabled");
    }

    // make sure we're not turned off
    init_powerbank();

    // read secrets from nvs (settings are safe to use because mutex is still locked)
    read_secrets_id(settings.chosen_device, PGP_CLONE_NAME, PGP_MAC, PGP_DEVICE_KEY, PGP_BLOB);

    if (!PGP_VALID())
    {
        // release mutex
        settings_ready();
        ESP_LOGE(PGPEMU_TAG, "NO PGP SECRETS AVAILABLE IN SLOT %d! Set them using secrets_upload.py or chose another using the 'X' menu!", settings.chosen_device);
        return;
    }

    // runtime counter
    init_stats();

    // start autobutton task
    if (!init_autobutton())
    {
        ESP_LOGI(PGPEMU_TAG, "creating button task failed");
        return;
    }

    // set clone mac and start bluetooth
    if (!init_bluetooth())
    {
        ESP_LOGI(PGPEMU_TAG, "bluetooth init failed");
        return;
    }

    // done
    ESP_LOGI(PGPEMU_TAG, "Device: %s", PGP_CLONE_NAME);
    ESP_LOGI(PGPEMU_TAG, "MAC: %02x:%02x:%02x:%02x:%02x:%02x",
             PGP_MAC[0], PGP_MAC[1], PGP_MAC[2],
             PGP_MAC[3], PGP_MAC[4], PGP_MAC[5]);
    ESP_LOGI(PGPEMU_TAG, "Ready.");

    // make settings available
    settings_ready();

    // show green for 1 s
    show_rgb_event(0, false, true, false, 1000);
    // show blue until someone connects
    show_rgb_event(0, false, false, true, 0);
}
