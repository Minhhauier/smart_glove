idf_component_register(
    SRCS "mqtt_esp32.c"
    INCLUDE_DIRS include
    REQUIRES
        mqtt
        esp_wifi

        save_to_nvs
)