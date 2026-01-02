#ifndef SETTINGS_H
#define SETTINGS_H

#include "esp_err.h"

#define DEFAULT_UART_BAUD_RATE CONFIG_UART_BITRATE
#define DEFAULT_TCP_PORT CONFIG_BRIDGE_PORT
#define DEFAULT_CONFIG_GPIO CONFIG_WEBSERVER_GPIO

typedef struct {
    int uart_baud_rate;
    int tcp_port;
    int use_static_ip; // 0: DHCP, 1: Static
    char ip_addr[16];
    char netmask[16];
    char gateway[16];
    char dns1[16];
    char dns2[16];
} settings_t;

esp_err_t load_settings(settings_t *settings);
esp_err_t save_settings(const settings_t *settings);

void start_webserver(void);
void stop_webserver(void);

#endif // SETTINGS_H
