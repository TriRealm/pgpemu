#include <stdio.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"

#include "web_server.h"
#include "wifi_config.h"

#include "pgp_gap.h"
#include "pgp_handshake_multi.h"
#include "settings.h"

static const char *TAG = "WEB";

static httpd_handle_t server = NULL;

// HTML

static const char *INDEX_HTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>PGP-EMU</title>

<style>

* {
    box-sizing: border-box;
}

body {
    margin: 0;
    padding: 20px;
    background: #111;
    color: #eee;
    font-family: Arial, sans-serif;
}

.container {
    max-width: 500px;
    margin: auto;
}

h1 {
    text-align: center;
    margin-bottom: 5px;
}

.subtitle {
    text-align: center;
    color: #888;
    margin-bottom: 25px;
}

.card {
    background: #1d1d1d;
    border-radius: 12px;
    padding: 18px;
    margin-bottom: 15px;
}

.card h2 {
    margin-top: 0;
}

.row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px 0;
    border-bottom: 1px solid #333;
}

.row:last-child {
    border-bottom: none;
}

.value {
    font-weight: bold;
}

.status-on {
    color: #55dd77;
}

.status-off {
    color: #ff6666;
}

button,
select {
    width: 100%;
    padding: 12px;
    margin-top: 8px;
    border: 0;
    border-radius: 8px;
    font-size: 16px;
}

button {
    background: #333;
    color: white;
}

button:active {
    background: #555;
}

select {
    background: #333;
    color: white;
}

label {
    display: block;
    margin-top: 12px;
}

input[type="checkbox"] {
    transform: scale(1.4);
    float: right;
}

.device-settings {
    margin-top: 10px;
}

.device-row {
    display: grid;
    grid-template-columns: 1fr 80px 80px;
    align-items: center;
    padding: 12px 0;
    border-bottom: 1px solid #333;
}

.device-row:last-child {
    border-bottom: none;
}

.device-header {
    color: #888;
    font-size: 13px;
    padding-bottom: 6px;
}

.device-name {
    font-weight: bold;
}

.device-check {
    text-align: center;
}

.device-check input[type="checkbox"] {
    float: none;
    margin: 0;
}

.save {
    background: #2563eb;
}

.danger {
    background: #8b2020;
}

#message {
    text-align: center;
    margin-top: 15px;
    min-height: 20px;
}

</style>
</head>

<body>

<div class="container">

<h1>PGP-EMU</h1>
<div class="subtitle">Device Control</div>

<div class="card">

<h2>Bluetooth</h2>

<div class="row">
    <span>Connections</span>
    <span class="value" id="connections">-</span>
</div>

<div class="row">
    <span>Advertising</span>
    <span class="value" id="advertising">-</span>
</div>

<div class="row">
    <span>Target connections</span>

    <select id="target">
        <option value="1">1</option>
        <option value="2">2</option>
        <option value="3">3</option>
        <option value="4">4</option>
    </select>
</div>

<button onclick="startAdvertising()">
    Start Advertising
</button>

<button onclick="stopAdvertising()">
    Stop Advertising
</button>

</div>

<div class="card">

<h2>Features</h2>

<div class="device-settings">

    <div class="device-row device-header">
        <span>Device</span>
        <span class="device-check">Catch</span>
        <span class="device-check">Spin</span>
    </div>

    <div class="device-row">
        <span class="device-name">Device 1</span>

        <span class="device-check">
            <input
                type="checkbox"
                id="autocatch_0">
        </span>

        <span class="device-check">
            <input
                type="checkbox"
                id="autospin_0">
        </span>
    </div>

    <div class="device-row">
        <span class="device-name">Device 2</span>

        <span class="device-check">
            <input
                type="checkbox"
                id="autocatch_1">
        </span>

        <span class="device-check">
            <input
                type="checkbox"
                id="autospin_1">
        </span>
    </div>

    <div class="device-row">
        <span class="device-name">Device 3</span>

        <span class="device-check">
            <input
                type="checkbox"
                id="autocatch_2">
        </span>

        <span class="device-check">
            <input
                type="checkbox"
                id="autospin_2">
        </span>
    </div>

    <div class="device-row">
        <span class="device-name">Device 4</span>

        <span class="device-check">
            <input
                type="checkbox"
                id="autocatch_3">
        </span>

        <span class="device-check">
            <input
                type="checkbox"
                id="autospin_3">
        </span>
    </div>

</div>

<br>

<label>
    Powerbank Ping
    <input type="checkbox" id="powerbank_ping">
</label>

<br>

<label>
    Button
    <input type="checkbox" id="use_button">
</label>

<br>

<label>
    LED
    <input type="checkbox" id="use_led">
</label>

<br>

<label>
    LED Brightness
    <input type="range" id="led_brightness" min="0" max="255" style="width:100%;">
</label>

<br>

<label>
    LED Interactions
    <input type="checkbox" id="led_interactions">
</label>

<br>

<label>
    Wi-Fi Network Name (SSID, 1-32 characters)
    <input type="text" id="wifi_ssid" maxlength="32" placeholder="PGP-EMU">
</label>
<div class="hint" style="font-size:12px;color:#888;margin-top:4px;">The network name broadcast for setup mode. Takes effect the next time Wi-Fi configuration mode starts.</div>

<br>

<label>
    Wi-Fi Config Password (8-63 characters)
    <input type="text" id="wifi_password" maxlength="63" placeholder="PGPHollow">
</label>
<div class="hint" style="font-size:12px;color:#888;margin-top:4px;">Used for the PGP-EMU setup network. Takes effect the next time Wi-Fi configuration mode starts.</div>

<br>

<label>
    Verbose Logging
    <input type="checkbox" id="verbose">
</label>

<button class="save" onclick="saveSettings()">
    Save Settings
</button>

</div>

<div id="message"></div>

</div>

<script>

// ============================================================
// UPDATE STATUS
// ============================================================

async function updateStatus()
{
    try
    {
        const response = await fetch('/api/status');

        if (!response.ok)
            return;

        const data = await response.json();

        document.getElementById('connections').textContent =
            data.connections + ' / ' + data.max_connections;

        const advertising =
            document.getElementById('advertising');

        advertising.textContent =
            data.advertising ? 'ON' : 'OFF';

        advertising.className =
            data.advertising
                ? 'value status-on'
                : 'value status-off';

        document.getElementById('target').value =
            data.target;
    }
    catch (e)
    {
        console.log(e);
    }
}


// LOAD SETTINGS


async function loadSettings()
{
    try
    {
        const response = await fetch('/api/settings');

        if (!response.ok)
            return;

        const data = await response.json();

        document.getElementById('target').value =
            data.target;

        // ----------------------------------------------------
        // Per-device settings
        // ----------------------------------------------------

        for (let i = 0; i < 4; i++)
        {
            document.getElementById(
                'autocatch_' + i
            ).checked = data.autocatch[i];

            document.getElementById(
                'autospin_' + i
            ).checked = data.autospin[i];
        }

        // ----------------------------------------------------
        // Global settings
        // ----------------------------------------------------

        document.getElementById('powerbank_ping').checked =
            data.powerbank_ping;

        document.getElementById('use_button').checked =
            data.use_button;

        document.getElementById('use_led').checked =
            data.use_led;

        document.getElementById('led_brightness').value =
            data.led_brightness;

        document.getElementById('led_interactions').checked =
            data.led_interactions;

        document.getElementById('wifi_ssid').value = 
            data.wifi_ssid;

        document.getElementById('wifi_password').value = 
            data.wifi_password;

        document.getElementById('verbose').checked =
            data.verbose;
    }
    catch (e)
    {
        console.log(e);
    }
}


// SAVE SETTINGS


async function saveSettings()
{
    const body = {

        target:
            parseInt(
                document.getElementById('target').value
            ),

        // ----------------------------------------------------
        // Device 1
        // ----------------------------------------------------

        autocatch_0:
            document.getElementById('autocatch_0').checked
                ? 1 : 0,

        autospin_0:
            document.getElementById('autospin_0').checked
                ? 1 : 0,

        // ----------------------------------------------------
        // Device 2
        // ----------------------------------------------------

        autocatch_1:
            document.getElementById('autocatch_1').checked
                ? 1 : 0,

        autospin_1:
            document.getElementById('autospin_1').checked
                ? 1 : 0,

        // ----------------------------------------------------
        // Device 3
        // ----------------------------------------------------

        autocatch_2:
            document.getElementById('autocatch_2').checked
                ? 1 : 0,

        autospin_2:
            document.getElementById('autospin_2').checked
                ? 1 : 0,

        // ----------------------------------------------------
        // Device 4
        // ----------------------------------------------------

        autocatch_3:
            document.getElementById('autocatch_3').checked
                ? 1 : 0,

        autospin_3:
            document.getElementById('autospin_3').checked
                ? 1 : 0,

        // ----------------------------------------------------
        // Global settings
        // ----------------------------------------------------

        powerbank_ping:
            document.getElementById('powerbank_ping').checked
                ? 1 : 0,

        use_button:
            document.getElementById('use_button').checked
                ? 1 : 0,

        use_led:
            document.getElementById('use_led').checked
                ? 1 : 0,

        led_brightness:
            parseInt(
                document.getElementById('led_brightness').value
            ),

        led_interactions:
            document.getElementById('led_interactions').checked
                ? 1 : 0,

        verbose:
            document.getElementById('verbose').checked
                ? 1 : 0,

        wifi_ssid:
            document.getElementById('wifi_ssid').value,

        wifi_password: 
            document.getElementById('wifi_password').value
    };

    try
    {
        const response = await fetch(
            '/api/settings',
            {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(body)
            }
        );

        const text = await response.text();

        showMessage(text);

        updateStatus();
    }
    catch (e)
    {
        console.log(e);

        showMessage(
            'Failed to save settings'
        );
    }
}

// ============================================================
// ADVERTISING
// ============================================================

async function startAdvertising()
{
    await fetch(
        '/api/advertising/start',
        {
            method: 'POST'
        }
    );

    updateStatus();
}

async function stopAdvertising()
{
    await fetch(
        '/api/advertising/stop',
        {
            method: 'POST'
        }
    );

    updateStatus();
}

// ============================================================
// MESSAGE
// ============================================================

function showMessage(message)
{
    document.getElementById('message').textContent =
        message;

    setTimeout(() => {
        document.getElementById('message').textContent = '';
    }, 2500);
}

// ============================================================
// START
// ============================================================

loadSettings();
updateStatus();

setInterval(
    updateStatus,
    2000
);

</script>

</body>
</html>
)rawliteral";

// ============================================================
// GET /
// ============================================================

static esp_err_t index_handler(
    httpd_req_t *req)
{
    httpd_resp_set_type(
        req,
        "text/html"
    );

    return httpd_resp_send(
        req,
        INDEX_HTML,
        HTTPD_RESP_USE_STRLEN
    );
}


// GET /api/status

static esp_err_t status_handler(
    httpd_req_t *req)
{
    int connections =
        get_active_connections();

    int target =
        get_setting_uint8(
            &settings.target_active_connections
        );

    char response[256];

    snprintf(
        response,
        sizeof(response),
        "{"
        "\"connections\":%d,"
        "\"max_connections\":%d,"
        "\"advertising\":%s,"
        "\"target\":%d"
        "}",
        connections,
        CONFIG_BT_ACL_CONNECTIONS,
        pgp_is_advertising() ? "true" : "false",
        target
    );

    httpd_resp_set_type(
        req,
        "application/json"
    );

    return httpd_resp_send(
        req,
        response,
        HTTPD_RESP_USE_STRLEN
    );
}




// GET /api/settings


static esp_err_t get_settings_handler(
    httpd_req_t *req)
{
    char response[1024];
    char wifi_ssid_buf[WIFI_SSID_MAX_LEN];
    get_wifi_ssid(wifi_ssid_buf, sizeof(wifi_ssid_buf));
    char wifi_password_buf[WIFI_PASSWORD_MAX_LEN];
    get_wifi_password(wifi_password_buf, sizeof(wifi_password_buf));

    snprintf(
        response,
        sizeof(response),
        "{"

        "\"target\":%d,"

        "\"autocatch\":[%s,%s,%s,%s],"

        "\"autospin\":[%s,%s,%s,%s],"

        "\"powerbank_ping\":%s,"

        "\"use_button\":%s,"

        "\"use_led\":%s,"

        "\"led_brightness\":%d,"

        "\"led_interactions\":%s,"

        "\"wifi_ssid\":\"%s\","

        "\"wifi_password\":\"%s\","

        "\"verbose\":%s"

        "}",

        
        // Target connections
        

        get_setting_uint8(
            &settings.target_active_connections
        ),

        
        // Autocatch
        

        get_device_autocatch(0)
            ? "true" : "false",

        get_device_autocatch(1)
            ? "true" : "false",

        get_device_autocatch(2)
            ? "true" : "false",

        get_device_autocatch(3)
            ? "true" : "false",

        
        // Autospin
        

        get_device_autospin(0)
            ? "true" : "false",

        get_device_autospin(1)
            ? "true" : "false",

        get_device_autospin(2)
            ? "true" : "false",

        get_device_autospin(3)
            ? "true" : "false",

        
        // Global settings
        

        get_setting(&settings.powerbank_ping)
            ? "true" : "false",

        get_setting(&settings.use_button)
            ? "true" : "false",

        get_setting(&settings.use_led)
            ? "true" : "false",
        
        get_led_brightness(),

        get_setting(&settings.led_interactions)
            ? "true" : "false",

        wifi_ssid_buf,

        wifi_password_buf,

        get_setting(&settings.verbose)
            ? "true" : "false"
    );

    httpd_resp_set_type(
        req,
        "application/json"
    );

    return httpd_resp_send(
        req,
        response,
        HTTPD_RESP_USE_STRLEN
    );
}


// POST /api/settings


static void deferred_wifi_config_stop_task(void *arg)
{

    vTaskDelay(pdMS_TO_TICKS(500));

    wifi_config_stop();

    vTaskDelete(NULL);
}

static bool json_get_string(const char *body, const char *key, char *out, size_t out_size)
{
    char search[48];
    snprintf(search, sizeof(search), "\"%s\":\"", key);

    const char *start = strstr(body, search);
    if (!start) return false;

    start += strlen(search);
    const char *end = strchr(start, '"');
    if (!end) return false;

    size_t len = (size_t)(end - start);
    if (len >= out_size) len = out_size - 1;

    memcpy(out, start, len);
    out[len] = '\0';

    return true;
}

static esp_err_t settings_handler(
    httpd_req_t *req)
{
    char body[1024];

    if (req->content_len <= 0 ||
        req->content_len >= sizeof(body))
    {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Invalid request"
        );

        return ESP_FAIL;
    }

    int received = httpd_req_recv(
        req,
        body,
        req->content_len
    );

    if (received <= 0)
    {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Could not read request"
        );

        return ESP_FAIL;
    }

    body[received] = '\0';

    
    // Settings received from web interface
    

    int target = 0;

    int autocatch_0 = 0;
    int autocatch_1 = 0;
    int autocatch_2 = 0;
    int autocatch_3 = 0;

    int autospin_0 = 0;
    int autospin_1 = 0;
    int autospin_2 = 0;
    int autospin_3 = 0;

    int powerbank_ping = 0;
    int use_button = 0;
    int use_led = 0;
    int led_brightness = 128;
    int led_interactions = 0;
    int verbose = 0;

    char wifi_ssid[WIFI_SSID_MAX_LEN] = {0};
    bool has_wifi_ssid = json_get_string(body, "wifi_ssid", wifi_ssid, sizeof(wifi_ssid));

    char wifi_password[WIFI_PASSWORD_MAX_LEN] = {0};
    bool has_wifi_password = json_get_string(body, "wifi_password", wifi_password, sizeof(wifi_password));

    
    // Parse JSON
    

    int parsed = sscanf(
        body,

        "{"
        "\"target\":%d,"

        "\"autocatch_0\":%d,"
        "\"autospin_0\":%d,"

        "\"autocatch_1\":%d,"
        "\"autospin_1\":%d,"

        "\"autocatch_2\":%d,"
        "\"autospin_2\":%d,"

        "\"autocatch_3\":%d,"
        "\"autospin_3\":%d,"

        "\"powerbank_ping\":%d,"
        "\"use_button\":%d,"
        "\"use_led\":%d,"
        "\"led_brightness\":%d,"
        "\"led_interactions\":%d,"
        "\"verbose\":%d"
        "}",

        &target,

        &autocatch_0,
        &autospin_0,

        &autocatch_1,
        &autospin_1,

        &autocatch_2,
        &autospin_2,

        &autocatch_3,
        &autospin_3,

        &powerbank_ping,
        &use_button,
        &use_led,
        &led_brightness,
        &led_interactions,
        &verbose
    );

    
    // Validate request
    

    if (parsed != 15)
    {
        ESP_LOGE(
            TAG,
            "Invalid settings JSON. Parsed %d/15 values",
            parsed
        );

        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Invalid settings"
        );

        return ESP_FAIL;
    }

        if (target < 1 ||
        target > CONFIG_BT_ACL_CONNECTIONS)
    {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Invalid target connection count"
        );

        return ESP_FAIL;
    }

    if (led_brightness < 0 ||
        led_brightness > 255)
    {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Invalid brightness value"
        );

        return ESP_FAIL;
    }

    if (has_wifi_ssid && strlen(wifi_ssid) == 0)
    {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Wi-Fi SSID cannot be empty"
        );

        return ESP_FAIL;
    }

    if (has_wifi_password &&
        strlen(wifi_password) > 0 &&
        strlen(wifi_password) < 8)
    {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Wi-Fi password must be at least 8 characters"
        );

        return ESP_FAIL;
    }

    
    // Apply target connection setting
    

    set_setting_uint8(
        &settings.target_active_connections,
        (uint8_t)target
    );

    
    // Apply per-device Autocatch settings
    

    set_device_autocatch(
        0,
        autocatch_0 != 0
    );

    set_device_autocatch(
        1,
        autocatch_1 != 0
    );

    set_device_autocatch(
        2,
        autocatch_2 != 0
    );

    set_device_autocatch(
        3,
        autocatch_3 != 0
    );

    
    // Apply per-device Autospin settings
    

    set_device_autospin(
        0,
        autospin_0 != 0
    );

    set_device_autospin(
        1,
        autospin_1 != 0
    );

    set_device_autospin(
        2,
        autospin_2 != 0
    );

    set_device_autospin(
        3,
        autospin_3 != 0
    );

    
    // Apply global settings
    

    set_setting_bool(
        &settings.powerbank_ping,
        powerbank_ping != 0
    );

    set_setting_bool(
        &settings.use_button,
        use_button != 0
    );

    set_setting_bool(
        &settings.use_led,
        use_led != 0
    );

    set_setting_bool(
        &settings.led_interactions,
        led_interactions != 0
    );

    set_setting_bool(
        &settings.verbose,
        verbose != 0
    );

    set_led_brightness((uint8_t)led_brightness);

    if (has_wifi_ssid && strlen(wifi_ssid) > 0)
    {
        set_wifi_ssid(wifi_ssid);
    }

    if (has_wifi_password && strlen(wifi_password) > 0)
    {
        set_wifi_password(wifi_password);
    }

    
    // Logging
    

    ESP_LOGI(
        TAG,
        "Settings updated:"
    );

    ESP_LOGI(
        TAG,
        "  Target connections: %d",
        target
    );

    ESP_LOGI(
        TAG,
        "  Device 1: autocatch=%d autospin=%d",
        autocatch_0,
        autospin_0
    );

    ESP_LOGI(
        TAG,
        "  Device 2: autocatch=%d autospin=%d",
        autocatch_1,
        autospin_1
    );

    ESP_LOGI(
        TAG,
        "  Device 3: autocatch=%d autospin=%d",
        autocatch_2,
        autospin_2
    );

    ESP_LOGI(
        TAG,
        "  Device 4: autocatch=%d autospin=%d",
        autocatch_3,
        autospin_3
    );

    ESP_LOGI(
        TAG,
        "  Powerbank Ping: %d",
        powerbank_ping
    );

    ESP_LOGI(
        TAG,
        "  Button: %d",
        use_button
    );

    ESP_LOGI(
        TAG,
        "  LED: %d",
        use_led
    );

    ESP_LOGI(
        TAG,
        "  LED interactions: %d",
        led_interactions
    );

    ESP_LOGI(
        TAG,
        "  Verbose: %d",
        verbose
    );

    
    // Persist settings to NVS
    

    if (!write_config_storage())
    {
        ESP_LOGE(
            TAG,
            "Failed to save settings to NVS"
        );

        return httpd_resp_send_err(
            req,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Failed to save settings"
        );
    }

        ESP_LOGI(
        TAG,
        "Settings saved to NVS"
    );

    httpd_resp_set_type(
        req,
        "text/plain"
    );

        esp_err_t send_ret = httpd_resp_sendstr(
        req,
        "Settings saved. Wi-Fi configuration mode will now shut down."
    );

    xTaskCreate(
        deferred_wifi_config_stop_task,
        "wifi_stop_deferred",
        2048,
        NULL,
        5,
        NULL
    );

    return send_ret;
}


// POST /api/advertising/start


static esp_err_t advertising_start_handler(
    httpd_req_t *req)
{
    ESP_LOGI(
        TAG,
        "Web interface requested advertising start"
    );

    pgp_advertise();

    return httpd_resp_sendstr(
        req,
        "Advertising started"
    );
}


// POST /api/advertising/stop


static esp_err_t advertising_stop_handler(
    httpd_req_t *req)
{
    ESP_LOGI(
        TAG,
        "Web interface requested advertising stop"
    );

    pgp_advertise_stop();

    return httpd_resp_sendstr(
        req,
        "Advertising stopped"
    );
}


// START SERVER


bool web_server_start()
{
    if (server != NULL)
    {
        ESP_LOGW(
            TAG,
            "Web server already running"
        );

        return true;
    }

    httpd_config_t config =
        HTTPD_DEFAULT_CONFIG();

    config.max_uri_handlers = 16;

    esp_err_t ret =
        httpd_start(
            &server,
            &config
        );

    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to start HTTP server: %s",
            esp_err_to_name(ret)
        );

        server = NULL;

        return false;
    }

    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL
    };

    httpd_uri_t status_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL
    };

    httpd_uri_t get_settings_uri = {
        .uri = "/api/settings",
        .method = HTTP_GET,
        .handler = get_settings_handler,
        .user_ctx = NULL
    };

    httpd_uri_t settings_uri = {
        .uri = "/api/settings",
        .method = HTTP_POST,
        .handler = settings_handler,
        .user_ctx = NULL
    };

    httpd_uri_t advertising_start_uri = {
        .uri = "/api/advertising/start",
        .method = HTTP_POST,
        .handler = advertising_start_handler,
        .user_ctx = NULL
    };

    httpd_uri_t advertising_stop_uri = {
        .uri = "/api/advertising/stop",
        .method = HTTP_POST,
        .handler = advertising_stop_handler,
        .user_ctx = NULL
    };

    httpd_register_uri_handler(
        server,
        &index_uri
    );

    httpd_register_uri_handler(
        server,
        &status_uri
    );

    httpd_register_uri_handler(
        server,
        &get_settings_uri
    );

    httpd_register_uri_handler(
        server,
        &settings_uri
    );

    httpd_register_uri_handler(
        server,
        &advertising_start_uri
    );

    httpd_register_uri_handler(
        server,
        &advertising_stop_uri
    );

    ESP_LOGI(
        TAG,
        "Web server started"
    );

    return true;
}

// STOP SERVER


void web_server_stop()
{
    if (server == NULL)
    {
        return;
    }

    httpd_stop(server);

    server = NULL;

    ESP_LOGI(
        TAG,
        "Web server stopped"
    );
}

bool web_server_is_active()
{
    return server != NULL;
}