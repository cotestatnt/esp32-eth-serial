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

    // Load static IP flag
    int32_t use_static_ip = 0;
    err = nvs_get_i32(nvs_handle, "use_static_ip", &use_static_ip);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        settings->use_static_ip = 0;
    } else if (err == ESP_OK) {
        settings->use_static_ip = use_static_ip;
    } else {
        ESP_LOGE(TAG, "Error getting use_static_ip from NVS: %s", esp_err_to_name(err));
        settings->use_static_ip = 0;
    }

    // Helper to load string keys safely
    size_t required_size = 0;

    // IP address
    err = nvs_get_str(nvs_handle, "ip_addr", NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND || required_size == 0) {
        settings->ip_addr[0] = '\0';
    } else if (err == ESP_OK && required_size < sizeof(settings->ip_addr)) {
        nvs_get_str(nvs_handle, "ip_addr", settings->ip_addr, &required_size);
    } else {
        ESP_LOGW(TAG, "Invalid ip_addr in NVS, clearing");
        settings->ip_addr[0] = '\0';
    }

    // Netmask
    required_size = 0;
    err = nvs_get_str(nvs_handle, "netmask", NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND || required_size == 0) {
        settings->netmask[0] = '\0';
    } else if (err == ESP_OK && required_size < sizeof(settings->netmask)) {
        nvs_get_str(nvs_handle, "netmask", settings->netmask, &required_size);
    } else {
        ESP_LOGW(TAG, "Invalid netmask in NVS, clearing");
        settings->netmask[0] = '\0';
    }

    // Gateway
    required_size = 0;
    err = nvs_get_str(nvs_handle, "gateway", NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND || required_size == 0) {
        settings->gateway[0] = '\0';
    } else if (err == ESP_OK && required_size < sizeof(settings->gateway)) {
        nvs_get_str(nvs_handle, "gateway", settings->gateway, &required_size);
    } else {
        ESP_LOGW(TAG, "Invalid gateway in NVS, clearing");
        settings->gateway[0] = '\0';
    }

    // DNS1
    required_size = 0;
    err = nvs_get_str(nvs_handle, "dns1", NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND || required_size == 0) {
        settings->dns1[0] = '\0';
    } else if (err == ESP_OK && required_size < sizeof(settings->dns1)) {
        nvs_get_str(nvs_handle, "dns1", settings->dns1, &required_size);
    } else {
        ESP_LOGW(TAG, "Invalid dns1 in NVS, clearing");
        settings->dns1[0] = '\0';
    }

    // DNS2
    required_size = 0;
    err = nvs_get_str(nvs_handle, "dns2", NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND || required_size == 0) {
        settings->dns2[0] = '\0';
    } else if (err == ESP_OK && required_size < sizeof(settings->dns2)) {
        nvs_get_str(nvs_handle, "dns2", settings->dns2, &required_size);
    } else {
        ESP_LOGW(TAG, "Invalid dns2 in NVS, clearing");
        settings->dns2[0] = '\0';
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

    // Save static IP config
    err = nvs_set_i32(nvs_handle, "use_static_ip", settings->use_static_ip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error setting use_static_ip in NVS: %s", esp_err_to_name(err));
    }

    if (settings->ip_addr[0] != '\0') {
        err = nvs_set_str(nvs_handle, "ip_addr", settings->ip_addr);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error setting ip_addr in NVS: %s", esp_err_to_name(err));
        }
    }
    if (settings->netmask[0] != '\0') {
        err = nvs_set_str(nvs_handle, "netmask", settings->netmask);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error setting netmask in NVS: %s", esp_err_to_name(err));
        }
    }
    if (settings->gateway[0] != '\0') {
        err = nvs_set_str(nvs_handle, "gateway", settings->gateway);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error setting gateway in NVS: %s", esp_err_to_name(err));
        }
    }
    if (settings->dns1[0] != '\0') {
        err = nvs_set_str(nvs_handle, "dns1", settings->dns1);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error setting dns1 in NVS: %s", esp_err_to_name(err));
        }
    }
    if (settings->dns2[0] != '\0') {
        err = nvs_set_str(nvs_handle, "dns2", settings->dns2);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error setting dns2 in NVS: %s", esp_err_to_name(err));
        }
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS changes: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}
