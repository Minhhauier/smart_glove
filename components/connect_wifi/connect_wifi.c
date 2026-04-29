#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>

#include "connect_wifi.h"
#include "config_parameter.h"

static const char *TAG = "connect_wifi";

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int retry_count = 0;
static esp_event_handler_instance_t wifi_event_instance = NULL;
static esp_event_handler_instance_t ip_event_instance = NULL;

bool got_ip = false;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
        if (!WIFI_DIAGNOSTIC_NO_CONNECT) {
            ESP_LOGI(TAG, "Bat dau esp_wifi_connect()...");
            esp_wifi_connect();
        } else {
            ESP_LOGW(TAG, "Diagnostic mode: bo qua esp_wifi_connect() de test reboot tai pha start/init");
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_count < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            retry_count++;
            ESP_LOGI(TAG, "Thử lại WiFi %d/%d...", retry_count, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            got_ip = false;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi OK! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        retry_count = 0;
        got_ip = true;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(wifi_event_group ? ESP_OK : ESP_ERR_NO_MEM);


    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                         &wifi_event_handler, NULL, &wifi_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                         &wifi_event_handler, NULL, &ip_event_instance));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "WiFi init: set STA mode");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_LOGI(TAG, "WiFi init: apply STA config");
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    if (WIFI_DIAGNOSTIC_SKIP_START) {
        ESP_LOGW(TAG, "Diagnostic mode: bo qua esp_wifi_start() de xac nhan brownout chi xay ra khi bat RF");
        xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
    } else {
        ESP_LOGI(TAG, "WiFi init: start driver");
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_LOGI(TAG, "WiFi init: apply max tx power");
        ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(WIFI_MAX_TX_POWER_QUARTER_DBM));
        ESP_LOGI(TAG, "WiFi init: apply power save mode");
        ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_POWER_SAVE_MODE));
        ESP_LOGI(TAG, "WiFi power save=%d, max tx power=%d quarter-dBm",
                 WIFI_POWER_SAVE_MODE, WIFI_MAX_TX_POWER_QUARTER_DBM);
    }

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                            pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Kết nối WiFi thành công!");
    } else {
        ESP_LOGE(TAG, "Kết nối WiFi thất bại!");
        got_ip = false;
    }      
}
