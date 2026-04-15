#include "unity.h"
#include "auth/pairing.h"
#include <string.h>
#include <stdlib.h>

static auth_config_t *test_config;

void setUp(void)
{
    test_config = auth_get_config();
}

void tearDown(void)
{
}

void test_auth_struct_exists(void)
{
    auth_config_t config = {0};
    TEST_ASSERT_NOT_NULL(&config);
}

void test_dm_policy_enum(void)
{
    TEST_ASSERT_EQUAL(0, DM_POLICY_PAIRING);
    TEST_ASSERT_EQUAL(1, DM_POLICY_ALLOWLIST);
    TEST_ASSERT_EQUAL(2, DM_POLICY_OPEN);
    TEST_ASSERT_EQUAL(3, DM_POLICY_DISABLED);
}

void test_group_policy_enum(void)
{
    TEST_ASSERT_EQUAL(0, GROUP_POLICY_ALLOWLIST);
    TEST_ASSERT_EQUAL(1, GROUP_POLICY_MENTION_ONLY);
    TEST_ASSERT_EQUAL(2, GROUP_POLICY_OPEN);
}

void test_auth_init(void)
{
    esp_err_t err = auth_init();
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_auth_check_dm_open_policy(void)
{
    auth_set_dm_policy(DM_POLICY_OPEN);
    esp_err_t err = auth_check_dm("123", "user1");
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_auth_check_dm_disabled_policy(void)
{
    auth_set_dm_policy(DM_POLICY_DISABLED);
    esp_err_t err = auth_check_dm("123", "user1");
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, err);
}

void test_auth_check_dm_allowlist_policy(void)
{
    auth_set_dm_policy(DM_POLICY_ALLOWLIST);
    auth_set_allowlist("allowed_user");
    esp_err_t err = auth_check_dm("123", "allowed_user");
    TEST_ASSERT_EQUAL(ESP_OK, err);
    err = auth_check_dm("123", "other_user");
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, err);
}

void test_auth_check_dm_pairing_policy(void)
{
    auth_set_dm_policy(DM_POLICY_PAIRING);
    auth_pair_user("paired_user");
    esp_err_t err = auth_check_dm("123", "paired_user");
    TEST_ASSERT_EQUAL(ESP_OK, err);
    err = auth_check_dm("123", "unpaired_user");
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, err);
}

void test_auth_check_group_open_policy(void)
{
    auth_set_group_policy(GROUP_POLICY_OPEN);
    esp_err_t err = auth_check_group("group1", "user1", false);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_auth_check_group_allowlist_policy(void)
{
    auth_set_group_policy(GROUP_POLICY_ALLOWLIST);
    auth_set_allowlist("group_allowed");
    esp_err_t err = auth_check_group("group1", "group_allowed", false);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    err = auth_check_group("group1", "not_allowed", false);
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, err);
}

void test_auth_check_group_mention_only_policy(void)
{
    auth_set_group_policy(GROUP_POLICY_MENTION_ONLY);
    esp_err_t err = auth_check_group("group1", "any_user", false);
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, err);
    err = auth_check_group("group1", "any_user", true);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_auth_set_allowlist(void)
{
    int initial_count = test_config->allowFrom_count;
    esp_err_t err = auth_set_allowlist("new_user");
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(initial_count + 1, test_config->allowFrom_count);
}

void test_auth_remove_allowlist(void)
{
    auth_set_allowlist("to_remove");
    int count_before = test_config->allowFrom_count;
    esp_err_t err = auth_remove_allowlist("to_remove");
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(count_before - 1, test_config->allowFrom_count);
}

void test_auth_remove_allowlist_not_found(void)
{
    esp_err_t err = auth_remove_allowlist("nonexistent");
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, err);
}

void test_auth_pair_user(void)
{
    esp_err_t err = auth_pair_user("new_pair");
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_TRUE(auth_is_paired("new_pair"));
}

void test_auth_is_paired_unknown_user(void)
{
    TEST_ASSERT_FALSE(auth_is_paired("unknown_user_xyz"));
}

void test_auth_rate_limit_default_allows(void)
{
    bool allowed = auth_rate_limit_check("any_user");
    TEST_ASSERT_TRUE(allowed);
}

void test_auth_set_dm_policy(void)
{
    esp_err_t err = auth_set_dm_policy(DM_POLICY_PAIRING);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(DM_POLICY_PAIRING, test_config->dm_policy);
}

void test_auth_set_group_policy(void)
{
    esp_err_t err = auth_set_group_policy(GROUP_POLICY_ALLOWLIST);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(GROUP_POLICY_ALLOWLIST, test_config->group_policy);
}

void test_auth_get_config(void)
{
    auth_config_t *cfg = auth_get_config();
    TEST_ASSERT_NOT_NULL(cfg);
    TEST_ASSERT_EQUAL(DM_POLICY_OPEN, cfg->dm_policy);
    TEST_ASSERT_EQUAL(GROUP_POLICY_OPEN, cfg->group_policy);
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_auth_struct_exists);
    RUN_TEST(test_dm_policy_enum);
    RUN_TEST(test_group_policy_enum);
    RUN_TEST(test_auth_init);
    RUN_TEST(test_auth_check_dm_open_policy);
    RUN_TEST(test_auth_check_dm_disabled_policy);
    RUN_TEST(test_auth_check_dm_allowlist_policy);
    RUN_TEST(test_auth_check_dm_pairing_policy);
    RUN_TEST(test_auth_check_group_open_policy);
    RUN_TEST(test_auth_check_group_allowlist_policy);
    RUN_TEST(test_auth_check_group_mention_only_policy);
    RUN_TEST(test_auth_set_allowlist);
    RUN_TEST(test_auth_remove_allowlist);
    RUN_TEST(test_auth_remove_allowlist_not_found);
    RUN_TEST(test_auth_pair_user);
    RUN_TEST(test_auth_is_paired_unknown_user);
    RUN_TEST(test_auth_rate_limit_default_allows);
    RUN_TEST(test_auth_set_dm_policy);
    RUN_TEST(test_auth_set_group_policy);
    RUN_TEST(test_auth_get_config);
    return UNITY_END();
}