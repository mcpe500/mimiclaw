#include "unity.h"
#include "memory/session_mgr.h"
#include <string.h>
#include <stdlib.h>

void setUp(void) {}
void tearDown(void) {}

void test_session_init(void)
{
    esp_err_t err = session_mgr_init();
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_session_append_signature(void)
{
    esp_err_t err = session_append("chat1", "user1", "user", "test message");
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_session_get_history_json_signature(void)
{
    char buf[4096];
    esp_err_t err = session_get_history_json("chat1", "user1", buf, sizeof(buf), 10);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL_STRING("[]", buf);
}

void test_session_clear_signature(void)
{
    session_append("chat_to_clear", "user_clear", "user", "temp");
    esp_err_t err = session_clear("chat_to_clear", "user_clear");
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_session_clear_not_found(void)
{
    esp_err_t err = session_clear("nonexistent_chat", "nonexistent_user");
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, err);
}

void test_session_isolation_different_users(void)
{
    session_append("same_chat", "user_a", "user", "message from A");
    session_append("same_chat", "user_b", "user", "message from B");

    char buf_a[4096];
    char buf_b[4096];

    session_get_history_json("same_chat", "user_a", buf_a, sizeof(buf_a), 10);
    session_get_history_json("same_chat", "user_b", buf_b, sizeof(buf_b), 10);

    TEST_ASSERT_TRUE(strstr(buf_a, "from A") != NULL);
    TEST_ASSERT_TRUE(strstr(buf_b, "from B") != NULL);
    TEST_ASSERT_TRUE(strstr(buf_a, "from B") == NULL);

    session_clear("same_chat", "user_a");
    session_clear("same_chat", "user_b");
}

void test_session_list_does_not_crash(void)
{
    session_list();
    TEST_ASSERT_TRUE(true);
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_session_init);
    RUN_TEST(test_session_append_signature);
    RUN_TEST(test_session_get_history_json_signature);
    RUN_TEST(test_session_clear_signature);
    RUN_TEST(test_session_clear_not_found);
    RUN_TEST(test_session_isolation_different_users);
    RUN_TEST(test_session_list_does_not_crash);
    return UNITY_END();
}