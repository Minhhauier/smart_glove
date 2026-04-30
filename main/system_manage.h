#ifndef SYSTEM_MANAGE_H
#define SYSTEM_MANAGE_H

extern QueueHandle_t sim_at_queue_handle;
extern QueueHandle_t mqtt_queue_handle;
extern QueueHandle_t gps_queue_handle;
extern QueueHandle_t publish_queue_handle;
void init_queues();
void sim_mqtt_task(void *pvParameters);

#endif