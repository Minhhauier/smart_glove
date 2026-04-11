#include <stdio.h>
#include <stdlib.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <string.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


#include "config_parameter.h"
#include "at_command.h"
// #include "control_relay.h"

#define BUF_SIZE_SIM 2048
#define BaUD_RATE 115200
#define TX_SIM GPIO_NUM_17
#define RX_SIM GPIO_NUM_39


void send_at_get_respond(char *cmd, int timeout)
{
    char data[BUF_SIZE_SIM];
    ESP_LOGI("SIM", "sent: %s", cmd);
    uart_write_bytes(UART_NUM_1, cmd, strlen(cmd));
    uart_write_bytes(UART_NUM_1, "\r\n", 2);
    int total_len = 0;
    int count = 0;
    for (int i = 0; i < timeout; i += 100)
    {
        int len = uart_read_bytes(UART_NUM_1, data + total_len, BUF_SIZE_SIM - total_len - 1, 100 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            total_len = total_len + len;
            count = 0;
        }
        else
            count++;
        if (count > 3)
            break;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if (total_len > 0)
    {
        data[total_len] = '\0';
       printf("RX_sent_at: %s\r\n", data);
    }
}
void uart_sim_init() {
    const uart_config_t uart_config = {
        .baud_rate = BaUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TX_SIM, RX_SIM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, BUF_SIZE_SIM * 2, 0, 0, NULL, 0);
}
void send_at(char *cmd)
{
    ESP_LOGI("SIM","sent: %s",cmd);
    uart_write_bytes(UART_NUM_1, cmd, strlen(cmd));
    uart_write_bytes(UART_NUM_1, "\r\n", 2);
}
char *get_respond(int timeout)
{
    int total_len = 0;
    int count = 0;
    char data[BUF_SIZE_SIM];
    for (int i = 0; i < timeout; i += 100)
    {
        int len = uart_read_bytes(UART_NUM_1, data + total_len, BUF_SIZE_SIM - total_len - 1, 100 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            total_len = total_len + len;
            count = 0;
        }
        else
            count++;
        if (count > 3)
            break;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if (total_len > 0)
    {
        data[total_len] = '\0';
        printf("RX: %s\r\n", data);
        char *data_new = calloc(total_len + 1, sizeof(char));
        strcpy(data_new, data);
        return data_new;
    }
    return NULL;
}
// void copy_respond_to_array(char *cmd,char *array){
//     send_at(cmd);
//     char *data = get_respond(1000);
//     if(strcmp(cmd,"AT+CCLK?")==0){
//         char *start = strchr(data,'"');
//         char *end = strrchr(data,'"');
//         if(start!=NULL && end!=NULL && end>start){
//             int len = end - start - 1;
//             if(len<64){
//                 memcpy(array,start+1,len);
//                 array[len]='\0';
//             }
//         }
//         // printf("Date time: %s\r\n",array);
//     }
//     // strcpy(array,data);
//     free(data);
// }

void request_call(const char *phone_number) {
    char cmd[64];
    send_at_get_respond("ATD?", 1000); // Kiểm tra xem module có sẵn sàng nhận lệnh gọi không
    send_at_get_respond("AT+CREG?", 1000); // Kiểm tra đăng ký mạng
    send_at_get_respond("AT+QCFG=\"volte\"",1000); // Kiểm tra cấu hình VoLTE
    snprintf(cmd, sizeof(cmd), "ATD%s;", phone_number);
    send_at_get_respond("AT+CMEE=2",5000);
    send_at_get_respond(cmd,5000);
}

void request_message(const char *phone_number, const char *message) {
    char cmd[64];
    send_at_get_respond("AT+CMGF=1", 1000); // Chuyển sang chế độ text
    send_at_get_respond("AT+CSCS=\"GSM\"", 1000); // Chọn bộ ký tự GSM
    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", phone_number);
    send_at_get_respond(cmd, 1000); // Gửi lệnh gửi tin nhắn
    uart_write_bytes(UART_NUM_1, message, strlen(message)); // Gửi nội dung tin nhắn
    uart_write_bytes(UART_NUM_1, "\x1A", 1); // Gửi ký tự Ctrl+Z để kết thúc tin nhắn
}