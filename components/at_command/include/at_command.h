#ifndef AT_COMMAND
#define AT_COMMAND

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern QueueHandle_t sim_at_queue_handle;
extern QueueHandle_t mqtt_queue_handle;
extern QueueHandle_t gps_queue_handle;
extern QueueHandle_t publish_queue_handle;
extern int queue_created;
/**
 * @brief use to send and read respond at command at the same time
 */
void send_at_get_respond(char *cmd,int timeout);
/**
 * @brief use to send at comamd
 */
void send_at(char *cmd);
/**
 * @brief use to get respond from module SIM
 */
char *get_respond(int timeout);
/**
 * @brief use to send all uart_sim respond to queue
 */
void read_and_send_to_queue_task(void *pvParameters);
/**
 * 
 */
void uart_sim_init();
void copy_respond_to_array(char *cmd,char *array);
void init_queues();
void request_call(const char *phone_number);
void request_message(const char *phone_number, const char *message);
#endif