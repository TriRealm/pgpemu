#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <stdbool.h>

bool wifi_config_start(void);
void wifi_config_stop(void);
bool wifi_config_is_active(void);

#endif /* WIFI_CONFIG_H */