#ifndef CONFIG_STORAGE_H
#define CONFIG_STORAGE_H

#include <stdbool.h>

// Initialize NVS partition
void init_config_storage();

// Read settings from NVS
void read_stored_settings(bool use_mutex);

// Write current settings to NVS
bool write_config_storage();

#endif /* CONFIG_STORAGE_H */