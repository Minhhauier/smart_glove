idf_component_register(SRCS "mpu6050_v2.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                        esp_driver_i2c
                        driver
                        log 
                        esp_common
                        config_parameter
                        nvs_flash
                        
                        save_to_nvs)