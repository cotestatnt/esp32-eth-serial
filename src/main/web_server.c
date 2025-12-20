#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "settings.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "web_server";
static httpd_handle_t server = NULL;

extern const char* settings_html_start;

static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");

    settings_t current_settings;
    load_settings(&current_settings);

    char html_buffer[1024];
    snprintf(html_buffer, sizeof(html_buffer),
        "<!DOCTYPE html><html><head><title>ESP32 Bridge Config</title></head>"
        "<body><h1>ESP32 Bridge Configuration</h1>"
        "<form action=\"/save\" method=\"post\">"
        "<p>UART Baud Rate: <input type=\"text\" name=\"baud_rate\" value=\"%d\"></p>"
        "<p>TCP Port: <input type=\"text\" name=\"tcp_port\" value=\"%d\"></p>"
        "<p><input type=\"submit\" value=\"Save and Reboot\"></p>"
        "</form></body></html>",
        current_settings.uart_baud_rate, current_settings.tcp_port);

    return httpd_resp_send(req, html_buffer, strlen(html_buffer));
}

static esp_err_t save_post_handler(httpd_req_t *req) {
    char buf[128];
    int ret, remaining = req->content_len;

    if (remaining > sizeof(buf) -1) {
        remaining = sizeof(buf) -1;
    }
    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    settings_t new_settings;
    char baud_rate_str[16];
    char tcp_port_str[16];

    if (httpd_query_key_value(buf, "baud_rate", baud_rate_str, sizeof(baud_rate_str)) == ESP_OK &&
        httpd_query_key_value(buf, "tcp_port", tcp_port_str, sizeof(tcp_port_str)) == ESP_OK) {

        new_settings.uart_baud_rate = atoi(baud_rate_str);
        new_settings.tcp_port = atoi(tcp_port_str);

        if (new_settings.uart_baud_rate > 0 && new_settings.tcp_port > 0) {
            save_settings(&new_settings);
            httpd_resp_send(req, "Settings saved. Rebooting...", HTTPD_RESP_USE_STRLEN);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            esp_restart();
        } else {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid settings");
        }
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to parse settings");
    }

    return ESP_OK;
}


static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler
};

static const httpd_uri_t save = {
    .uri       = "/save",
    .method    = HTTP_POST,
    .handler   = save_post_handler
};


void start_webserver(void) {
    if (server) {
        return;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    
    ESP_LOGI(TAG, "Starting server on port: \'%d\'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &save);
    }
}

void stop_webserver(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
}
