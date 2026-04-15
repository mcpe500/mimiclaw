#pragma once
#include "esp_err.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

esp_err_t admin_server_start(void);
esp_err_t admin_server_stop(void);

#ifdef __cplusplus
}
#endif
