idf_component_register(SRCS "save_to_nvs.c" "parse_json.c"
                    INCLUDE_DIRS "include"
                    REQUIRES 
                        nvs_flash
                        json)