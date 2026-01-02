#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "settings.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "web_server";
static httpd_handle_t server = NULL;

extern const char* settings_html_start;

// Attractive yet lightweight page skeleton using string literals
static const char *PAGE_HEAD =
    "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<title>ESP32 Bridge Config</title>"
    "<style>"
    "body{font-family:system-ui,-apple-system,Segoe UI,Roboto;background:#f7f7f8;color:#222;margin:0}"
    ".container{max-width:720px;margin:32px auto;padding:24px;background:#fff;border-radius:12px;box-shadow:0 4px 16px rgba(0,0,0,.08)}"
    "h1{margin:0 0 12px;font-size:20px}"
    "fieldset{border:1px solid #e6e6e9;border-radius:8px;margin:16px 0;padding:16px}"
    "legend{padding:0 8px;color:#333;font-weight:600}"
    "label{display:block;margin:10px 0 6px;color:#555;font-size:14px}"
    "input[type=text],input[type=number]{width:100%;padding:10px;border:1px solid #ccc;border-radius:6px;font-size:14px;box-sizing:border-box}"
    ".row{display:grid;grid-template-columns:1fr 1fr;gap:12px}"
    ".actions{margin-top:16px}"
    "button{background:#0078d4;color:#fff;border:0;border-radius:8px;padding:10px 14px;font-weight:600;cursor:pointer}"
    "button:active{transform:translateY(1px)}"
    "</style>"
    "<script>function tg(cb){document.querySelectorAll('.static-field').forEach(function(el){el.disabled=!cb.checked;});}</script>"
    "</head><body><div class=\"container\"><h1>ESP32 Bridge Configuration</h1>";

static const char *PAGE_TAIL = "</form></div></body></html>";

static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");

    settings_t current_settings;
    load_settings(&current_settings);

    // Stream response to reduce stack usage
    httpd_resp_sendstr_chunk(req, PAGE_HEAD);
    httpd_resp_sendstr_chunk(req, "<form action=\"/save\" method=\"post\">\n");

    char tmp[192];
    httpd_resp_sendstr_chunk(req, "<fieldset><legend>Serial & TCP</legend>\n");
    snprintf(tmp, sizeof(tmp), "<label>UART Baud Rate</label><input type=\"number\" name=\"baud_rate\" value=\"%d\" min=\"1200\" step=\"1\">\n",
             current_settings.uart_baud_rate);
    httpd_resp_sendstr_chunk(req, tmp);
    snprintf(tmp, sizeof(tmp), "<label>TCP Port</label><input type=\"number\" name=\"tcp_port\" value=\"%d\" min=\"1\" max=\"65535\">\n",
             current_settings.tcp_port);
    httpd_resp_sendstr_chunk(req, tmp);
    httpd_resp_sendstr_chunk(req, "</fieldset>\n");

    httpd_resp_sendstr_chunk(req, "<fieldset><legend>Network (Ethernet)</legend>\n");
    snprintf(tmp, sizeof(tmp), "<label><input type=\"checkbox\" name=\"use_static_ip\" %s onclick=\"tg(this)\"> Use static IP (disable DHCP)</label>",
             current_settings.use_static_ip ? "checked" : "");
    httpd_resp_sendstr_chunk(req, tmp);
    const char *disabled_attr = current_settings.use_static_ip ? "" : "disabled";

    httpd_resp_sendstr_chunk(req, "<div class=\"row\">\n");
    snprintf(tmp, sizeof(tmp), "<div><label>IP Address</label><input class=\"static-field\" %s type=\"text\" name=\"ip_addr\" value=\"%s\" placeholder=\"192.168.1.100\"></div>\n",
             disabled_attr,
             current_settings.ip_addr[0] ? current_settings.ip_addr : "");
    httpd_resp_sendstr_chunk(req, tmp);
    snprintf(tmp, sizeof(tmp), "<div><label>Netmask</label><input class=\"static-field\" %s type=\"text\" name=\"netmask\" value=\"%s\" placeholder=\"255.255.255.0\"></div>\n",
             disabled_attr,
             current_settings.netmask[0] ? current_settings.netmask : "");
    httpd_resp_sendstr_chunk(req, tmp);
    httpd_resp_sendstr_chunk(req, "</div><div class=\"row\">\n");
    snprintf(tmp, sizeof(tmp), "<div><label>Gateway</label><input class=\"static-field\" %s type=\"text\" name=\"gateway\" value=\"%s\" placeholder=\"192.168.1.1\"></div>\n",
             disabled_attr,
             current_settings.gateway[0] ? current_settings.gateway : "");
    httpd_resp_sendstr_chunk(req, tmp);
    snprintf(tmp, sizeof(tmp), "<div><label>DNS 1</label><input class=\"static-field\" %s type=\"text\" name=\"dns1\" value=\"%s\" placeholder=\"8.8.8.8\"></div>\n",
             disabled_attr,
             current_settings.dns1[0] ? current_settings.dns1 : "");
    httpd_resp_sendstr_chunk(req, tmp);
    httpd_resp_sendstr_chunk(req, "</div>\n");
    snprintf(tmp, sizeof(tmp), "<label>DNS 2</label><input class=\"static-field\" %s type=\"text\" name=\"dns2\" value=\"%s\" placeholder=\"1.1.1.1\">\n",
             disabled_attr,
             current_settings.dns2[0] ? current_settings.dns2 : "");
    httpd_resp_sendstr_chunk(req, tmp);

    httpd_resp_sendstr_chunk(req, "<div class=\"actions\"><button type=\"submit\">Save and Reboot</button></div>\n");
    httpd_resp_sendstr_chunk(req, "</fieldset>\n");
    httpd_resp_sendstr_chunk(req, PAGE_TAIL);
    return httpd_resp_sendstr_chunk(req, NULL);
}

static esp_err_t save_post_handler(httpd_req_t *req) {
    char buf[512];
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
    char use_static_ip_str[8];
    char ip_addr_str[32];
    char netmask_str[32];
    char gateway_str[32];
    char dns1_str[32];
    char dns2_str[32];

    if (httpd_query_key_value(buf, "baud_rate", baud_rate_str, sizeof(baud_rate_str)) == ESP_OK &&
        httpd_query_key_value(buf, "tcp_port", tcp_port_str, sizeof(tcp_port_str)) == ESP_OK) {

        new_settings.uart_baud_rate = atoi(baud_rate_str);
        new_settings.tcp_port = atoi(tcp_port_str);

        // Optional fields
        new_settings.use_static_ip = (httpd_query_key_value(buf, "use_static_ip", use_static_ip_str, sizeof(use_static_ip_str)) == ESP_OK) ? 1 : 0;
        if (httpd_query_key_value(buf, "ip_addr", ip_addr_str, sizeof(ip_addr_str)) == ESP_OK) {
            strncpy(new_settings.ip_addr, ip_addr_str, sizeof(new_settings.ip_addr));
            new_settings.ip_addr[sizeof(new_settings.ip_addr)-1] = '\0';
        } else {
            new_settings.ip_addr[0] = '\0';
        }
        if (httpd_query_key_value(buf, "netmask", netmask_str, sizeof(netmask_str)) == ESP_OK) {
            strncpy(new_settings.netmask, netmask_str, sizeof(new_settings.netmask));
            new_settings.netmask[sizeof(new_settings.netmask)-1] = '\0';
        } else {
            new_settings.netmask[0] = '\0';
        }
        if (httpd_query_key_value(buf, "gateway", gateway_str, sizeof(gateway_str)) == ESP_OK) {
            strncpy(new_settings.gateway, gateway_str, sizeof(new_settings.gateway));
            new_settings.gateway[sizeof(new_settings.gateway)-1] = '\0';
        } else {
            new_settings.gateway[0] = '\0';
        }
        if (httpd_query_key_value(buf, "dns1", dns1_str, sizeof(dns1_str)) == ESP_OK) {
            strncpy(new_settings.dns1, dns1_str, sizeof(new_settings.dns1));
            new_settings.dns1[sizeof(new_settings.dns1)-1] = '\0';
        } else {
            new_settings.dns1[0] = '\0';
        }
        if (httpd_query_key_value(buf, "dns2", dns2_str, sizeof(dns2_str)) == ESP_OK) {
            strncpy(new_settings.dns2, dns2_str, sizeof(new_settings.dns2));
            new_settings.dns2[sizeof(new_settings.dns2)-1] = '\0';
        } else {
            new_settings.dns2[0] = '\0';
        }

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
    // Increase stack size to avoid overflow when rendering pages
    if (config.stack_size < 6144) {
        config.stack_size = 6144;
    }
    
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
