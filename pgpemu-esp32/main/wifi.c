#include "wifi.h"

#include "freertos/FreeRTOS.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

static const char *TAG = "WIFI";

static bool wifi_initialized = false;
static bool wifi_running = false;


/*
 * ============================================================
 * INITIALIZE
 * ============================================================
 *
 * Initializes the ESP32 networking stack and Wi-Fi driver.
 *
 * IMPORTANT:
 *
 * This does NOT start Wi-Fi.
 *
 * It also does not configure AP/STA mode.
 *
 * The caller is responsible for configuring the desired
 * Wi-Fi mode and credentials before calling wifi_start().
 *
 * This allows wifi_config.c to temporarily configure an AP
 * without having two different pieces of code trying to
 * initialize the Wi-Fi driver.
 *
 * ============================================================
 */

bool init_wifi()
{
    if (wifi_initialized)
    {
        ESP_LOGW(
            TAG,
            "Wi-Fi already initialized"
        );

        return true;
    }

    /*
     * Initialize TCP/IP networking.
     */
    esp_err_t ret = esp_netif_init();

    if (ret != ESP_OK &&
        ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(
            TAG,
            "esp_netif_init failed: %s",
            esp_err_to_name(ret)
        );

        return false;
    }

    /*
     * Create the default event loop.
     *
     * ESP_ERR_INVALID_STATE simply means another component
     * has already created it.
     */
    ret = esp_event_loop_create_default();

    if (ret != ESP_OK &&
        ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(
            TAG,
            "event loop creation failed: %s",
            esp_err_to_name(ret)
        );

        return false;
    }

    /*
     * Initialize the Wi-Fi driver.
     */
    wifi_init_config_t wifi_config =
        WIFI_INIT_CONFIG_DEFAULT();

    ret = esp_wifi_init(&wifi_config);

    if (ret != ESP_OK &&
        ret != ESP_ERR_WIFI_INIT_STATE)
    {
        ESP_LOGE(
            TAG,
            "esp_wifi_init failed: %s",
            esp_err_to_name(ret)
        );

        return false;
    }

    wifi_initialized = true;

    ESP_LOGI(
        TAG,
        "Wi-Fi driver initialized"
    );

    return true;
}


/*
 * ============================================================
 * START
 * ============================================================
 *
 * Starts the Wi-Fi radio/driver.
 *
 * The caller must configure the Wi-Fi mode first, for example:
 *
 *     esp_wifi_set_mode(WIFI_MODE_AP);
 *
 * followed by:
 *
 *     esp_wifi_set_config(...);
 *
 * ============================================================
 */

bool wifi_start()
{
    if (!wifi_initialized)
    {
        ESP_LOGE(
            TAG,
            "cannot start Wi-Fi: driver not initialized"
        );

        return false;
    }

    if (wifi_running)
    {
        ESP_LOGW(
            TAG,
            "Wi-Fi already running"
        );

        return true;
    }

    esp_err_t ret = esp_wifi_start();

    if (ret == ESP_OK)
    {
        wifi_running = true;

        ESP_LOGI(
            TAG,
            "Wi-Fi started"
        );

        return true;
    }

    /*
     * If ESP-IDF says Wi-Fi is already started, keep our
     * internal state consistent.
     */
    if (ret == ESP_ERR_WIFI_STATE)
    {
        wifi_running = true;

        ESP_LOGW(
            TAG,
            "Wi-Fi was already started"
        );

        return true;
    }

    ESP_LOGE(
        TAG,
        "failed to start Wi-Fi: %s",
        esp_err_to_name(ret)
    );

    return false;
}


/*
 * STOP
 * ------------------------------------------------------
 *
 * Stops the Wi-Fi radio.
 *
 * The Wi-Fi driver remains initialized.
 *
 * This is important for configuration mode:
 *
 *     start Wi-Fi
 *     run web server
 *     save settings
 *     stop Wi-Fi
 *
 * The ESP32 does NOT need to reinitialize the Wi-Fi driver
 * every time the configuration panel is opened.
 *
 */

bool wifi_stop()
{
    if (!wifi_initialized)
    {
        ESP_LOGW(
            TAG,
            "Wi-Fi driver is not initialized"
        );

        return true;
    }

    if (!wifi_running)
    {
        return true;
    }

    esp_err_t ret = esp_wifi_stop();

    if (ret == ESP_OK)
    {
        wifi_running = false;

        ESP_LOGI(
            TAG,
            "Wi-Fi stopped"
        );

        return true;
    }

    /*
     * Already stopped.
     */
    if (ret == ESP_ERR_WIFI_NOT_STARTED)
    {
        wifi_running = false;

        return true;
    }

    ESP_LOGE(
        TAG,
        "failed to stop Wi-Fi: %s",
        esp_err_to_name(ret)
    );

    return false;
}


/*
 * STATE
 */

bool wifi_is_initialized()
{
    return wifi_initialized;
}


bool wifi_is_running()
{
    return wifi_running;
}


/*
 * SCANNING
 */

bool wifi_scan_start()
{
    if (!wifi_initialized)
    {
        ESP_LOGE(
            TAG,
            "cannot scan: Wi-Fi driver not initialized"
        );

        return false;
    }

    if (!wifi_running)
    {
        ESP_LOGE(
            TAG,
            "cannot scan: Wi-Fi is not running"
        );

        return false;
    }

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false
    };

    esp_err_t ret = esp_wifi_scan_start(
        &scan_config,
        true
    );

    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Wi-Fi scan failed: %s",
            esp_err_to_name(ret)
        );

        return false;
    }

    ESP_LOGI(
        TAG,
        "Wi-Fi scan completed"
    );

    return true;
}


bool wifi_scan_stop()
{
    if (!wifi_initialized)
    {
        return false;
    }

    esp_err_t ret = esp_wifi_scan_stop();

    if (ret == ESP_OK)
    {
        return true;
    }

    if (ret == ESP_ERR_WIFI_NOT_STARTED)
    {
        return true;
    }

    ESP_LOGW(
        TAG,
        "failed to stop Wi-Fi scan: %s",
        esp_err_to_name(ret)
    );

    return false;
}