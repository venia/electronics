#include <stdio.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_a2dp_api.h"
#include "esp_log.h"
#include "driver/i2s.h"

// Конфигурация Wi-Fi
#define WIFI_SSID "aquila"
#define WIFI_PASSWORD "1qwerty!"

// Конфигурация HTTP-потока
#define MP3_STREAM_URL "http://194.44.230.64:8000/liveradio.mp3"

// Конфигурация Bluetooth A2DP
static const char *TAG = "HV-H2575BT";

// Буфер для обработки аудио
#define AUDIO_BUFFER_SIZE 1024
int16_t audio_buffer[AUDIO_BUFFER_SIZE];

// Инициализация Wi-Fi
void wifi_init_sta() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

// Обработчик HTTP-потока
esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            // Здесь можно обрабатывать полученные данные (MP3-поток)
            ESP_LOGI(TAG, "Received data: %d bytes", evt->data_len);
            // Пример: копирование данных в буфер
            memcpy(audio_buffer, evt->data, evt->data_len);
            break;
        default:
            break;
    }
    return ESP_OK;
}

void http_stream_mp3() {
    esp_http_client_config_t config = {
        .url = MP3_STREAM_URL,
        .event_handler = http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

// Усиление басов (простой фильтр низких частот)
float bass_boost_filter(float input, float *buffer, float alpha) {
    float output = alpha * input + (1 - alpha) * *buffer;
    *buffer = output;
    return output;
}

// Обработка аудиоданных
void process_audio(int16_t *data, size_t len) {
    static float buffer = 0;
    float alpha = 0.5; // Коэффициент фильтра

    for (int i = 0; i < len; i++) {
        data[i] = (int16_t)bass_boost_filter((float)data[i], &buffer, alpha);
    }
}

// Инициализация Bluetooth A2DP
void bt_a2dp_sink_init() {
    esp_a2d_sink_init();
    esp_a2d_sink_register_data_callback(bt_a2dp_sink_data_cb);
    esp_a2d_sink_register_connection_state_callback(bt_a2dp_sink_connection_state_cb);
}

// Callback для обработки аудиоданных A2DP
void bt_a2dp_sink_data_cb(const uint8_t *data, uint32_t len) {
    // Пример обработки данных перед отправкой
    process_audio((int16_t *)data, len / sizeof(int16_t));
    // Отправка данных на Bluetooth-наушники
    // (здесь можно добавить дополнительную обработку)
}

// Callback для отслеживания состояния соединения A2DP
void bt_a2dp_sink_connection_state_cb(esp_a2d_connection_state_t state, void *param) {
    ESP_LOGI(TAG, "A2DP connection state: %d", state);
}

// Основная функция
void app_main() {
    // Инициализация NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Инициализация Wi-Fi
    wifi_init_sta();

    // Инициализация Bluetooth A2DP
    bt_a2dp_sink_init();

    // Получение MP3-потока
    http_stream_mp3();
}