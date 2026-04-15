#include "skill_manifest.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "skill_manifest";

static void safe_strcpy(char *dst, const char *src, size_t dst_size)
{
    if (src) {
        strncpy(dst, src, dst_size - 1);
        dst[dst_size - 1] = '\0';
    }
}

esp_err_t skill_manifest_parse(const char *json, skill_manifest_t *manifest)
{
    if (!json || !manifest) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(manifest, 0, sizeof(*manifest));

    cJSON *root = cJSON_Parse(json);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse manifest JSON");
        return ESP_FAIL;
    }

    cJSON *name = cJSON_GetObjectItem(root, "name");
    if (name && cJSON_IsString(name)) {
        safe_strcpy(manifest->name, name->valuestring, sizeof(manifest->name));
    }

    cJSON *version = cJSON_GetObjectItem(root, "version");
    if (version && cJSON_IsString(version)) {
        safe_strcpy(manifest->version, version->valuestring, sizeof(manifest->version));
    }

    cJSON *description = cJSON_GetObjectItem(root, "description");
    if (description && cJSON_IsString(description)) {
        safe_strcpy(manifest->description, description->valuestring, sizeof(manifest->description));
    }

    cJSON *author = cJSON_GetObjectItem(root, "author");
    if (author && cJSON_IsString(author)) {
        safe_strcpy(manifest->author, author->valuestring, sizeof(manifest->author));
    }

    cJSON *tools = cJSON_GetObjectItem(root, "tools_required");
    if (tools && cJSON_IsArray(tools)) {
        int count = cJSON_GetArraySize(tools);
        if (count > 8) count = 8;
        manifest->tool_count = count;
        for (int i = 0; i < count; i++) {
            cJSON *item = cJSON_GetArrayItem(tools, i);
            if (item && cJSON_IsString(item)) {
                safe_strcpy(manifest->tools_required[i], item->valuestring,
                            sizeof(manifest->tools_required[i]));
            }
        }
    }

    cJSON *deps = cJSON_GetObjectItem(root, "dependencies");
    if (deps && cJSON_IsArray(deps)) {
        int count = cJSON_GetArraySize(deps);
        if (count > 8) count = 8;
        manifest->dep_count = count;
        for (int i = 0; i < count; i++) {
            cJSON *item = cJSON_GetArrayItem(deps, i);
            if (item && cJSON_IsString(item)) {
                safe_strcpy(manifest->dependencies[i], item->valuestring,
                            sizeof(manifest->dependencies[i]));
            }
        }
    }

    cJSON_Delete(root);
    return ESP_OK;
}

esp_err_t skill_manifest_validate(const skill_manifest_t *manifest)
{
    if (!manifest) {
        return ESP_ERR_INVALID_ARG;
    }

    if (manifest->name[0] == '\0') {
        ESP_LOGE(TAG, "Manifest missing required field: name");
        return ESP_ERR_INVALID_ARG;
    }

    if (manifest->version[0] == '\0') {
        ESP_LOGE(TAG, "Manifest missing required field: version");
        return ESP_ERR_INVALID_ARG;
    }

    for (int i = 0; manifest->name[i] != '\0'; i++) {
        char c = manifest->name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            ESP_LOGE(TAG, "Invalid character '%c' in manifest name", c);
            return ESP_ERR_INVALID_ARG;
        }
    }

    return ESP_OK;
}
