#ifndef CONNECT_WIFI_H
#define CONNECT_WIFI_H

#include <stdbool.h>

extern bool got_ip;
void wifi_init(void);
void connect_wifi(const char *ssid_wf,const char *password_wf);

#endif