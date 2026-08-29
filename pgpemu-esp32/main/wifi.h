#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>

bool init_wifi();

bool wifi_start();

bool wifi_stop();

bool wifi_is_initialized();

bool wifi_is_running();

bool wifi_scan_start();

bool wifi_scan_stop();

#endif /* WIFI_H */