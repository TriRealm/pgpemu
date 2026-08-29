#include <stdbool.h>

#include "esp_log.h"

#include "pgp_gap.h"

#include "log_tags.h"
#include "pgp_handshake_multi.h"
#include "settings.h"

uint8_t adv_config_done = 0;

static bool is_advertising = false;

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .peer_addr = {0},
    .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

bool pgp_is_advertising(void)
{
    return is_advertising;
}

void advertise_if_needed()
{
    int target_active_connections =
        get_setting_uint8(
            &settings.target_active_connections
        );

    int active_connections =
        get_active_connections();

    if (active_connections < target_active_connections)
    {
        pgp_advertise();
    }
    else
    {
        ESP_LOGI(
            BT_GAP_TAG,
            "target of %d connections reached, stopping advertising",
            target_active_connections
        );

        pgp_advertise_stop();
    }
}

void pgp_advertise()
{
    if (is_advertising)
    {
        ESP_LOGI(
            BT_GAP_TAG,
            "already advertising"
        );

        return;
    }

    if (get_active_connections() >= CONFIG_BT_ACL_CONNECTIONS)
    {
        ESP_LOGW(
            BT_GAP_TAG,
            "cannot advertise: maximum BT connections reached"
        );

        return;
    }

    ESP_LOGI(
        BT_GAP_TAG,
        "starting advertising"
    );

    esp_err_t err =
        esp_ble_gap_start_advertising(
            &adv_params
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            BT_GAP_TAG,
            "esp_ble_gap_start_advertising failed: %s",
            esp_err_to_name(err)
        );
    }
}

void pgp_advertise_stop()
{
    if (!is_advertising)
    {
        ESP_LOGI(
            BT_GAP_TAG,
            "already not advertising"
        );

        return;
    }

    ESP_LOGI(
        BT_GAP_TAG,
        "stopping advertising"
    );

    esp_err_t err =
        esp_ble_gap_stop_advertising();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            BT_GAP_TAG,
            "esp_ble_gap_stop_advertising failed: %s",
            esp_err_to_name(err)
        );
    }
}

void gap_event_handler(
    esp_gap_ble_cb_event_t event,
    esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:

            adv_config_done &= (~ADV_CONFIG_FLAG);

            if (adv_config_done == 0)
            {
                pgp_advertise();
            }

            break;

        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:

            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);

            if (adv_config_done == 0)
            {
                pgp_advertise();
            }

            break;

        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:

            if (param->adv_start_cmpl.status !=
                ESP_BT_STATUS_SUCCESS)
            {
                ESP_LOGE(
                    BT_GAP_TAG,
                    "advertising start failed"
                );

                is_advertising = false;
            }
            else
            {
                is_advertising = true;

                ESP_LOGI(
                    BT_GAP_TAG,
                    "advertising start successful"
                );
            }

            break;

        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:

            if (param->adv_stop_cmpl.status !=
                ESP_BT_STATUS_SUCCESS)
            {
                ESP_LOGE(
                    BT_GAP_TAG,
                    "advertising stop failed"
                );
            }
            else
            {
                is_advertising = false;

                ESP_LOGI(
                    BT_GAP_TAG,
                    "advertising stop successful"
                );
            }

            break;

        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:

            ESP_LOGI(
                BT_GAP_TAG,
                "update conn params status=%d, min_int=%d, max_int=%d, conn_int=%d, latency=%d, timeout=%d",
                param->update_conn_params.status,
                param->update_conn_params.min_int,
                param->update_conn_params.max_int,
                param->update_conn_params.conn_int,
                param->update_conn_params.latency,
                param->update_conn_params.timeout
            );

            break;

        default:
            break;
    }
}