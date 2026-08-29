#include "led_output.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "driver/gpio.h"
#include "led_strip.h"
#include "settings.h"

#include "log_tags.h"


#define WS2812_GPIO        GPIO_NUM_27


typedef struct
{
    int device_slot;

    uint8_t red;
    uint8_t green;
    uint8_t blue;

    int duration_ms;

} LedEvent;


static QueueHandle_t led_queue = NULL;

static led_strip_handle_t led_strip = NULL;

static bool led_ready = false;



static void led_output_task(void *pvParameters);

static void apply_led(
    int device_slot,
    uint8_t red,
    uint8_t green,
    uint8_t blue
);

static void clear_led(
    int device_slot
);




void init_led_output()
{
    ESP_LOGI(
        LEDOUTPUT_TAG,
        "Initializing WS2812B LED output"
    );


    led_strip_config_t strip_config =
    {
        .strip_gpio_num = WS2812_GPIO,
        .max_leds = MAX_DEVICE_LEDS,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags.invert_out = false,
    };


    led_strip_rmt_config_t rmt_config =
    {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags.with_dma = false,
    };


    ESP_ERROR_CHECK(
        led_strip_new_rmt_device(
            &strip_config,
            &rmt_config,
            &led_strip
        )
    );


    ESP_ERROR_CHECK(
        led_strip_clear(led_strip)
    );


    led_queue = xQueueCreate(
        16,
        sizeof(LedEvent)
    );


    if (!led_queue)
    {
        ESP_LOGE(
            LEDOUTPUT_TAG,
            "Failed to create LED queue"
        );

        return;
    }


    led_ready = true;


    xTaskCreate(
        led_output_task,
        "led_output_task",
        4096,
        NULL,
        14,
        NULL
    );


    ESP_LOGI(
        LEDOUTPUT_TAG,
        "WS2812B ready: %d LEDs on GPIO %d",
        MAX_DEVICE_LEDS,
        WS2812_GPIO
    );
}




void set_device_led(
    int device_slot,
    bool red,
    bool green,
    bool blue
)
{
    if (!led_ready)
    {
        return;
    }


    if (
        device_slot < 0 ||
        device_slot >= MAX_DEVICE_LEDS
    )
    {
        ESP_LOGE(
            LEDOUTPUT_TAG,
            "Invalid LED slot %d",
            device_slot
        );

        return;
    }


    apply_led(
        device_slot,

        red ? 255 : 0,
        green ? 255 : 0,
        blue ? 255 : 0
    );
}



void clear_device_led(
    int device_slot
)
{
    if (!led_ready)
    {
        return;
    }


    if (
        device_slot < 0 ||
        device_slot >= MAX_DEVICE_LEDS
    )
    {
        return;
    }


    clear_led(device_slot);
}




void clear_all_leds()
{
    if (!led_ready)
    {
        return;
    }


    ESP_ERROR_CHECK(
        led_strip_clear(led_strip)
    );
}




void show_rgb_event(
    int device_slot,
    bool red,
    bool green,
    bool blue,
    int duration_ms
)
{
    if (!led_ready)
    {
        return;
    }


    if (
        device_slot < 0 ||
        device_slot >= MAX_DEVICE_LEDS
    )
    {
        ESP_LOGE(
            LEDOUTPUT_TAG,
            "Invalid LED slot %d",
            device_slot
        );

        return;
    }


    LedEvent event =
    {
        .device_slot = device_slot,

        .red = red ? 255 : 0,
        .green = green ? 255 : 0,
        .blue = blue ? 255 : 0,

        .duration_ms = duration_ms
    };


    ESP_LOGD(
        LEDOUTPUT_TAG,
        "Queue LED event: Device %d RGB=(%d,%d,%d)",
        device_slot + 1,
        event.red,
        event.green,
        event.blue
    );


    xQueueSend(
        led_queue,
        &event,
        0
    );
}



static void apply_led(
    int device_slot,
    uint8_t red,
    uint8_t green,
    uint8_t blue
)
{
    if (!led_strip)
    {
        return;
    }


    uint8_t brightness = get_led_brightness();


    uint8_t scaled_red =
        (uint16_t)red * brightness / 255;

    uint8_t scaled_green =
        (uint16_t)green * brightness / 255;

    uint8_t scaled_blue =
        (uint16_t)blue * brightness / 255;


    ESP_ERROR_CHECK(
        led_strip_set_pixel(
            led_strip,
            device_slot,
            scaled_red,
            scaled_green,
            scaled_blue
        )
    );


    ESP_ERROR_CHECK(
        led_strip_refresh(led_strip)
    );
}




static void clear_led(
    int device_slot
)
{
    apply_led(
        device_slot,
        0,
        0,
        0
    );
}




static void led_output_task(
    void *pvParameters
)
{
    LedEvent event;


    while (true)
    {
        if (
            xQueueReceive(
                led_queue,
                &event,
                portMAX_DELAY
            )
        )
        {
            apply_led(
                event.device_slot,
                event.red,
                event.green,
                event.blue
            );


            if (event.duration_ms > 0)
            {
                vTaskDelay(
                    pdMS_TO_TICKS(
                        event.duration_ms
                    )
                );


                clear_led(
                    event.device_slot
                );
            }
        }
    }
}