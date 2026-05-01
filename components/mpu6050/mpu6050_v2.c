#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#include "mpu6050.h"
#include "config_parameter.h"
#include "ssd1306.h"

static const char *TAG = "TCA_MPU";


#define TCA9548A_ADDR   0x70   // A0=A1=A2=GND
#define MPU6050_ADDR    0x68   // AD0=GND

#define MPU_PWR_MGMT_1  0x6B
#define MPU_ACCEL_XOUT  0x3B
#define MPU_WHO_AM_I    0x75

i2c_master_dev_handle_t tca_handle;
i2c_master_dev_handle_t mpu_handle;
SSD1306_t dev;
static esp_err_t init_tca_mpu_handles(i2c_master_bus_handle_t bus_handle)
{
    if (bus_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (tca_handle == NULL) {
        i2c_device_config_t tca_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = TCA9548A_ADDR,
            .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        };
        esp_err_t ret = i2c_master_bus_add_device(bus_handle, &tca_cfg, &tca_handle);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    if (mpu_handle == NULL) {
        i2c_device_config_t mpu_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = MPU6050_ADDR,
            .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        };
        esp_err_t ret = i2c_master_bus_add_device(bus_handle, &mpu_cfg, &mpu_handle);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}

mpu6050_data_t data_mpu[3];  // Lưu dữ liệu từ 3 MPU6050
// static esp_err_t i2c_master_init(void)
// {
//     i2c_config_t conf = {
//         .mode             = I2C_MODE_MASTER,
//         .sda_io_num       = I2C_MASTER_SDA_IO,
//         .scl_io_num       = I2C_MASTER_SCL_IO,
//         .sda_pullup_en    = GPIO_PULLUP_ENABLE,
//         .scl_pullup_en    = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = I2C_MASTER_FREQ_HZ,
//     };
//     ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_PORT, &conf));

//     esp_err_t ret = i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0);
//     return ret;
// }

esp_err_t tca9548a_select_channel(i2c_master_dev_handle_t tca_handle, uint8_t channel) {
    if (channel > 7) return ESP_ERR_INVALID_ARG;
    uint8_t data = 1 << channel;
    // Truyền trực tiếp data vào thiết bị
    return i2c_master_transmit(tca_handle, &data, 1, -1);
}

esp_err_t mpu6050_write_reg(i2c_master_dev_handle_t mpu_handle, uint8_t reg_addr, uint8_t data) {
    uint8_t write_buf[2] = {reg_addr, data}; // Mảng chứa [Địa chỉ thanh ghi, Dữ liệu]
    return i2c_master_transmit(mpu_handle, write_buf, 2, -1);
}

esp_err_t mpu6050_read_reg(i2c_master_dev_handle_t mpu_handle, uint8_t reg_addr, uint8_t *data_buf, size_t len) {
    // Truyền reg_addr đi, sau đó tự động đọc 'len' byte lưu vào 'data_buf'
    return i2c_master_transmit_receive(mpu_handle, &reg_addr, 1, data_buf, len, -1);
}

static esp_err_t mpu6050_init(i2c_master_dev_handle_t tca_handle, i2c_master_dev_handle_t mpu_handle, uint8_t channel)
{
    // 1. Chuyển kênh TCA
    esp_err_t ret = tca9548a_select_channel(tca_handle, channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "CH%d: select TCA channel failed: %s", channel, esp_err_to_name(ret));
        return ret;
    }

    // 2. Kiểm tra WHO_AM_I
    uint8_t who_am_i = 0;
    // Thêm mpu_handle vào hàm đọc
    ret = mpu6050_read_reg(mpu_handle, MPU_WHO_AM_I, &who_am_i, 1);
    
    // Lưu ý: Một số lô hàng MPU6050 trả về 0x68, một số MPU6500 trả về 0x70 hoặc 0x71
    if (ret != ESP_OK || (who_am_i != 0x68 && who_am_i != 0x71)) {
        ESP_LOGE(TAG, "CH%d: MPU6050 not found! WHO_AM_I=0x%02X", channel, who_am_i);
        return ESP_FAIL;
    }

    // 3. Wake up MPU6050 (tắt sleep mode)
    // Thêm mpu_handle vào hàm ghi
    ret = mpu6050_write_reg(mpu_handle, MPU_PWR_MGMT_1, 0x00);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "CH%d: MPU6050 initialized OK", channel);
    }
    return ret;
}

static esp_err_t mpu6050_read_data(i2c_master_dev_handle_t tca_handle, i2c_master_dev_handle_t mpu_handle, uint8_t channel, mpu6050_data_t *out)
{
    // 1. Chuyển kênh tới ngón tay cần đọc
    esp_err_t ret = tca9548a_select_channel(tca_handle, channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "CH%d: select TCA channel failed while reading: %s", channel, esp_err_to_name(ret));
        return ret;
    }

    // 2. Đọc 14 byte dữ liệu thô
    uint8_t raw[14];
    // Thêm mpu_handle vào hàm đọc
    ret = mpu6050_read_reg(mpu_handle, MPU_ACCEL_XOUT, raw, 14);
    if (ret != ESP_OK) {
        return ret;
    }

    // 3. Xử lý dữ liệu
    int16_t ax_raw = (int16_t)((raw[0]  << 8) | raw[1]);
    int16_t ay_raw = (int16_t)((raw[2]  << 8) | raw[3]);
    int16_t az_raw = (int16_t)((raw[4]  << 8) | raw[5]);
    
    // (Bỏ comment phần Gyro nếu bro cần dùng để tính góc ngón tay)
    // int16_t gx_raw = (int16_t)((raw[8]  << 8) | raw[9]);
    // int16_t gy_raw = (int16_t)((raw[10] << 8) | raw[11]);
    // int16_t gz_raw = (int16_t)((raw[12] << 8) | raw[13]);

    out->x = ax_raw / 16384.0f;
    out->y = ay_raw / 16384.0f;
    out->z = az_raw / 16384.0f;

    return ESP_OK;
}

void TCA9548A_task(void *arg)
{
    ESP_LOGI(TAG, "Initializing I2C...");
    // ESP_ERROR_CHECK(i2c_master_init());
    i2c_master_init(&dev, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, -1);
    ssd1306_init(&dev, 128, 64);
    ssd1306_clear_screen(&dev, false);

    esp_err_t init_ret = init_tca_mpu_handles(dev._i2c_bus_handle);
    if (init_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init TCA/MPU I2C handles: %s", esp_err_to_name(init_ret));
        vTaskDelete(NULL);
        return;
    }

    ssd1306_display_text(&dev, 0, "Smart glove", strlen("Smart glove"), false);
    ssd1306_display_text(&dev, 2, "Haui", strlen("Haui"), false);
    ssd1306_display_text(&dev, 4, "Final project", strlen("Final project"), false);

    // Khởi tạo 3 MPU6050 trên kênh 0, 1, 2
    for (uint8_t ch = 0; ch < 3; ch++) {
        esp_err_t ret = mpu6050_init(tca_handle, mpu_handle, ch);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "MPU init failed on CH%d: %s", ch, esp_err_to_name(ret));
        }
    }
    
    while (1) {
        for (uint8_t ch = 0; ch < 3; ch++) {
            if (mpu6050_read_data(tca_handle, mpu_handle, ch, &data_mpu[ch]) == ESP_OK) {
                continue;
            } else {
                ESP_LOGE(TAG, "MPU#%d: Read failed!", ch);
            }
        }
        ESP_LOGI(TAG, "===============================");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}