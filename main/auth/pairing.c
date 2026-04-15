#include "pairing.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

static const char *TAG = "auth";

static auth_config_t s_config = {
    .dm_policy = DM_POLICY_OPEN,
    .group_policy = GROUP_POLICY_OPEN,
    .allowFrom_count = 0,
    .rate_limit_per_min = 60
};

#define MAX_PAIRED_USERS 256
static char s_paired_users[MAX_PAIRED_USERS][64];
static int s_paired_count = 0;

#define RATE_LIMIT_WINDOW 60
typedef struct {
    char user_id[64];
    time_t last_check;
    int count;
} rate_limit_entry_t;

static rate_limit_entry_t s_rate_limits[128];
static int s_rate_limit_count = 0;

static bool is_in_allowlist(const char *user_id)
{
    for (int i = 0; i < s_config.allowFrom_count; i++) {
        if (strcmp(s_config.allowFrom[i], user_id) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_paired_user(const char *user_id)
{
    for (int i = 0; i < s_paired_count; i++) {
        if (strcmp(s_paired_users[i], user_id) == 0) {
            return true;
        }
    }
    return false;
}

esp_err_t auth_init(void)
{
    ESP_LOGI(TAG, "Auth initialized: DM=%d, GROUP=%d", s_config.dm_policy, s_config.group_policy);
    return ESP_OK;
}

esp_err_t auth_check_dm(const char *chat_id, const char *user_id)
{
    (void)chat_id;
    
    switch (s_config.dm_policy) {
        case DM_POLICY_DISABLED:
            return ESP_ERR_NOT_FOUND;
        case DM_POLICY_OPEN:
            return ESP_OK;
        case DM_POLICY_ALLOWLIST:
            return is_in_allowlist(user_id) ? ESP_OK : ESP_ERR_NOT_FOUND;
        case DM_POLICY_PAIRING:
            return is_paired_user(user_id) ? ESP_OK : ESP_ERR_NOT_FOUND;
    }
    return ESP_ERR_INVALID_STATE;
}

esp_err_t auth_check_group(const char *chat_id, const char *user_id, bool mentioned)
{
    (void)chat_id;
    
    switch (s_config.group_policy) {
        case GROUP_POLICY_OPEN:
            return ESP_OK;
        case GROUP_POLICY_ALLOWLIST:
            return is_in_allowlist(user_id) ? ESP_OK : ESP_ERR_NOT_FOUND;
        case GROUP_POLICY_MENTION_ONLY:
            return mentioned ? ESP_OK : ESP_ERR_NOT_FOUND;
    }
    return ESP_ERR_INVALID_STATE;
}

esp_err_t auth_set_allowlist(const char *user_id)
{
    if (s_config.allowFrom_count >= 32) {
        return ESP_ERR_NO_MEM;
    }
    strncpy(s_config.allowFrom[s_config.allowFrom_count++], user_id, 63);
    ESP_LOGI(TAG, "Added to allowlist: %s", user_id);
    return ESP_OK;
}

esp_err_t auth_remove_allowlist(const char *user_id)
{
    for (int i = 0; i < s_config.allowFrom_count; i++) {
        if (strcmp(s_config.allowFrom[i], user_id) == 0) {
            for (int j = i; j < s_config.allowFrom_count - 1; j++) {
                strcpy(s_config.allowFrom[j], s_config.allowFrom[j + 1]);
            }
            s_config.allowFrom_count--;
            ESP_LOGI(TAG, "Removed from allowlist: %s", user_id);
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t auth_set_dm_policy(dm_policy_t policy)
{
    s_config.dm_policy = policy;
    ESP_LOGI(TAG, "DM policy set to: %d", policy);
    return ESP_OK;
}

esp_err_t auth_set_group_policy(group_policy_t policy)
{
    s_config.group_policy = policy;
    ESP_LOGI(TAG, "Group policy set to: %d", policy);
    return ESP_OK;
}

bool auth_rate_limit_check(const char *user_id)
{
    if (s_config.rate_limit_per_min == 0) {
        return true;
    }
    
    time_t now = time(NULL);
    
    for (int i = 0; i < s_rate_limit_count; i++) {
        if (strcmp(s_rate_limits[i].user_id, user_id) == 0) {
            if (now - s_rate_limits[i].last_check < RATE_LIMIT_WINDOW) {
                if (s_rate_limits[i].count >= s_config.rate_limit_per_min) {
                    return false;
                }
                s_rate_limits[i].count++;
                return true;
            }
            s_rate_limits[i].count = 1;
            s_rate_limits[i].last_check = now;
            return true;
        }
    }
    
    if (s_rate_limit_count < 128) {
        strncpy(s_rate_limits[s_rate_limit_count].user_id, user_id, 63);
        s_rate_limits[s_rate_limit_count].count = 1;
        s_rate_limits[s_rate_limit_count].last_check = now;
        s_rate_limit_count++;
        return true;
    }
    
    return true;
}

bool auth_is_paired(const char *user_id)
{
    return is_paired_user(user_id);
}

esp_err_t auth_pair_user(const char *user_id)
{
    if (s_paired_count >= MAX_PAIRED_USERS) {
        return ESP_ERR_NO_MEM;
    }
    strncpy(s_paired_users[s_paired_count++], user_id, 63);
    ESP_LOGI(TAG, "User paired: %s", user_id);
    return ESP_OK;
}

auth_config_t *auth_get_config(void)
{
    return &s_config;
}