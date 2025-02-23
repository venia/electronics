# set(ADF_PATH "D:\\PROGRAMM\\esp-adf")
# include($ENV{ADF_PATH}/idf_patches.cmake)

# idf_component_register(SRCS "main.c"
#     INCLUDE_DIRS "."
#     REQUIRES esp_wifi nvs_flash esp_http_client, esp_a2dp_api)


idf_component_register(SRCS "check_wifi.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi nvs_flash esp_http_client, esp_a2dp_api)



