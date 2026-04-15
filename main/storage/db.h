#pragma once
#include "esp_err.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

esp_err_t db_init(const char *path);
esp_err_t db_exec(const char *sql);
esp_err_t db_query_one(const char *sql, char *result, size_t size);
void db_close(void);

#ifdef __cplusplus
}
#endif
