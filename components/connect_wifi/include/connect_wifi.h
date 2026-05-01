#ifndef CONNECT_WIFI_H
#define CONNECT_WIFI_H

#include <stdbool.h>

extern bool got_ip;
void wifi_init(void);
void wifi_connect(const char *ssid, const char *password);
void wifi_disconnect(void);
void wifi_driver_init(void);
#endif