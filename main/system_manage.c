#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <string.h>
#include <esp_log.h>

#include "config_parameter.h"
#include "at_command.h"
#include "save_to_nvs.h"   
#include "system_manage.h"

char data_recei[1024];
static const char *MQTT_TAG = "SYSTEM_MANAGE";

QueueHandle_t publish_queue_handle;
QueueHandle_t sim_at_queue_handle;
QueueHandle_t mqtt_queue_handle;

void init_queues() {
    sim_at_queue_handle = xQueueCreate(10, BUF_SIZE_SIM);
    mqtt_queue_handle = xQueueCreate(10, BUF_SIZE_SIM);
    publish_queue_handle = xQueueCreate(10, BUF_SIZE_SIM);
}

void sim_mqtt_task(void *pvParameters){
    while (1)
    {
        if(mqtt_sub_success){
            // if(xQueueReceive(sim_at_queue_handle,data_recei,pdMS_TO_TICKS(1000))==pdTRUE){
            //   //  ESP_LOGI(DATA_SIM_TAG,"%s",data_recei);
            //     if(strstr(data_recei,"+QMTSTAT: 1,1") || strstr(data_recei,"RDY")||strstr(data_recei,"+QMTSTAT: 1,2")){
            //         mqtt_sub_success=false;
            //     }
            //     else if(strstr(data_recei,"AT+QMTPUBEX") && strstr(data_recei,"ERROR")){
            //         mqtt_sub_success=false;
            //     }
            // }
            if(xQueueReceive(publish_queue_handle,data_recei,pdMS_TO_TICKS(1000))==pdTRUE){
                ESP_LOGI(MQTT_TAG,"%s",data_recei);
                if(strstr(data_recei,"data")!=NULL)
                {
                    mqtt_pub(PUB_TOPIC,data_recei); 
                }
            }
        }
        // else{
        //     mqtt_sim_init();
        // }
        vTaskDelay(pdMS_TO_TICKS(100));
    }   
}