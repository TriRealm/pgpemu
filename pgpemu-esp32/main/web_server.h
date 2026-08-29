#ifndef WEB_SERVER_H
#define WEB_SERVER_H
#include "config_storage.h"

#include <stdbool.h>

bool web_server_start();
void web_server_stop();
bool web_server_is_active();

#endif