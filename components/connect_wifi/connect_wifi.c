// connect_wifi.c

#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"          // thêm mutex
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "connect_wifi.h"
#include "config_parameter.h"
#include "at_command.h"

static const char *TAG = "connect_wifi";

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

bool got_ip = false;

// ── State nội bộ ─────────────────────────────────────────────────────────────
static EventGroupHandle_t            s_wifi_event_group = NULL;
static esp_event_handler_instance_t  s_wifi_inst        = NULL;
static esp_event_handler_instance_t  s_ip_inst          = NULL;
static SemaphoreHandle_t             s_wifi_mutex        = NULL;
static bool                          s_stack_ready       = false;
static int                           s_retry             = 0;
static volatile bool                 s_intentional_disc  = false; // 🔑 flag chính

// ── Forward declarations ──────────────────────────────────────────────────────
void get_wifi_info_and_publish(void);
static void event_handler(void *arg, esp_event_base_t base,
                          int32_t id, void *data);

// ── Khởi tạo stack (1 lần) ───────────────────────────────────────────────────
static esp_err_t stack_init_once(void)
{
    if (s_wifi_event_group == NULL) {
        s_wifi_event_group = xEventGroupCreate();
        if (!s_wifi_event_group) return ESP_ERR_NO_MEM;
    }

    if (s_wifi_mutex == NULL) {
        s_wifi_mutex = xSemaphoreCreateMutex();
        if (!s_wifi_mutex) return ESP_ERR_NO_MEM;
    }

    if (s_stack_ready) return ESP_OK;

    esp_err_t err;
    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    if (!esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"))
        if (!esp_netif_create_default_wifi_sta()) return ESP_FAIL;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK && err != ESP_ERR_WIFI_INIT_STATE) return err;

    if (!s_wifi_inst)
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL, &s_wifi_inst));

    if (!s_ip_inst)
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, NULL, &s_ip_inst));

    s_stack_ready = true;
    return ESP_OK;
}

// ── Event handler ─────────────────────────────────────────────────────────────
static void event_handler(void *arg, esp_event_base_t base,
                          int32_t id, void *data)
{
    if (base == WIFI_EVENT) {
        switch (id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "STA_START → connect");
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                got_ip = false;
                if (s_intentional_disc) {
                    ESP_LOGI(TAG, "Disconnect chủ động, bỏ qua retry");
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                    break;
                }
                if (s_retry < WIFI_MAX_RETRY) {
                    s_retry++;
                    ESP_LOGI(TAG, "Retry %d/%d", s_retry, WIFI_MAX_RETRY);
                    esp_wifi_connect();
                } else {
                    ESP_LOGE(TAG, "Hết retry → FAIL");
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                }
                break;

            default: break;
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        s_retry           = 0;
        s_intentional_disc = false;
        got_ip            = true;
        get_wifi_info_and_publish();
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// ── Hàm nội bộ: áp config và start/connect ───────────────────────────────────
static void apply_config_and_start(const char *ssid, const char *pass)
{
    wifi_config_t cfg = {0};
    strncpy((char *)cfg.sta.ssid,     ssid, sizeof(cfg.sta.ssid)     - 1);
    strncpy((char *)cfg.sta.password, pass, sizeof(cfg.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));

    esp_err_t err = esp_wifi_start();
    if (err == ESP_ERR_WIFI_CONN) {
        // Đã started → chỉ reconnect
        ESP_LOGI(TAG, "Wi-Fi đã started → esp_wifi_connect()");
        ESP_ERROR_CHECK(esp_wifi_connect());
    } else {
        ESP_ERROR_CHECK(err);
        // STA_START event sẽ tự gọi connect
    }
}

// ── API công khai ─────────────────────────────────────────────────────────────

void wifi_init(void)
{
    ESP_ERROR_CHECK(stack_init_once());

    xSemaphoreTake(s_wifi_mutex, portMAX_DELAY);
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    s_intentional_disc = false;
    s_retry            = 0;
    apply_config_and_start(WIFI_SSID, WIFI_PASS);
    xSemaphoreGive(s_wifi_mutex);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT)
        ESP_LOGI(TAG, "wifi_init: OK");
    else
        ESP_LOGE(TAG, "wifi_init: FAIL");
}

void wifi_connect(const char *ssid, const char *password)
{
    if (!ssid || !password) { ESP_LOGE(TAG, "NULL arg"); return; }
    ESP_LOGI(TAG, "wifi_connect → '%s'", ssid);

    ESP_ERROR_CHECK(stack_init_once());

    xSemaphoreTake(s_wifi_mutex, portMAX_DELAY);

    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    s_intentional_disc = false;   // cho phép retry khi kết nối mới
    s_retry            = 0;
    got_ip             = false;

    // Ngắt kết nối cũ — đánh dấu intentional để event handler không retry
    s_intentional_disc = true;
    esp_wifi_disconnect();
    // Delay ngắn để event handler xử lý STA_DISCONNECTED xong
    vTaskDelay(pdMS_TO_TICKS(300));

    // Bây giờ mới cho phép retry trở lại cho lần connect mới
    s_intentional_disc = false;

    apply_config_and_start(ssid, password);

    xSemaphoreGive(s_wifi_mutex);

    ESP_LOGI(TAG, "Đã gửi lệnh connect, chờ kết quả qua event...");
}

void wifi_disconnect(void)
{
    ESP_LOGI(TAG, "wifi_disconnect: reset hoàn toàn");

    // Đảm bảo mutex đã được tạo trước khi dùng
    ESP_ERROR_CHECK(stack_init_once());

    xSemaphoreTake(s_wifi_mutex, portMAX_DELAY);

    s_intentional_disc = true;
    got_ip             = false;
    s_retry            = 0;
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    esp_err_t err = esp_wifi_disconnect();
    if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_CONNECT)
        ESP_LOGW(TAG, "esp_wifi_disconnect: %s", esp_err_to_name(err));

    err = esp_wifi_stop();
    if (err != ESP_OK)
        ESP_LOGW(TAG, "esp_wifi_stop: %s", esp_err_to_name(err));

    wifi_config_t empty_cfg = {0};
    esp_wifi_set_config(WIFI_IF_STA, &empty_cfg);

    xSemaphoreGive(s_wifi_mutex);

    ESP_LOGI(TAG, "Wi-Fi đã reset hoàn toàn.");
}

void wifi_driver_init(void)
{
    ESP_ERROR_CHECK(stack_init_once());
}

// ── Publish info sau khi có IP ────────────────────────────────────────────────
void get_wifi_info_and_publish(void)
{
    wifi_ap_record_t    ap  = {0};
    esp_netif_ip_info_t ip  = {0};
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) {
        ESP_LOGE(TAG, "Không lấy được AP info"); return;
    }
    if (netif) {
        esp_netif_get_ip_info(netif, &ip);
        char ip_str[16];
        esp_ip4addr_ntoa(&ip.ip, ip_str, sizeof(ip_str));
        ESP_LOGI(TAG, "SSID: %s  IP: %s", ap.ssid, ip_str);
        publish_response_connect_wifi((char *)ap.ssid, ip_str, got_ip ? 1 : 0);
    }
}