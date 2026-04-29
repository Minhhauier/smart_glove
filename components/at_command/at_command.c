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

static char cmd[128];
static char client[64];
static char buffer[2048];

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
void mqtt_connect(){
    send_at_get_respond("AT+QMTCFG=\"recv/mode\",0,0,1", 1000);        
    send_at_get_respond("AT+QMTCFG=\"SSL\",0,1,2", 1000);
    snprintf(cmd,sizeof(cmd),"AT+QFUPL=\"UFS:cacert.pem\",%d,100", strlen(cert_pem_v1));
    send_at_get_respond(cmd, 1000);
    send_at_get_respond(cert_pem_v1, 1000);
    send_at_get_respond("AT+QMTCFG=\"keepalive\",1,60",1000);
    snprintf(cmd,sizeof(cmd),"AT+QMTOPEN=1,%s",MQTT_BROKER_URL);
    send_at_get_respond(cmd,1000);
    snprintf(client, sizeof(client), "%s_%s","SmartGlove" , "haui");
    snprintf(cmd,sizeof(cmd),"AT+QMTCONN=1,\"%s\",[\"%s\",\"%s\"]",client,USERNAME,PASSWORD);
    send_at_get_respond(cmd,1000);
}

void request_call(const char *phone_number) {
    char cmd[64];
    send_at_get_respond("AT+CMEE=2", 1000);          // Bật verbose error
    send_at_get_respond("AT+CPIN?", 1000);           // Kiểm tra SIM PIN
    send_at_get_respond("AT+CEREG?", 3000);          // Kiểm tra đăng ký LTE (quan trọng với EG800K)
    send_at_get_respond("AT+QCFG=\"ims\"", 1000);   // Kiểm tra IMS/VoLTE có bật không
    send_at_get_respond("AT+QCFG=\"ims\",1", 2000); // Bật IMS/VoLTE (bắt buộc với EG800K)
    send_at_get_respond("AT+QIMSREG?", 3000);        // Kiểm tra trạng thái đăng ký IMS
    vTaskDelay(pdMS_TO_TICKS(3000));                 // Chờ IMS đăng ký xong
    snprintf(cmd, sizeof(cmd), "ATD%s;", phone_number);
    send_at_get_respond(cmd, 5000);
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