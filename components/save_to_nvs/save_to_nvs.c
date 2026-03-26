#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>
#include <string.h>


#include "save_to_nvs.h"

void init_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}


void save_text_to_nvs(nvs_handle_t my_handle,const char* key, const char* text) {
    // Mở NVS với quyền READWRITE
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        // Ghi chuỗi vào key tương ứng
        err = nvs_set_str(my_handle, key, text);
        if (err == ESP_OK) {
            // Commit để xác nhận thay đổi
            nvs_commit(my_handle);
            printf("Đã lưu thành công: %s\n", text);
        }
        nvs_close(my_handle);
    }
}

char* read_text_from_nvs(nvs_handle_t my_handle, const char* key) {
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) return NULL;

    size_t required_size;
    // Bước 1: Gọi nvs_get_str với buffer NULL để lấy độ dài chuỗi (bao gồm '\0')
    err = nvs_get_str(my_handle, key, NULL, &required_size);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return NULL;
    }

    // Bước 2: Cấp phát bộ nhớ dựa trên độ dài đã lấy
    char* text = malloc(required_size);
    if (text) {
        err = nvs_get_str(my_handle, key, text, &required_size);
    }

    nvs_close(my_handle);
    return text; // Lưu ý: Cần free() sau khi sử dụng xong
}

//nvs_erase_all(my_handle_t)
//nvs_erase_key(my_handle_t, key)
//nvs_commit(my_handle_t) - sau khi xóa cần commit để xác nhận thay đổi


