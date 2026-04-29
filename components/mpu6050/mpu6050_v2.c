#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

#include "mpu6050.h"
#include "config_parameter.h"

static const char *TAG = "TCA_MPU";


#define TCA9548A_ADDR   0x70   // A0=A1=A2=GND
#define MPU6050_ADDR    0x68   // AD0=GND

#define MPU_PWR_MGMT_1  0x6B
#define MPU_ACCEL_XOUT  0x3B
#define MPU_WHO_AM_I    0x75

mpu6050_data_t data[3];  // Lưu dữ liệu từ 3 MPU6050
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = I2C_MASTER_SDA_IO,
        .scl_io_num       = I2C_MASTER_SCL_IO,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_PORT, &conf));
    return i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0);
}

static esp_err_t tca9548a_select_channel(uint8_t channel)
{
    uint8_t mask = (channel > 7) ? 0x00 : (1 << channel);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (TCA9548A_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, mask, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, I2C_MASTER_FREQ_HZ);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TCA9548A select channel %d failed: %s", channel, esp_err_to_name(ret));
    }
    return ret;
}

static esp_err_t mpu6050_write_reg(uint8_t reg, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, I2C_MASTER_FREQ_HZ);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t mpu6050_read_reg(uint8_t reg, uint8_t *buf, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);  // repeated start
    i2c_master_write_byte(cmd, (MPU6050_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buf, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, I2C_MASTER_FREQ_HZ);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t mpu6050_init(uint8_t channel)
{
    ESP_ERROR_CHECK(tca9548a_select_channel(channel));

    // Kiểm tra WHO_AM_I
    uint8_t who_am_i = 0;
    esp_err_t ret = mpu6050_read_reg(MPU_WHO_AM_I, &who_am_i, 1);
    if (ret != ESP_OK || who_am_i != 0x68) {
        ESP_LOGE(TAG, "CH%d: MPU6050 not found! WHO_AM_I=0x%02X", channel, who_am_i);
        return ESP_FAIL;
    }

    // Wake up MPU6050 (tắt sleep mode)
    ret = mpu6050_write_reg(MPU_PWR_MGMT_1, 0x00);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "CH%d: MPU6050 initialized OK", channel);
    }
    return ret;
}


static esp_err_t mpu6050_read_data(uint8_t channel, mpu6050_data_t *out)
{
    ESP_ERROR_CHECK(tca9548a_select_channel(channel));

    uint8_t raw[14];
    esp_err_t ret = mpu6050_read_reg(MPU_ACCEL_XOUT, raw, 14);
    if (ret != ESP_OK) return ret;

    int16_t ax_raw = (int16_t)((raw[0]  << 8) | raw[1]);
    int16_t ay_raw = (int16_t)((raw[2]  << 8) | raw[3]);
    int16_t az_raw = (int16_t)((raw[4]  << 8) | raw[5]);
    // int16_t gx_raw = (int16_t)((raw[8]  << 8) | raw[9]);
    // int16_t gy_raw = (int16_t)((raw[10] << 8) | raw[11]);
    // int16_t gz_raw = (int16_t)((raw[12] << 8) | raw[13]);

    // Chia theo độ nhạy mặc định: Accel ±2g, Gyro ±250°/s
    out->x = ax_raw / 16384.0f;
    out->y = ay_raw / 16384.0f;
    out->z = az_raw / 16384.0f;
    // out->gx = gx_raw / 131.0f;
    // out->gy = gy_raw / 131.0f;
    // out->gz = gz_raw / 131.0f;

    return ESP_OK;
}

void TCA9548A_task(void *arg)
{
    ESP_LOGI(TAG, "Initializing I2C...");
    ESP_ERROR_CHECK(i2c_master_init());

    // Khởi tạo 3 MPU6050 trên kênh 0, 1, 2
    for (uint8_t ch = 0; ch < 3; ch++) {
        mpu6050_init(ch);
    }
    
    while (1) {
        for (uint8_t ch = 0; ch < 3; ch++) {
            if (mpu6050_read_data(ch, &data[ch]) == ESP_OK) {
                ESP_LOGI(TAG,
                    "MPU#%d | Accel: X=%.2fg Y=%.2fg Z=%.2fg",
                    ch,
                    data[ch].x, data[ch].y, data[ch].z
                );
            } else {
                ESP_LOGE(TAG, "MPU#%d: Read failed!", ch);
            }
        }
        ESP_LOGI(TAG, "===============================");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}