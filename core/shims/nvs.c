#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "nvs";

static char nvs_path[512] = "";
static int next_handle = 1;

typedef struct {
    int handle;
    char ns[64];
    int mode;
} nvs_entry_t;

#define MAX_ENTRIES 32
static nvs_entry_t entries[MAX_ENTRIES];

static const char *get_nvs_path(void) {
    if (nvs_path[0] == '\0') {
        const char *home = getenv("HOME");
        if (!home) home = ".";
        snprintf(nvs_path, sizeof(nvs_path), "%s/.mimiclaw/nvs.json", home);
    }
    return nvs_path;
}

static void ensure_dir(void) {
    char dir[512];
    const char *path = get_nvs_path();
    strncpy(dir, path, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';
    char *slash = strrchr(dir, '/');
    if (slash) { *slash = '\0'; mkdir(dir, 0755); }
}

static void ensure_file(void) {
    ensure_dir();
    const char *path = get_nvs_path();
    struct stat st;
    if (stat(path, &st) != 0) {
        FILE *f = fopen(path, "w");
        if (f) { fprintf(f, "{}\n"); fclose(f); }
    }
}

esp_err_t nvs_flash_init(void) {
    ensure_file();
    ESP_LOGI(TAG, "NVS initialized at %s", get_nvs_path());
    return ESP_OK;
}

esp_err_t nvs_open(const char *ns, int mode, nvs_handle_t *handle) {
    if (!ns || !handle) return ESP_ERR_INVALID_ARG;
    int idx = -1;
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (entries[i].handle == 0) { idx = i; break; }
    }
    if (idx < 0) return ESP_ERR_NO_MEM;
    entries[idx].handle = next_handle++;
    strncpy(entries[idx].ns, ns, sizeof(entries[idx].ns) - 1);
    entries[idx].mode = mode;
    *handle = entries[idx].handle;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle) {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (entries[i].handle == handle) { memset(&entries[i], 0, sizeof(nvs_entry_t)); return; }
    }
}

esp_err_t nvs_get_str(nvs_handle_t handle, const char *key, char *buf, size_t *len) {
    (void)handle;
    if (!key || !len) return ESP_ERR_INVALID_ARG;
    ensure_file();
    FILE *f = fopen(get_nvs_path(), "r");
    if (!f) return ESP_ERR_NOT_FOUND;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(sz + 1);
    if (!data) { fclose(f); return ESP_ERR_NO_MEM; }
    fread(data, 1, sz, f);
    data[sz] = '\0';
    fclose(f);
    char search_key[256];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    char *pos = strstr(data, search_key);
    if (!pos) { free(data); return ESP_ERR_NOT_FOUND; }
    char *colon = strchr(pos, ':');
    if (!colon) { free(data); return ESP_ERR_NOT_FOUND; }
    char *val_start = strchr(colon + 1, '"');
    if (!val_start) { free(data); return ESP_ERR_NOT_FOUND; }
    val_start++;
    char *val_end = strchr(val_start, '"');
    if (!val_end) { free(data); return ESP_ERR_NOT_FOUND; }
    size_t vlen = val_end - val_start;
    if (!buf || *len < vlen + 1) { *len = vlen + 1; free(data); return ESP_ERR_INVALID_ARG; }
    memcpy(buf, val_start, vlen);
    buf[vlen] = '\0';
    *len = vlen + 1;
    free(data);
    return ESP_OK;
}

esp_err_t nvs_set_str(nvs_handle_t handle, const char *key, const char *val) {
    (void)handle;
    if (!key || !val) return ESP_ERR_INVALID_ARG;
    ensure_file();
    FILE *f = fopen(get_nvs_path(), "r");
    if (!f) return ESP_FAIL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(sz + 1);
    if (!data) { fclose(f); return ESP_ERR_NO_MEM; }
    fread(data, 1, sz, f);
    data[sz] = '\0';
    fclose(f);
    char search_key[256];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    char *pos = strstr(data, search_key);
    if (pos) {
        char *colon = strchr(pos, ':');
        char *val_start = strchr(colon + 1, '"');
        char *val_end = strchr(val_start + 1, '"');
        size_t old_len = val_end - val_start - 1;
        size_t new_len = strlen(val);
        if (old_len == new_len) {
            memcpy((char *)(val_start + 1), val, new_len);
        } else {
            size_t prefix = val_start - data + 1;
            size_t suffix = strlen(val_end + 1);
            char *new_data = malloc(prefix + new_len + 2 + suffix + 1);
            memcpy(new_data, data, prefix);
            memcpy(new_data + prefix, val, new_len);
            new_data[prefix + new_len] = '"';
            memcpy(new_data + prefix + new_len + 1, val_end + 1, suffix + 1);
            free(data);
            data = new_data;
        }
    } else {
        size_t dlen = strlen(data);
        while (dlen > 0 && (data[dlen - 1] == '}' || data[dlen - 1] == '\n' || data[dlen - 1] == ' ')) dlen--;
        size_t need = dlen + (dlen > 1 ? 1 : 0) + strlen(key) + strlen(val) + 8;
        char *new_data = malloc(need);
        size_t off = 0;
        memcpy(new_data, data, dlen); off = dlen;
        if (dlen > 1) { new_data[off++] = ','; }
        off += sprintf(new_data + off, "\"%s\":\"%s\"}", key, val);
        free(data);
        data = new_data;
    }
    f = fopen(get_nvs_path(), "w");
    if (!f) { free(data); return ESP_FAIL; }
    fprintf(f, "%s\n", data);
    fclose(f);
    free(data);
    return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle) {
    (void)handle;
    return ESP_OK;
}
