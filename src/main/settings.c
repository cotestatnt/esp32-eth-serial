#include "settings.h"
#include "nvs_flash.h"
#include "esp_log.h"

static const char *TAG = "settings";
static const char *NVS_NAMESPACE = "bridge_cfg";

esp_err_t load_settings(settings_t *settings) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    int32_t uart_baud_rate = 0;
    err = nvs_get_i32(nvs_handle, "baud_rate", &uart_baud_rate);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Baud rate not found in NVS, using default");
        settings->uart_baud_rate = DEFAULT_UART_BAUD_RATE;
    } else if (err == ESP_OK) {
        settings->uart_baud_rate = uart_baud_rate;
    } else {
        ESP_LOGE(TAG, "Error getting baud rate from NVS: %s", esp_err_to_name(err));
    }

    int32_t tcp_port = 0;
    err = nvs_get_i32(nvs_handle, "tcp_port", &tcp_port);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "TCP port not found in NVS, using default");
        settings->tcp_port = DEFAULT_TCP_PORT;
    } else if (err == ESP_OK) {
        settings->tcp_port = tcp_port;
    } else {
        ESP_LOGE(TAG, "Error getting TCP port from NVS: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

esp_err_t save_settings(const settings_t *settings) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_i32(nvs_handle, "baud_rate", settings->uart_baud_rate);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error setting baud rate in NVS: %s", esp_err_to_name(err));
    }

    err = nvs_set_i32(nvs_handle, "tcp_port", settings->tcp_port);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error setting TCP port in NVS: %s", esp_err_to_name(err));
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS changes: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}
