#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DM_POLICY_PAIRING,
    DM_POLICY_ALLOWLIST,
    DM_POLICY_OPEN,
    DM_POLICY_DISABLED
} dm_policy_t;

typedef enum {
    GROUP_POLICY_ALLOWLIST,
    GROUP_POLICY_MENTION_ONLY,
    GROUP_POLICY_OPEN
} group_policy_t;

typedef struct {
    dm_policy_t dm_policy;
    group_policy_t group_policy;
    char allowFrom[32][64];
    int allowFrom_count;
    int rate_limit_per_min;
} auth_config_t;

esp_err_t auth_init(void);

esp_err_t auth_check_dm(const char *chat_id, const char *user_id);

esp_err_t auth_check_group(const char *chat_id, const char *user_id, bool mentioned);

esp_err_t auth_set_allowlist(const char *user_id);

esp_err_t auth_remove_allowlist(const char *user_id);

esp_err_t auth_set_dm_policy(dm_policy_t policy);

esp_err_t auth_set_group_policy(group_policy_t policy);

bool auth_rate_limit_check(const char *user_id);

bool auth_is_paired(const char *user_id);

esp_err_t auth_pair_user(const char *user_id);

auth_config_t *auth_get_config(void);

#ifdef __cplusplus
}
#endif