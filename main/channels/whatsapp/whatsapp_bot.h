#pragma once

#include "esp_err.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t whatsapp_bot_init(void);
esp_err_t whatsapp_bot_start(void);
esp_err_t whatsapp_send_message(const char *phone_number, const char *text);
esp_err_t whatsapp_set_credentials(const char *access_token, const char *phone_number_id);
esp_err_t whatsapp_set_verify_token(const char *verify_token);

#ifdef __cplusplus
}
#endif
