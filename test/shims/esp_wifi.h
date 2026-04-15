#pragma once
#include "esp_err.h"

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

typedef struct {
    wifi_err_reason_t reason;
} wifi_event_sta_disconnected_t;

static inline esp_err_t esp_wifi_init(void) { return ESP_OK; }
static inline esp_err_t esp_wifi_set_mode(int mode) { (void)mode; return ESP_OK; }
static inline esp_err_t esp_wifi_start(void) { return ESP_OK; }
static inline esp_err_t esp_wifi_connect(void) { return ESP_OK; }
static inline esp_err_t esp_wifi_disconnect(void) { return ESP_OK; }
