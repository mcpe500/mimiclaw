#pragma once

#include "esp_err.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t slack_bot_init(void);
esp_err_t slack_bot_start(void);
esp_err_t slack_send_message(const char *channel_id, const char *text);
esp_err_t slack_set_token(const char *token);

#ifdef __cplusplus
}
#endif
