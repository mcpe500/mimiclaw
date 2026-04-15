#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char *base_path;
    const char *partition_label;
    uint32_t max_files;
    bool format_if_mount_failed;
} esp_vfs_spiffs_conf_t;

static inline esp_err_t esp_vfs_spiffs_register(const esp_vfs_spiffs_conf_t *conf) { (void)conf; return ESP_OK; }
static inline esp_err_t esp_spiffs_info(const char *path, size_t *total, size_t *used) { (void)path; *total = 1024; *used = 512; return ESP_OK; }
