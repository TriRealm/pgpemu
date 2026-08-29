#ifndef LED_OUTPUT_H
#define LED_OUTPUT_H

#include <stdbool.h>

#define MAX_DEVICE_LEDS    4

void init_led_output();

/*
 * device_slot:
 *
 * 0 = Device 1
 * 1 = Device 2
 * 2 = Device 3
 * 3 = Device 4
 */
void show_rgb_event(
    int device_slot,
    bool red,
    bool green,
    bool blue,
    int duration_ms
);

void set_device_led(
    int device_slot,
    bool red,
    bool green,
    bool blue
);

void clear_device_led(
    int device_slot
);

void clear_all_leds();

#endif