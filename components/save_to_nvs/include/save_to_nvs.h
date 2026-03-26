#ifndef SAVE_TO_NVS_H
#define SAVE_TO_NVS_H

#include <nvs_flash.h>
#include <nvs.h>

void init_nvs();
void save_text_to_nvs(nvs_handle_t my_handle,const char* key, const char* text);
char* read_text_from_nvs(nvs_handle_t my_handle, const char* key);
void parse_json(const char* json_str);

#endif