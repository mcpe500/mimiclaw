#include "unity.h"
#include "channel.h"
#include <string.h>
#include <stdlib.h>

void setUp(void) {}
void tearDown(void) {}

void test_channel_struct_exists(void)
{
    channel_t chan = {0};
    TEST_ASSERT_NOT_NULL(&chan);
}

void test_channel_vtable_exists(void)
{
    channel_vtable_t vtable = {0};
    TEST_ASSERT_NOT_NULL(&vtable);
    TEST_ASSERT_NOT_NULL(vtable.connect);
    TEST_ASSERT_NOT_NULL(vtable.disconnect);
    TEST_ASSERT_NOT_NULL(vtable.poll);
    TEST_ASSERT_NOT_NULL(vtable.send);
    TEST_ASSERT_NOT_NULL(vtable.free);
}

void test_channel_caps_struct(void)
{
    channel_caps_t caps = {
        .supports_pairing = true,
        .supports_groups = false,
        .supports_media = true,
        .supports_inline_media = false
    };
    TEST_ASSERT_TRUE(caps.supports_pairing);
    TEST_ASSERT_FALSE(caps.supports_groups);
    TEST_ASSERT_TRUE(caps.supports_media);
    TEST_ASSERT_FALSE(caps.supports_inline_media);
}

void test_channel_telegram_create(void)
{
    channel_t *chan = channel_telegram_create();
    TEST_ASSERT_NOT_NULL(chan);
    TEST_ASSERT_EQUAL_STRING("telegram", chan->channel_id);
    TEST_ASSERT_EQUAL_STRING("telegram", chan->name);
    TEST_ASSERT_TRUE(chan->caps.supports_pairing);
    TEST_ASSERT_TRUE(chan->caps.supports_groups);
    TEST_ASSERT_TRUE(chan->caps.supports_media);
    free(chan);
}

void test_channel_feishu_create(void)
{
    channel_t *chan = channel_feishu_create();
    TEST_ASSERT_NOT_NULL(chan);
    TEST_ASSERT_EQUAL_STRING("feishu", chan->channel_id);
    TEST_ASSERT_EQUAL_STRING("feishu", chan->name);
    TEST_ASSERT_FALSE(chan->caps.supports_pairing);
    TEST_ASSERT_TRUE(chan->caps.supports_groups);
    TEST_ASSERT_FALSE(chan->caps.supports_media);
    free(chan);
}

void test_channel_websocket_create(void)
{
    channel_t *chan = channel_websocket_create();
    TEST_ASSERT_NOT_NULL(chan);
    TEST_ASSERT_EQUAL_STRING("websocket", chan->channel_id);
    TEST_ASSERT_EQUAL_STRING("websocket", chan->name);
    TEST_ASSERT_FALSE(chan->caps.supports_pairing);
    TEST_ASSERT_FALSE(chan->caps.supports_groups);
    TEST_ASSERT_FALSE(chan->caps.supports_media);
    free(chan);
}

void test_channel_get_returns_null_for_unknown(void)
{
    channel_t *chan = channel_get("unknown");
    TEST_ASSERT_NULL(chan);
}

void test_channel_manager_max_channels(void)
{
    for (int i = 0; i < 8; i++) {
        channel_t *chan = channel_telegram_create();
        TEST_ASSERT_NOT_NULL(chan);
    }
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_channel_struct_exists);
    RUN_TEST(test_channel_vtable_exists);
    RUN_TEST(test_channel_caps_struct);
    RUN_TEST(test_channel_telegram_create);
    RUN_TEST(test_channel_feishu_create);
    RUN_TEST(test_channel_websocket_create);
    RUN_TEST(test_channel_get_returns_null_for_unknown);
    RUN_TEST(test_channel_manager_max_channels);
    return UNITY_END();
}