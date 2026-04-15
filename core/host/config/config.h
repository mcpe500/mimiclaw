#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#define MIMI_CONFIG_PATH_MAX 256

typedef struct {
    char provider[32];
    char api_key[256];
    char model[64];
    char api_url[256];
    int max_tokens;
} mimi_llm_config_t;

typedef struct {
    bool enabled;
    char token[256];
} mimi_channel_config_t;

typedef struct {
    int port;
} mimi_ws_config_t;

typedef struct {
    int port;
} mimi_admin_config_t;

typedef struct {
    char data_dir[MIMI_CONFIG_PATH_MAX];
    mimi_llm_config_t llm;
    mimi_channel_config_t telegram;
    mimi_channel_config_t discord;
    mimi_channel_config_t slack;
    mimi_channel_config_t whatsapp;
    mimi_ws_config_t websocket;
    mimi_admin_config_t admin;
    char timezone[32];
} mimi_config_t;

esp_err_t config_init(const char *config_path);
const mimi_config_t *config_get(void);
esp_err_t config_set(const mimi_config_t *cfg);
esp_err_t config_load(void);
esp_err_t config_save(void);
bool config_exists(void);
void config_defaults(mimi_config_t *cfg);
const char *config_data_dir(void);
