#pragma once
#include "esp_err.h"
#include <stdint.h>
static inline esp_err_t esp_read_mac(uint8_t *mac, int type) { (void)type; if(mac) memset(mac, 0x02, 6); return 0; }
static inline esp_err_t esp_efuse_mac_get_default(uint8_t *mac) { if(mac) memset(mac, 0x02, 6); return 0; }
