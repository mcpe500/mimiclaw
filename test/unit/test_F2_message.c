#include "unity.h"
#include "message_bus.h"
#include <string.h>
#include <stdlib.h>

void setUp(void) {}
void tearDown(void) {}

void test_msg_struct_has_base_fields(void)
{
    mimi_msg_t msg = {0};
    strncpy(msg.channel, "telegram", sizeof(msg.channel) - 1);
    strncpy(msg.chat_id, "123456", sizeof(msg.chat_id) - 1);
    msg.content = strdup("Hello");
    
    TEST_ASSERT_EQUAL_STRING("telegram", msg.channel);
    TEST_ASSERT_EQUAL_STRING("123456", msg.chat_id);
    TEST_ASSERT_EQUAL_STRING("Hello", msg.content);
    
    free(msg.content);
}

void test_msg_struct_has_user_id(void)
{
    mimi_msg_t msg = {0};
    strncpy(msg.user_id, "user123", sizeof(msg.user_id) - 1);
    TEST_ASSERT_EQUAL_STRING("user123", msg.user_id);
}

void test_msg_struct_has_message_id(void)
{
    mimi_msg_t msg = {0};
    strncpy(msg.message_id, "msg_abc", sizeof(msg.message_id) - 1);
    TEST_ASSERT_EQUAL_STRING("msg_abc", msg.message_id);
}

void test_msg_struct_has_media_fields(void)
{
    mimi_msg_t msg = {0};
    msg.media_count = 2;
    strncpy(msg.media_paths[0], "/sdcard/img1.jpg", sizeof(msg.media_paths[0]) - 1);
    strncpy(msg.media_paths[1], "/sdcard/img2.jpg", sizeof(msg.media_paths[1]) - 1);
    
    TEST_ASSERT_EQUAL(2, msg.media_count);
    TEST_ASSERT_EQUAL_STRING("/sdcard/img1.jpg", msg.media_paths[0]);
    TEST_ASSERT_EQUAL_STRING("/sdcard/img2.jpg", msg.media_paths[1]);
}

void test_msg_struct_media_count_limit(void)
{
    mimi_msg_t msg = {0};
    msg.media_count = 4;
    TEST_ASSERT_EQUAL(4, msg.media_count);
    msg.media_count = 5;
    TEST_ASSERT_EQUAL(5, msg.media_count);
}

void test_msg_struct_has_reply_to(void)
{
    mimi_msg_t msg = {0};
    strncpy(msg.reply_to, "reply_msg", sizeof(msg.reply_to) - 1);
    TEST_ASSERT_EQUAL_STRING("reply_to", msg.reply_to);
}

void test_msg_struct_has_metadata(void)
{
    mimi_msg_t msg = {0};
    strncpy(msg.metadata, "{\"key\":\"value\"}", sizeof(msg.metadata) - 1);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", msg.metadata);
}

void test_msg_backward_compatibility(void)
{
    mimi_msg_t msg = {0};
    strncpy(msg.channel, "telegram", sizeof(msg.channel) - 1);
    strncpy(msg.chat_id, "123", sizeof(msg.chat_id) - 1);
    msg.content = strdup("test");
    
    TEST_ASSERT_NOT_NULL(msg.content);
    TEST_ASSERT_EQUAL_STRING("telegram", msg.channel);
    TEST_ASSERT_EQUAL_STRING("123", msg.chat_id);
    TEST_ASSERT_EQUAL_STRING("test", msg.content);
    
    free(msg.content);
}

void test_msg_all_fields_empty_by_default(void)
{
    mimi_msg_t msg = {0};
    TEST_ASSERT_EQUAL(0, msg.media_count);
    TEST_ASSERT_EQUAL_STRING("", msg.user_id);
    TEST_ASSERT_EQUAL_STRING("", msg.message_id);
    TEST_ASSERT_EQUAL_STRING("", msg.reply_to);
    TEST_ASSERT_EQUAL_STRING("", msg.metadata);
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_msg_struct_has_base_fields);
    RUN_TEST(test_msg_struct_has_user_id);
    RUN_TEST(test_msg_struct_has_message_id);
    RUN_TEST(test_msg_struct_has_media_fields);
    RUN_TEST(test_msg_struct_media_count_limit);
    RUN_TEST(test_msg_struct_has_reply_to);
    RUN_TEST(test_msg_struct_has_metadata);
    RUN_TEST(test_msg_backward_compatibility);
    RUN_TEST(test_msg_all_fields_empty_by_default);
    return UNITY_END();
}