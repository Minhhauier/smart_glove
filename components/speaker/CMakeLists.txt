idf_component_register(SRCS "speaker.c"
                    INCLUDE_DIRS "include"
                    REQUIRES 
                        esp_http_client
                        esp_driver_i2s
                        nvs_flash

                        minimp3
                        config_parameter
                        esp_wifi
                        cache_manager
                        mpu6050
                        save_to_nvs
                        at_command)