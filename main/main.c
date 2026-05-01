#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <minimp3.h>

#include "connect_wifi.h"
#include "speaker.h"
// #include "cache_manager.h"
#include "cache_manager.h"
#include "config_parameter.h"
#include "mpu6050.h"
#include "mqtt_esp32.h"
#include "at_command.h"
#include "system_manage.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Khởi động ứng dụng...");
    esp_reset_reason_t reason = esp_reset_reason();
    ESP_LOGW(TAG, "Reset reason: %d", (int)reason); // check reset reason at startup

    ESP_LOGI(TAG, "=== ESP32 Tiếng Việt TTS (ESP-IDF v5.x) ===");
    ESP_LOGI(TAG, "Heap khả dụng: %d bytes", (int)esp_get_free_heap_size());
    ESP_LOGI(TAG, "Volume: %d%%", AUDIO_VOLUME_PERCENT);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    tts_cache_init();   // khởi tạo partition 'storage' cho cache MP3
    printf("wifi đã được khởi tạo.\n");
    i2s_init();
    vTaskDelay(pdMS_TO_TICKS(3000)); 
    uart_sim_init();    
    mqtt_sim_init();
    wifi_driver_init();
    while (mqtt_sub_success==false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
    
    xTaskCreate(TCA9548A_task, "TCA9548A_task", 1024*4, NULL, 5, NULL);
    xTaskCreate(speaker_task, "speaker_task", 1024*24, NULL, 5, NULL);
    xTaskCreate(read_and_send_to_queue_task, "read_and_send_to_queue_task", 1024*8, NULL, 5, NULL);
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}