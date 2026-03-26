#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <mqtt_client.h>
#include <string.h>

#include "mqtt_esp32.h"
#include "save_to_nvs.h"

static const char *TAG = "mqtt";
static char mqtt_client_id[64] = "";
static char *mqtt_broker_uri = "mqtts://b89cc7c8dfe0427c9dbbe952fc9c426a.s1.eu.hivemq.cloud:8883";
static char *mqtt_username = "minhhaui";
static char *mqtt_password = "Minh123456";
static char json_payload[1024];
static char buffer[1024] = "";
static char *sub_topic = "SmartGlove/sub";
static char *pub_topic = "SmartGlove/pub";


esp_mqtt_client_handle_t client;
static esp_err_t mqtt_subscribe(esp_mqtt_client_handle_t client_id, const char *topic, int qos)
{
    int msg_id = esp_mqtt_client_subscribe_single(client_id, topic, qos);
    ESP_LOGI(TAG, "Subscribed to topic %s with message ID: %d", topic, msg_id);
    if (msg_id == -1)
        return ESP_FAIL;
    else
        return ESP_OK;
}

static esp_err_t mqtt_publish(esp_mqtt_client_handle_t client_id, const char *topic, const char *data, int len, int qos, int retain)
{
    int msg_id = esp_mqtt_client_publish(client_id, topic, data, len, qos, retain);
    ESP_LOGI(TAG, "Published message with ID: %d", msg_id);
    if (msg_id == -1)
        return ESP_OK;
    else
        return ESP_FAIL;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // snprintf(cmd, sizeof(cmd), "%s/Test", device_name);
        mqtt_subscribe(client, sub_topic, 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
         printf("DATA=%.*s\r\n", event->data_len, event->data);
        memcpy(buffer, event->data, event->data_len);
        buffer[event->data_len] = '\0';
        
        //convert_to_json_update(buffer);
        parse_json(buffer);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_init(char *mqtt_address, char *client_id, char *username, char *password)
{
    ESP_LOGI(TAG, "MQTT configuration function called");
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = mqtt_address,
        .credentials.client_id = client_id,
        .credentials.username = username,
        .credentials.authentication.password = password,
        .network.reconnect_timeout_ms = 5000,
        .session.keepalive = 120,
        .broker.verification.certificate = cert_pem,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void mqtt_start()
{
    snprintf(mqtt_client_id, sizeof(mqtt_client_id), "%s_%s", "smartglove", "hauifinalproject");
    // mqtt_init(mqtt_broker_uri, mqtt_client_id, mqtt_username, mqtt_password);
    mqtt_init(mqtt_broker_uri, mqtt_client_id, mqtt_username, mqtt_password);
}
