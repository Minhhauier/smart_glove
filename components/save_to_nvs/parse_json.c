#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <stdio.h>
#include <cJSON.h> 
#include <esp_log.h>

#include "save_to_nvs.h"

nvs_handle_t my_nvs_handle_json;

void parse_json(const char* json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        ESP_LOGE("JSON", "Error parsing JSON");
        return;
    }
    const cJSON *command_type = cJSON_GetObjectItemCaseSensitive(root, "command_type");
    const cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
    if(cJSON_IsNumber(command_type)){
        int cmd_type = command_type->valueint;
        if(cmd_type == 101){
            const cJSON *activity = cJSON_GetObjectItemCaseSensitive(data, "activity");
            const cJSON *want = cJSON_GetObjectItemCaseSensitive(data, "want");
            if(cJSON_IsNumber(activity) && cJSON_IsString(want) && (want->valuestring != NULL)){
                int act = activity->valueint;
                char *want_str = want->valuestring;
                switch (act)
                {
                case 1:
                    save_text_to_nvs(my_nvs_handle_json, "activity_1", want_str);
                    break;
                case 2:
                    save_text_to_nvs(my_nvs_handle_json, "activity_2", want_str);
                    break;
                case 3:
                    save_text_to_nvs(my_nvs_handle_json, "activity_3", want_str);
                    break; 
                case 4:
                    save_text_to_nvs(my_nvs_handle_json, "activity_4", want_str);
                    break;
                default:
                    break;
                }
            }
        } 
    }

}