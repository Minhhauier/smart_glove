#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <stdio.h>
#include <cJSON.h> 
#include <esp_log.h>

#include "save_to_nvs.h"
#include "speaker.h"
#include "connect_wifi.h"

nvs_handle_t my_nvs_handle_json;
char phone_number[64]= "0374337713";


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
                    save_change_tts_data("activity_1", want_str);
                    save_text_to_nvs(my_nvs_handle_json,"activity__text1", want_str);
                    break;
                case 2:
                    save_change_tts_data("activity_2", want_str);
                    save_text_to_nvs(my_nvs_handle_json,"activity__text2", want_str);
                    break;
                case 3:
                    save_change_tts_data("activity_3", want_str);
                    save_text_to_nvs(my_nvs_handle_json,"activity__text3", want_str);
                    break; 
                case 4:
                    save_change_tts_data("activity_4", want_str);
                    save_text_to_nvs(my_nvs_handle_json,"activity__text4", want_str);
                    break;
                case 5:
                    save_change_tts_data("activity_5", want_str);
                    save_text_to_nvs(my_nvs_handle_json,"activity__text5", want_str);
                    break;
                case 6:
                    save_change_tts_data("activity_6", want_str);
                    save_text_to_nvs(my_nvs_handle_json,"activity__text6", want_str);
                    break;
                case 7:
                    save_change_tts_data("activity_7", want_str);
                    save_text_to_nvs(my_nvs_handle_json,"activity__text7", want_str);
                    break;
                case 8:
                    save_change_tts_data("activity_8", want_str);
                    save_text_to_nvs(my_nvs_handle_json,"activity__text8", want_str);
                    break;
                case 9:
                    save_change_tts_data("activity_9", want_str);
                    save_text_to_nvs(my_nvs_handle_json,"activity__text9", want_str);
                    break;
                case 10:
                    save_change_tts_data("activity_10", want_str);
                    save_text_to_nvs(my_nvs_handle_json,"activity__text10", want_str);
                    break;
                default:
                    break;
                }
            }
        } 
        else if(cmd_type == 103){
            const cJSON *phone_number_json = cJSON_GetObjectItemCaseSensitive(data, "phone_number");
            if(cJSON_IsString(phone_number_json) && (phone_number_json->valuestring != NULL)){
                char *phone_number_str = phone_number_json->valuestring;
                strncpy(phone_number, phone_number_str, sizeof(phone_number) - 1);
                phone_number[sizeof(phone_number) - 1] = '\0';
                ESP_LOGI("JSON", "Updated phone number: %s", phone_number);
            }
        }
        else if(cmd_type == 104){
            const cJSON *wifi_ssid = cJSON_GetObjectItemCaseSensitive(data, "ssid");
            const cJSON *wifi_password = cJSON_GetObjectItemCaseSensitive(data, "password");
            if(cJSON_IsString(wifi_ssid) && (wifi_ssid->valuestring != NULL) && cJSON_IsString(wifi_password) && (wifi_password->valuestring != NULL)){
                char *ssid_str = wifi_ssid->valuestring;
                char *password_str = wifi_password->valuestring;
                if(ssid_str==NULL || password_str==NULL) {
                    ESP_LOGE("JSON", "SSID or password is NULL");
                    cJSON_Delete(root);
                    return;
                }
                wifi_connect(ssid_str, password_str);

            }
        }
        else if(cmd_type == 105){
            wifi_disconnect();
        }
    }

    cJSON_Delete(root);

}

