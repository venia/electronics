# idf_component_register(SRCS "hello_world_main.c"
#                        PRIV_REQUIRES spi_flash
#                        INCLUDE_DIRS "")

idf_component_register(SRCS "check_wifi.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_wifi nvs_flash)
