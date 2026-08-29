#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "button_input.h"

#include "pgp_handshake_multi.h"
#include "pgp_gap.h"
#include "wifi_config.h"

#include "led_output.h"
#include "log_tags.h"
#include "settings.h"

static const int CONFIG_GPIO_INPUT_BUTTON0 = GPIO_NUM_14;

/*
 * Hold the button for this long to enter Wi-Fi
 * configuration mode.
 */
#define WIFI_CONFIG_LONG_PRESS_MS 3000

static void button_input_task(void *pvParameters);

static QueueHandle_t button_input_queue;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;

    xQueueSendFromISR(
        button_input_queue,
        &gpio_num,
        NULL);
}

void init_button_input()
{
    /*
     * Create a queue to handle GPIO events from the ISR.
     *
     * Size 1 means additional button interrupts are dropped
     * while the current press is being processed.
     */
    button_input_queue =
        xQueueCreate(
            1,
            sizeof(uint32_t));

    gpio_config_t io_conf = {};

    /*
     * Button is active-low:
     *
     * HIGH = released
     * LOW  = pressed
     */
    io_conf.intr_type = GPIO_INTR_NEGEDGE;

    io_conf.pin_bit_mask =
        (1ULL << CONFIG_GPIO_INPUT_BUTTON0);

    io_conf.mode = GPIO_MODE_INPUT;

    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;

    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    gpio_config(&io_conf);

    /*
     * Install GPIO ISR service.
     */
    gpio_install_isr_service(0);

    /*
     * Hook ISR to our button.
     */
    gpio_isr_handler_add(
        CONFIG_GPIO_INPUT_BUTTON0,
        gpio_isr_handler,
        (void *)CONFIG_GPIO_INPUT_BUTTON0);

    /*
     * Start button task.
     */
    xTaskCreate(
        button_input_task,
        "button_input",
        3072,
        NULL,
        15,
        NULL);
}

static void button_input_task(void *pvParameters)
{
    uint32_t button_event;

    ESP_LOGI(
        BUTTON_INPUT_TAG,
        "button task started");

    while (true)
    {
        /*
         * Wait indefinitely for a button press.
         */
        if (xQueueReceive(
                button_input_queue,
                &button_event,
                portMAX_DELAY))
        {
            ESP_LOGV(
                BUTTON_INPUT_TAG,
                "button0 down");

            /*
             * Initial debounce.
             */
            vTaskDelay(
                pdMS_TO_TICKS(50));

            /*
             * If the button has already been released,
             * this was just contact bounce.
             */
            if (gpio_get_level(
                    CONFIG_GPIO_INPUT_BUTTON0) != 0)
            {
                continue;
            }


            ESP_LOGD(
                BUTTON_INPUT_TAG,
                "button0 pressed");

            /*
             * Wait to see whether this becomes a long press.
             */
            vTaskDelay(
                pdMS_TO_TICKS(
                    WIFI_CONFIG_LONG_PRESS_MS));

            /*
             * LONG PRESS
             *
             * Still LOW after 3 seconds.
             */

            if (gpio_get_level(
                    CONFIG_GPIO_INPUT_BUTTON0) == 0)
            {
                ESP_LOGI(
                    BUTTON_INPUT_TAG,
                    "button -> long press, entering Wi-Fi configuration");

                /*
                 * Visual feedback.
                 */
                show_rgb_event(
                    0,
                    true,
                    true,
                    true,
                    500);

                if (!wifi_config_is_active())
                {
                    if (!wifi_config_start())
                    {
                        ESP_LOGE(
                            BUTTON_INPUT_TAG,
                            "failed to start Wi-Fi configuration");

                        show_rgb_event(
                            0,
                            true,
                            false,
                            false,
                            1000);
                    }
                    else
                    {
                        /*
                         * Green means configuration mode is active.
                         */
                        show_rgb_event(
                            0,
                            false,
                            true,
                            false,
                            0);
                    }
                }

                /*
                 * Wait until the button is released.
                 *
                 * This prevents one long press from generating
                 * another button event.
                 */
                while (gpio_get_level(
                           CONFIG_GPIO_INPUT_BUTTON0) == 0)
                {
                    vTaskDelay(
                        pdMS_TO_TICKS(50));
                }

                /*
                 * Debounce release.
                 */
                vTaskDelay(
                    pdMS_TO_TICKS(100));

                continue;
            }

            /*
             * SHORT PRESS
             *
             * Button was released before 3 seconds.
             */

            ESP_LOGI(
                BUTTON_INPUT_TAG,
                "button -> short press");

            int active_connections =
                get_active_connections();

            /*
             * If Wi-Fi configuration mode is active,
             * don't allow a short press to alter BLE
             * advertising.
             */
            if (wifi_config_is_active())
            {
                ESP_LOGI(
                    BUTTON_INPUT_TAG,
                    "Wi-Fi configuration active, ignoring BLE toggle");

                continue;
            }

            /*
             * BLE ADVERTISING TOGGLE
             */

            if (pgp_is_advertising())
            {
                /*
                 * Currently advertising.
                 *
                 * Stop advertising regardless of how many
                 * connections we currently have.
                 */
                ESP_LOGI(
                    BUTTON_INPUT_TAG,
                    "button -> stop advertising");

                pgp_advertise_stop();

                show_rgb_event(
                    0,
                    false,
                    false,
                    false,
                    0);
            }
            else if (active_connections <
                     CONFIG_BT_ACL_CONNECTIONS)
            {
                /*
                 * Not advertising and there is still physical
                 * Bluetooth connection capacity.
                 */
                ESP_LOGI(
                    BUTTON_INPUT_TAG,
                    "button -> start advertising");

                pgp_advertise();

                show_rgb_event(
                    0,
                    false,
                    false,
                    true,
                    0);
            }
            else
            {
                /*
                 * Hardware connection limit reached.
                 */
                ESP_LOGW(
                    BUTTON_INPUT_TAG,
                    "button -> max. BT connections reached");

                show_rgb_event(
                    0,
                    true,
                    false,
                    false,
                    200);
            }
        }
    }

    vTaskDelete(NULL);
}