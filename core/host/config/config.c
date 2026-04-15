#include "config.h"
#include "esp_log.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *TAG = "config";

static mimi_config_t s_config;
static char s_config_path[MIMI_CONFIG_PATH_MAX] = {0};
static bool s_initialized = false;

static void str_copy(char *dst, const char *src, size_t dst_size) {
    if (src) {
        strncpy(dst, src, dst_size - 1);
        dst[dst_size - 1] = '\0';
    }
}

static void mkdir_recursive(const char *path) {
    char tmp[MIMI_CONFIG_PATH_MAX];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    size_t len = strlen(tmp);
    if (len > 0 && (tmp[len - 1] == '/' || tmp[len - 1] == '\\')) {
        tmp[len - 1] = '\0';
    }
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            struct stat st = {0};
            if (stat(tmp, &st) != 0) {
                mkdir(tmp, 0755);
            }
            *p = '/';
        }
    }
    struct stat st = {0};
    if (stat(tmp, &st) != 0) {
        mkdir(tmp, 0755);
    }
}

static void extract_dir(const char *filepath, char *dir, size_t dir_size) {
    strncpy(dir, filepath, dir_size - 1);
    dir[dir_size - 1] = '\0';
    char *sep = strrchr(dir, '/');
    if (!sep) sep = strrchr(dir, '\\');
    if (sep) *sep = '\0';
}

void config_defaults(mimi_config_t *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    str_copy(cfg->data_dir, "~/.mimiclaw", sizeof(cfg->data_dir));
    str_copy(cfg->llm.provider, "anthropic", sizeof(cfg->llm.provider));
    str_copy(cfg->llm.model, "claude-sonnet-4-5", sizeof(cfg->llm.model));
    str_copy(cfg->llm.api_url, "", sizeof(cfg->llm.api_url));
    cfg->llm.max_tokens = 4096;
    cfg->websocket.port = 18789;
    cfg->admin.port = 8080;
    str_copy(cfg->timezone, "UTC0", sizeof(cfg->timezone));
    cfg->telegram.enabled = false;
    cfg->discord.enabled = false;
    cfg->slack.enabled = false;
    cfg->whatsapp.enabled = false;
}

esp_err_t config_init(const char *config_path) {
    if (config_path) {
        strncpy(s_config_path, config_path, sizeof(s_config_path) - 1);
        s_config_path[sizeof(s_config_path) - 1] = '\0';
    } else {
        const char *home = getenv("HOME");
        if (!home) home = getenv("USERPROFILE");
        if (home) {
            snprintf(s_config_path, sizeof(s_config_path),
                     "%s/.mimiclaw/config.json", home);
        } else {
            str_copy(s_config_path, ".mimiclaw/config.json",
                     sizeof(s_config_path));
        }
    }
    config_defaults(&s_config);
    s_initialized = true;
    ESP_LOGI(TAG, "config path: %s", s_config_path);
    return ESP_OK;
}

const mimi_config_t *config_get(void) {
    return &s_config;
}

esp_err_t config_set(const mimi_config_t *cfg) {
    if (!cfg) return ESP_ERR_INVALID_ARG;
    memcpy(&s_config, cfg, sizeof(s_config));
    return ESP_OK;
}

bool config_exists(void) {
    if (s_config_path[0] == '\0') return false;
    return access(s_config_path, F_OK) == 0;
}

static void load_channel(cJSON *parent, const char *name,
                         mimi_channel_config_t *ch,
                         const mimi_channel_config_t *def) {
    cJSON *obj = cJSON_GetObjectItem(parent, name);
    if (!obj) {
        *ch = *def;
        return;
    }
    cJSON *enabled = cJSON_GetObjectItem(obj, "enabled");
    ch->enabled = enabled ? cJSON_IsTrue(enabled) : def->enabled;
    cJSON *token = cJSON_GetObjectItem(obj, "token");
    if (token && cJSON_IsString(token)) {
        str_copy(ch->token, token->valuestring, sizeof(ch->token));
    } else {
        str_copy(ch->token, def->token, sizeof(ch->token));
    }
}

esp_err_t config_load(void) {
    if (!s_initialized) {
        ESP_LOGE(TAG, "config not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    if (!config_exists()) {
        ESP_LOGW(TAG, "config file not found, using defaults");
        return ESP_ERR_NOT_FOUND;
    }
    FILE *fp = fopen(s_config_path, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "failed to open config file");
        return ESP_FAIL;
    }
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (fsize <= 0 || fsize > 1024 * 64) {
        fclose(fp);
        ESP_LOGE(TAG, "config file size invalid: %ld", fsize);
        return ESP_FAIL;
    }
    char *buf = malloc((size_t)fsize + 1);
    if (!buf) {
        fclose(fp);
        return ESP_ERR_NO_MEM;
    }
    size_t nread = fread(buf, 1, (size_t)fsize, fp);
    fclose(fp);
    buf[nread] = '\0';

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        ESP_LOGE(TAG, "failed to parse config JSON");
        return ESP_FAIL;
    }

    mimi_config_t defaults;
    config_defaults(&defaults);

    cJSON *val;
    val = cJSON_GetObjectItem(root, "data_dir");
    if (val && cJSON_IsString(val))
        str_copy(s_config.data_dir, val->valuestring,
                 sizeof(s_config.data_dir));
    else
        str_copy(s_config.data_dir, defaults.data_dir,
                 sizeof(s_config.data_dir));

    cJSON *llm = cJSON_GetObjectItem(root, "llm");
    if (llm) {
        val = cJSON_GetObjectItem(llm, "provider");
        str_copy(s_config.llm.provider,
                 val && cJSON_IsString(val) ? val->valuestring
                                            : defaults.llm.provider,
                 sizeof(s_config.llm.provider));
        val = cJSON_GetObjectItem(llm, "api_key");
        str_copy(s_config.llm.api_key,
                 val && cJSON_IsString(val) ? val->valuestring
                                            : defaults.llm.api_key,
                 sizeof(s_config.llm.api_key));
        val = cJSON_GetObjectItem(llm, "model");
        str_copy(s_config.llm.model,
                 val && cJSON_IsString(val) ? val->valuestring
                                            : defaults.llm.model,
                 sizeof(s_config.llm.model));
        val = cJSON_GetObjectItem(llm, "api_url");
        str_copy(s_config.llm.api_url,
                 val && cJSON_IsString(val) ? val->valuestring
                                            : defaults.llm.api_url,
                 sizeof(s_config.llm.api_url));
        val = cJSON_GetObjectItem(llm, "max_tokens");
        s_config.llm.max_tokens =
            val && cJSON_IsNumber(val) ? val->valueint
                                       : defaults.llm.max_tokens;
    } else {
        s_config.llm = defaults.llm;
    }

    load_channel(root, "telegram", &s_config.telegram, &defaults.telegram);
    load_channel(root, "discord", &s_config.discord, &defaults.discord);
    load_channel(root, "slack", &s_config.slack, &defaults.slack);
    load_channel(root, "whatsapp", &s_config.whatsapp, &defaults.whatsapp);

    cJSON *ws = cJSON_GetObjectItem(root, "websocket");
    val = ws ? cJSON_GetObjectItem(ws, "port") : NULL;
    s_config.websocket.port =
        val && cJSON_IsNumber(val) ? val->valueint
                                   : defaults.websocket.port;

    cJSON *admin = cJSON_GetObjectItem(root, "admin");
    val = admin ? cJSON_GetObjectItem(admin, "port") : NULL;
    s_config.admin.port =
        val && cJSON_IsNumber(val) ? val->valueint : defaults.admin.port;

    val = cJSON_GetObjectItem(root, "timezone");
    str_copy(s_config.timezone,
             val && cJSON_IsString(val) ? val->valuestring
                                        : defaults.timezone,
             sizeof(s_config.timezone));

    cJSON_Delete(root);
    ESP_LOGI(TAG, "config loaded from %s", s_config_path);
    return ESP_OK;
}

static cJSON *save_channel(const mimi_channel_config_t *ch) {
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddBoolToObject(obj, "enabled", ch->enabled);
    if (ch->token[0]) {
        cJSON_AddStringToObject(obj, "token", ch->token);
    }
    return obj;
}

esp_err_t config_save(void) {
    if (!s_initialized) {
        ESP_LOGE(TAG, "config not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    char dir[MIMI_CONFIG_PATH_MAX];
    extract_dir(s_config_path, dir, sizeof(dir));
    mkdir_recursive(dir);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "data_dir", s_config.data_dir);

    cJSON *llm = cJSON_CreateObject();
    cJSON_AddStringToObject(llm, "provider", s_config.llm.provider);
    if (s_config.llm.api_key[0])
        cJSON_AddStringToObject(llm, "api_key", s_config.llm.api_key);
    cJSON_AddStringToObject(llm, "model", s_config.llm.model);
    if (s_config.llm.api_url[0])
        cJSON_AddStringToObject(llm, "api_url", s_config.llm.api_url);
    cJSON_AddNumberToObject(llm, "max_tokens", s_config.llm.max_tokens);
    cJSON_AddItemToObject(root, "llm", llm);

    cJSON_AddItemToObject(root, "telegram", save_channel(&s_config.telegram));
    cJSON_AddItemToObject(root, "discord", save_channel(&s_config.discord));
    cJSON_AddItemToObject(root, "slack", save_channel(&s_config.slack));
    cJSON_AddItemToObject(root, "whatsapp", save_channel(&s_config.whatsapp));

    cJSON *ws = cJSON_CreateObject();
    cJSON_AddNumberToObject(ws, "port", s_config.websocket.port);
    cJSON_AddItemToObject(root, "websocket", ws);

    cJSON *admin = cJSON_CreateObject();
    cJSON_AddNumberToObject(admin, "port", s_config.admin.port);
    cJSON_AddItemToObject(root, "admin", admin);

    cJSON_AddStringToObject(root, "timezone", s_config.timezone);

    char *json = cJSON_Print(root);
    cJSON_Delete(root);
    if (!json) {
        ESP_LOGE(TAG, "failed to serialize config");
        return ESP_FAIL;
    }

    FILE *fp = fopen(s_config_path, "w");
    if (!fp) {
        free(json);
        ESP_LOGE(TAG, "failed to open config for writing");
        return ESP_FAIL;
    }
    fputs(json, fp);
    fputc('\n', fp);
    fclose(fp);
    free(json);

    ESP_LOGI(TAG, "config saved to %s", s_config_path);
    return ESP_OK;
}

const char *config_data_dir(void) {
    return s_config.data_dir;
}
