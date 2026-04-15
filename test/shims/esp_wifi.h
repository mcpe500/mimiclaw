#pragma once
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

typedef int wifi_err_reason_t;
#define WIFI_REASON_AUTH_EXPIRE 1
#define WIFI_REASON_AUTH_FAIL 2
#define WIFI_REASON_ASSOC_EXPIRE 3
#define WIFI_REASON_ASSOC_FAIL 4
#define WIFI_REASON_HANDSHAKE_TIMEOUT 5
#define WIFI_REASON_NO_AP_FOUND 6
#define WIFI_REASON_BEACON_TIMEOUT 7
#define WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT 8
#define WIFI_REASON_MIC_FAILURE 9
#define WIFI_REASON_CONNECTION_FAIL 10

#define WIFI_EVENT 1
#define WIFI_EVENT_STA_START 2
#define WIFI_EVENT_STA_DISCONNECTED 3

#define WIFI_MODE_STA   1
#define WIFI_MODE_AP    2
#define WIFI_MODE_APSTA 3

#define WIFI_IF_STA 0
#define WIFI_IF_AP  1

typedef struct {
    char sta_ssid[32];
    char sta_password[64];
} wifi_sta_config_t;

typedef struct {
    char ap_ssid[32];
    char ap_password[64];
} wifi_ap_config_t;

typedef union {
    wifi_sta_config_t sta;
    wifi_ap_config_t ap;
} wifi_config_t;

typedef struct {
    char ssid[33];
    uint8_t bssid[6];
    int rssi;
} wifi_ap_record_t;

typedef struct {
    uint8_t *ssid;
    uint8_t ssid_len;
    uint8_t *bssid;
    uint8_t channel;
    bool show_hidden;
} wifi_scan_config_t;

typedef struct {
    wifi_err_reason_t reason;
} wifi_event_sta_disconnected_t;

static inline esp_err_t esp_wifi_init(void) { return ESP_OK; }
static inline esp_err_t esp_wifi_set_mode(int mode) { (void)mode; return ESP_OK; }
static inline esp_err_t esp_wifi_start(void) { return ESP_OK; }
static inline esp_err_t esp_wifi_connect(void) { return ESP_OK; }
static inline esp_err_t esp_wifi_disconnect(void) { return ESP_OK; }
static inline esp_err_t esp_wifi_set_config(int iface, const wifi_config_t *conf) { (void)iface; (void)conf; return ESP_OK; }
static inline esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *conf, bool block) { (void)conf; (void)block; return ESP_OK; }
static inline esp_err_t esp_wifi_scan_get_ap_num(uint16_t *num) { (void)num; if (num) *num = 0; return ESP_OK; }
static inline esp_err_t esp_wifi_scan_get_ap_records(uint16_t *num, wifi_ap_record_t *aps) { (void)num; (void)aps; return ESP_OK; }
