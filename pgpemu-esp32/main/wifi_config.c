#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "esp_http_server.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi_config.h"
#include "wifi.h"

#include "config_storage.h"
#include "log_tags.h"
#include "pgp_handshake_multi.h"
#include "settings.h"
#include "web_server.h"

static const char *TAG = "WIFI_CONFIG";

/*
 * CONFIGURATION
 */

#define WIFI_CONFIG_SSID       "PGP-EMU"
#define WIFI_CONFIG_PASSWORD   "PGPHollow"
#define WIFI_CONFIG_CHANNEL    1
#define WIFI_CONFIG_MAX_CONN   1

#define WIFI_CONFIG_IP         "192.168.4.1"

#define WIFI_CONFIG_TIMEOUT_MS (5 * 60 * 1000)

/*
 * STATE
 */

static bool wifi_config_active = false;

static esp_netif_t *wifi_ap_netif = NULL;
static httpd_handle_t http_server = NULL;

static TaskHandle_t wifi_timeout_task_handle = NULL;

/*
 * HELPERS
 */

static void url_decode(char *str)
{
    char *src = str;
    char *dst = str;

    while (*src)
    {
        if (*src == '+')
        {
            *dst++ = ' ';
            src++;
        }
        else if (*src == '%' &&
                 src[1] &&
                 src[2])
        {
            char hex[3];

            hex[0] = src[1];
            hex[1] = src[2];
            hex[2] = '\0';

            *dst++ = (char)strtol(hex, NULL, 16);
            src += 3;
        }
        else
        {
            *dst++ = *src++;
        }
    }

    *dst = '\0';
}

static bool form_get_value(
    const char *body,
    const char *key,
    char *output,
    size_t output_size)
{
    if (!body || !key || !output || output_size == 0)
    {
        return false;
    }

    char search[64];

    snprintf(search, sizeof(search), "%s=", key);

    const char *start = strstr(body, search);

    if (!start)
    {
        return false;
    }

    start += strlen(search);

    const char *end = strchr(start, '&');

    size_t len;

    if (end)
    {
        len = (size_t)(end - start);
    }
    else
    {
        len = strlen(start);
    }

    if (len >= output_size)
    {
        len = output_size - 1;
    }

    memcpy(output, start, len);
    output[len] = '\0';

    url_decode(output);

    return true;
}

static bool form_has_value(
    const char *body,
    const char *key)
{
    char value[16];

    return form_get_value(
        body,
        key,
        value,
        sizeof(value));
}

/*
 * HTML
 */

static const char *html_page =
"<!DOCTYPE html>"
"<html>"
"<head>"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
"<title>PGP Emulator</title>"
"<style>"
"body{"
"font-family:Arial,sans-serif;"
"background:#111;"
"color:#eee;"
"margin:0;"
"padding:20px;"
"}"
".container{"
"max-width:500px;"
"margin:auto;"
"}"
"h1{"
"font-size:28px;"
"}"
".card{"
"background:#222;"
"border-radius:12px;"
"padding:20px;"
"}"
"label{"
"display:block;"
"margin-top:18px;"
"margin-bottom:6px;"
"}"
"input[type=number],select{"
"width:100%%;"
"box-sizing:border-box;"
"padding:12px;"
"font-size:18px;"
"border-radius:8px;"
"border:1px solid #555;"
"background:#111;"
"color:#fff;"
"}"
".switch{"
"display:flex;"
"justify-content:space-between;"
"align-items:center;"
"margin:18px 0;"
"}"
"button{"
"width:100%%;"
"padding:14px;"
"margin-top:20px;"
"font-size:18px;"
"font-weight:bold;"
"border:0;"
"border-radius:8px;"
"background:#2ecc71;"
"color:#111;"
"}"
".info{"
"color:#aaa;"
"font-size:14px;"
"margin-top:15px;"
"}"
"</style>"
"</head>"
"<body>"
"<div class=\"container\">"
"<h1>PGP Emulator</h1>"
"<div class=\"card\">"
"<form method=\"POST\" action=\"/save\">"

"<label for=\"connections\">Target connections</label>"
"<select id=\"connections\" name=\"connections\">"
"%s"
"</select>"

"<div class=\"switch\">"
"<span>AutoCatch</span>"
"<input type=\"checkbox\" name=\"autocatch\" %s>"
"</div>"

"<div class=\"switch\">"
"<span>AutoSpin</span>"
"<input type=\"checkbox\" name=\"autospin\" %s>"
"</div>"

"<div class=\"switch\">"
"<span>Powerbank ping</span>"
"<input type=\"checkbox\" name=\"powerbank\" %s>"
"</div>"

"<div class=\"switch\">"
"<span>LED interactions</span>"
"<input type=\"checkbox\" name=\"ledinteractions\" %s>"
"</div>"

"<div class=\"switch\">"
"<span>Verbose logging</span>"
"<input type=\"checkbox\" name=\"verbose\" %s>"
"</div>"

"<button type=\"submit\">Save Settings</button>"

"</form>"

"<div class=\"info\">"
"Wi-Fi configuration mode automatically shuts down after 5 minutes."
"</div>"

"</div>"
"</div>"
"</body>"
"</html>";

static void send_settings_page(httpd_req_t *req)
{
    static char page[10000];

    char connection_options[512];

    connection_options[0] = '\0';

    int target = get_setting_uint8(
        &settings.target_active_connections);

    for (int i = 1; i <= CONFIG_BT_ACL_CONNECTIONS; i++)
    {
        char option[128];

        snprintf(
            option,
            sizeof(option),
            "<option value=\"%d\"%s>%d</option>",
            i,
            i == target ? " selected" : "",
            i);

        strlcat(
            connection_options,
            option,
            sizeof(connection_options));
    }

    uint8_t device = settings.chosen_device;

if (device >= MAX_PGP_DEVICES)
{
    ESP_LOGW(
        TAG,
        "invalid chosen device %u, using device 0",
        device);

    device = 0;
}

bool autocatch = get_device_autocatch(device);
bool autospin = get_device_autospin(device);
    bool powerbank = get_setting(&settings.powerbank_ping);
    bool ledinteractions = get_setting(&settings.led_interactions);
    bool verbose = get_setting(&settings.verbose);

    snprintf(
        page,
        sizeof(page),
        html_page,

        connection_options,

        autocatch ? "checked" : "",
        autospin ? "checked" : "",
        powerbank ? "checked" : "",
        ledinteractions ? "checked" : "",
        verbose ? "checked" : "");

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, page, HTTPD_RESP_USE_STRLEN);
}

/*
 * HTTP HANDLERS
 */

static esp_err_t index_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "configuration page requested");

    send_settings_page(req);

    return ESP_OK;
}

static esp_err_t save_handler(httpd_req_t *req)
{
    char body[2048];

    int received = httpd_req_recv(
        req,
        body,
        sizeof(body) - 1);

    if (received <= 0)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    body[received] = '\0';

    ESP_LOGI(TAG, "received configuration update");

    /*
     * CONNECTION COUNT
     */

    char connections_string[16];

    if (form_get_value(
            body,
            "connections",
            connections_string,
            sizeof(connections_string)))
    {
        int connections = atoi(connections_string);

        if (connections >= 1 &&
            connections <= CONFIG_BT_ACL_CONNECTIONS)
        {
            set_setting_uint8(
                &settings.target_active_connections,
                (uint8_t)connections);

            ESP_LOGI(
                TAG,
                "target connections = %d",
                connections);
        }
        else
        {
            ESP_LOGW(
                TAG,
                "invalid target connections: %d",
                connections);
        }
    }

    /*
     * BOOLEAN SETTINGS
     */

    uint8_t device = settings.chosen_device;

if (device >= MAX_PGP_DEVICES)
{
    ESP_LOGW(
        TAG,
        "invalid chosen device %u, saving to device 0",
        device);

    device = 0;
}

bool autocatch_enabled =
    form_has_value(body, "autocatch");

bool autospin_enabled =
    form_has_value(body, "autospin");

set_device_autocatch(
    device,
    autocatch_enabled);

set_device_autospin(
    device,
    autospin_enabled);

    set_setting_bool(
        &settings.powerbank_ping,
        form_has_value(body, "powerbank"));

    set_setting_bool(
        &settings.led_interactions,
        form_has_value(body, "ledinteractions"));

    set_setting_bool(
        &settings.verbose,
        form_has_value(body, "verbose"));

    /*
     * SAVE TO NVS
     */

    if (!write_config_storage())
    {
        ESP_LOGE(TAG, "failed to save settings");

        httpd_resp_set_status(
            req,
            "500 Internal Server Error");

        httpd_resp_send(
            req,
            "Failed to save settings.",
            HTTPD_RESP_USE_STRLEN);

        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "settings saved successfully");

    const char *response =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
        "<title>Saved</title>"
        "</head>"
        "<body style=\"font-family:Arial;text-align:center;padding:40px\">"
        "<h1>Settings saved!</h1>"
        "<p>Wi-Fi configuration mode will now shut down.</p>"
        "<p>You can disconnect from PGP-EMU.</p>"
        "</body>"
        "</html>";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(
        req,
        response,
        HTTPD_RESP_USE_STRLEN);

    /*
     * Give the HTTP response a moment to reach the phone
     * before shutting down Wi-Fi.
     */
    vTaskDelay(pdMS_TO_TICKS(500));

    wifi_config_stop();

    return ESP_OK;
}

/*
 * HTTP SERVER
 */

static bool start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.server_port = 80;
    config.max_uri_handlers = 8;

    config.stack_size = 8192;

    if (httpd_start(&http_server, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to start HTTP server");
        http_server = NULL;
        return false;
    }

    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL};

    httpd_uri_t save_uri = {
        .uri = "/save",
        .method = HTTP_POST,
        .handler = save_handler,
        .user_ctx = NULL};

    if (httpd_register_uri_handler(
            http_server,
            &index_uri) != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to register / handler");
        httpd_stop(http_server);
        http_server = NULL;
        return false;
    }

    if (httpd_register_uri_handler(
            http_server,
            &save_uri) != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to register /save handler");
        httpd_stop(http_server);
        http_server = NULL;
        return false;
    }

    ESP_LOGI(TAG, "HTTP server started");

    return true;
}

static void stop_http_server(void)
{
    if (http_server)
    {
        ESP_LOGI(TAG, "stopping HTTP server");

        httpd_stop(http_server);

        http_server = NULL;
    }
}

/*
 * TIMEOUT TASK
 */

static void wifi_timeout_task(void *arg)
{
    ESP_LOGI(
        TAG,
        "configuration timeout started: %d seconds",
        WIFI_CONFIG_TIMEOUT_MS / 1000);

    vTaskDelay(
        pdMS_TO_TICKS(WIFI_CONFIG_TIMEOUT_MS));

    if (wifi_config_active)
    {
        ESP_LOGI(
            TAG,
            "configuration timeout reached");

        wifi_config_stop();
    }

    wifi_timeout_task_handle = NULL;

    vTaskDelete(NULL);
}

/*
 * WIFI CONFIGURATION
 */

bool wifi_config_start(void)
{
    if (wifi_config_active)
    {
        ESP_LOGI(TAG, "Wi-Fi configuration already active");
        return true;
    }

    ESP_LOGI(TAG, "starting Wi-Fi configuration mode");

    /*
     * Initialise TCP/IP stack.
     *
     * These return ESP_ERR_INVALID_STATE if another component
     * has already initialised them, which is harmless.
     */

    esp_err_t err = esp_netif_init();

    if (err != ESP_OK &&
        err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(
            TAG,
            "esp_netif_init failed: %s",
            esp_err_to_name(err));

        return false;
    }

    err = esp_event_loop_create_default();

    if (err != ESP_OK &&
        err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(
            TAG,
            "event loop creation failed: %s",
            esp_err_to_name(err));

        return false;
    }

    /*
     * Initialise Wi-Fi driver.
     */

        /*
     * Reuse the shared Wi-Fi driver init from wifi.c.
     * It's safe to call even if Wi-Fi is already
     * initialized (e.g. from app_main at boot) - it
     * checks its own internal state and just returns
     * true without re-initializing.
     */

    if (!init_wifi())
    {
        ESP_LOGE(
            TAG,
            "failed to initialize Wi-Fi driver");

        return false;
    }

    if (!wifi_ap_netif)
    {
        wifi_ap_netif =
            esp_netif_create_default_wifi_ap();

        if (!wifi_ap_netif)
        {
            ESP_LOGE(
                TAG,
                "failed to create Wi-Fi AP interface");

            return false;
        }
    }

    wifi_config_t ap_config = {
        .ap = {
            .ssid = WIFI_CONFIG_SSID,
            .ssid_len = strlen(WIFI_CONFIG_SSID),
            .channel = WIFI_CONFIG_CHANNEL,
            .password = WIFI_CONFIG_PASSWORD,
            .max_connection = WIFI_CONFIG_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    err = esp_wifi_set_mode(WIFI_MODE_AP);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "failed to set AP mode: %s",
            esp_err_to_name(err));

        return false;
    }

    err = esp_wifi_set_config(
        WIFI_IF_AP,
        &ap_config);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "failed to configure AP: %s",
            esp_err_to_name(err));

        return false;
    }

    err = esp_wifi_start();

    if (err != ESP_OK &&
        err != ESP_ERR_WIFI_STATE)
    {
        ESP_LOGE(
            TAG,
            "failed to start Wi-Fi: %s",
            esp_err_to_name(err));

        return false;
    }

        /*
     * Start HTTP server (the 4-device web UI).
     */

    if (!web_server_start())
    {
        esp_wifi_stop();
        return false;
    }

    wifi_config_active = true;

    /*
     * Start automatic timeout.
     */

    if (xTaskCreate(
            wifi_timeout_task,
            "wifi_config_timeout",
            2048,
            NULL,
            5,
            &wifi_timeout_task_handle) != pdPASS)
    {
        ESP_LOGE(
            TAG,
            "failed to create Wi-Fi timeout task");

        web_server_stop();
        esp_wifi_stop();

        wifi_config_active = false;

        return false;
    }

    ESP_LOGI(TAG, "====================================");
    ESP_LOGI(TAG, "Wi-Fi configuration mode ACTIVE");
    ESP_LOGI(TAG, "SSID: %s", WIFI_CONFIG_SSID);
    ESP_LOGI(TAG, "Password: %s", WIFI_CONFIG_PASSWORD);
    ESP_LOGI(TAG, "Open: http://%s", WIFI_CONFIG_IP);
    ESP_LOGI(TAG, "====================================");

    return true;
}

void wifi_config_stop(void)
{
    if (!wifi_config_active)
    {
        ESP_LOGI(TAG, "Wi-Fi configuration already stopped");
        return;
    }

    ESP_LOGI(TAG, "stopping Wi-Fi configuration mode");

    wifi_config_active = false;

    web_server_stop();

    if (wifi_timeout_task_handle)
    {

        if (xTaskGetCurrentTaskHandle() !=
            wifi_timeout_task_handle)
        {
            vTaskDelete(wifi_timeout_task_handle);
        }

        wifi_timeout_task_handle = NULL;
    }

    esp_err_t err = esp_wifi_stop();

    if (err != ESP_OK &&
        err != ESP_ERR_WIFI_NOT_INIT &&
        err != ESP_ERR_WIFI_NOT_STARTED)
    {
        ESP_LOGW(
            TAG,
            "Wi-Fi stop returned: %s",
            esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "Wi-Fi configuration mode stopped");
}

bool wifi_config_is_active(void)
{
    return wifi_config_active;
}