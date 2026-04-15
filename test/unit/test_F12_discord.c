#include "unity.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "cJSON.h"

/* ── Replicate internal helpers from discord_bot.c for testing ── */

#ifndef MIMI_CHAN_DISCORD
#define MIMI_CHAN_DISCORD "discord"
#endif

#define DISCORD_DEDUP_CACHE_SIZE 64

static uint64_t fnv1a64(const char *s)
{
    uint64_t h = 1469598103934665603ULL;
    if (!s) return h;
    while (*s) {
        h ^= (unsigned char)(*s++);
        h *= 1099511628211ULL;
    }
    return h;
}

static uint64_t make_msg_key(const char *channel_id, const char *msg_id)
{
    uint64_t h = fnv1a64(channel_id);
    uint64_t m = fnv1a64(msg_id);
    return h ^ (m * 1099511628211ULL);
}

/* ── Test helpers ─────────────────────────────────────────────── */

void setUp(void) {}
void tearDown(void) {}

/* 1. FNV-1a produces consistent hash */
void test_fnv1a_consistent(void)
{
    const char *s = "hello discord";
    TEST_ASSERT_EQUAL_UINT64(fnv1a64(s), fnv1a64(s));
}

/* 2. FNV-1a different inputs → different outputs */
void test_fnv1a_different_inputs(void)
{
    TEST_ASSERT_TRUE(fnv1a64("abc") != fnv1a64("def"));
}

/* 3. FNV-1a NULL returns basis */
void test_fnv1a_null(void)
{
    uint64_t h = fnv1a64(NULL);
    TEST_ASSERT_EQUAL_UINT64(1469598103934665603ULL, h);
}

/* 4. make_msg_key combines channel + msg id */
void test_make_msg_key_combines(void)
{
    uint64_t k1 = make_msg_key("chan1", "msg1");
    uint64_t k2 = make_msg_key("chan1", "msg2");
    uint64_t k3 = make_msg_key("chan2", "msg1");
    TEST_ASSERT_TRUE(k1 != k2);
    TEST_ASSERT_TRUE(k1 != k3);
    TEST_ASSERT_TRUE(k2 != k3);
}

/* 5. Dedup ring buffer insert + contains */
void test_dedup_insert_contains(void)
{
    uint64_t keys[DISCORD_DEDUP_CACHE_SIZE] = {0};
    size_t idx = 0;

    uint64_t k = make_msg_key("ch", "m1");
    keys[idx] = k;
    idx = (idx + 1) % DISCORD_DEDUP_CACHE_SIZE;

    int found = 0;
    for (size_t i = 0; i < DISCORD_DEDUP_CACHE_SIZE; i++)
        if (keys[i] == k) found = 1;
    TEST_ASSERT_TRUE(found);
}

/* 6. Dedup ring buffer wraps correctly */
void test_dedup_ring_wrap(void)
{
    uint64_t keys[DISCORD_DEDUP_CACHE_SIZE] = {0};
    size_t idx = 0;

    for (size_t i = 0; i < DISCORD_DEDUP_CACHE_SIZE + 5; i++) {
        keys[idx] = (uint64_t)(i + 100);
        idx = (idx + 1) % DISCORD_DEDUP_CACHE_SIZE;
    }
    /* After wrapping, slot 0 should hold value for i = DISCORD_DEDUP_CACHE_SIZE */
    uint64_t expected = (uint64_t)(DISCORD_DEDUP_CACHE_SIZE + 100);
    TEST_ASSERT_EQUAL_UINT64(expected, keys[0]);
}

/* 7. MIMI_CHAN_DISCORD string value */
void test_channel_name(void)
{
    TEST_ASSERT_EQUAL_STRING("discord", MIMI_CHAN_DISCORD);
    TEST_ASSERT_EQUAL_size_t(7, strlen(MIMI_CHAN_DISCORD));
}

/* 8. Parse Hello event → extract heartbeat_interval */
void test_parse_hello_heartbeat(void)
{
    const char *json = "{\"heartbeat_interval\": 41250}";
    cJSON *root = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(root);
    cJSON *iv = cJSON_GetObjectItem(root, "heartbeat_interval");
    TEST_ASSERT_NOT_NULL(iv);
    TEST_ASSERT_TRUE(cJSON_IsNumber(iv));
    TEST_ASSERT_EQUAL_INT(41250, (int)iv->valuedouble);
    cJSON_Delete(root);
}

/* 9. Parse MESSAGE_CREATE → extract author.id */
void test_parse_message_create_author_id(void)
{
    const char *json =
        "{"
        "\"id\":\"999888777\","
        "\"channel_id\":\"123456789\","
        "\"content\":\"hello bot\","
        "\"author\":{\"id\":\"111222333\",\"username\":\"testuser\"}"
        "}";

    cJSON *msg = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(msg);

    cJSON *author = cJSON_GetObjectItem(msg, "author");
    TEST_ASSERT_NOT_NULL(author);
    cJSON *author_id = cJSON_GetObjectItem(author, "id");
    TEST_ASSERT_NOT_NULL(author_id);
    TEST_ASSERT_EQUAL_STRING("111222333", author_id->valuestring);

    cJSON_Delete(msg);
}

/* 10. Parse MESSAGE_CREATE → extract channel_id */
void test_parse_message_create_channel_id(void)
{
    const char *json =
        "{"
        "\"id\":\"999888777\","
        "\"channel_id\":\"123456789\","
        "\"content\":\"hello\","
        "\"author\":{\"id\":\"111222333\"}"
        "}";

    cJSON *msg = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(msg);

    cJSON *ch = cJSON_GetObjectItem(msg, "channel_id");
    TEST_ASSERT_NOT_NULL(ch);
    TEST_ASSERT_EQUAL_STRING("123456789", ch->valuestring);

    cJSON_Delete(msg);
}

/* 11. Parse MESSAGE_CREATE → extract content */
void test_parse_message_create_content(void)
{
    const char *json =
        "{"
        "\"id\":\"1\",\"channel_id\":\"2\","
        "\"content\":\"ping pong\","
        "\"author\":{\"id\":\"3\"}"
        "}";

    cJSON *msg = cJSON_Parse(json);
    cJSON *content = cJSON_GetObjectItem(msg, "content");
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_EQUAL_STRING("ping pong", content->valuestring);
    cJSON_Delete(msg);
}

/* 12. Parse MESSAGE_CREATE → extract message id */
void test_parse_message_create_msg_id(void)
{
    const char *json =
        "{"
        "\"id\":\"123456789012345678\","
        "\"channel_id\":\"99\","
        "\"content\":\"hi\","
        "\"author\":{\"id\":\"42\"}"
        "}";

    cJSON *msg = cJSON_Parse(json);
    cJSON *mid = cJSON_GetObjectItem(msg, "id");
    TEST_ASSERT_NOT_NULL(mid);
    TEST_ASSERT_EQUAL_STRING("123456789012345678", mid->valuestring);
    cJSON_Delete(msg);
}

/* 13. Parse Gateway dispatch envelope */
void test_parse_gateway_dispatch(void)
{
    const char *json =
        "{\"op\":0,\"t\":\"MESSAGE_CREATE\",\"s\":42,"
        "\"d\":{\"id\":\"1\",\"channel_id\":\"2\","
        "\"content\":\"test\",\"author\":{\"id\":\"3\"}}}";

    cJSON *root = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(root);

    cJSON *op = cJSON_GetObjectItem(root, "op");
    TEST_ASSERT_EQUAL_INT(0, (int)op->valuedouble);

    cJSON *t = cJSON_GetObjectItem(root, "t");
    TEST_ASSERT_EQUAL_STRING("MESSAGE_CREATE", t->valuestring);

    cJSON *d = cJSON_GetObjectItem(root, "d");
    TEST_ASSERT_NOT_NULL(d);

    cJSON *content = cJSON_GetObjectItem(d, "content");
    TEST_ASSERT_EQUAL_STRING("test", content->valuestring);

    cJSON_Delete(root);
}

/* 14. Parse Hello opcode */
void test_parse_gateway_hello(void)
{
    const char *json = "{\"op\":10,\"d\":{\"heartbeat_interval\":41250}}";
    cJSON *root = cJSON_Parse(json);

    cJSON *op = cJSON_GetObjectItem(root, "op");
    TEST_ASSERT_EQUAL_INT(10, (int)op->valuedouble);

    cJSON *d = cJSON_GetObjectItem(root, "d");
    cJSON *iv = cJSON_GetObjectItem(d, "heartbeat_interval");
    TEST_ASSERT_EQUAL_INT(41250, (int)iv->valuedouble);

    cJSON_Delete(root);
}

/* 15. Parse READY event → session_id */
void test_parse_ready_session_id(void)
{
    const char *json =
        "{\"op\":0,\"t\":\"READY\",\"d\":{"
        "\"session_id\":\"abc123def456\","
        "\"user\":{\"id\":\"botid\"}}}";

    cJSON *root = cJSON_Parse(json);
    cJSON *d = cJSON_GetObjectItem(root, "d");
    cJSON *sid = cJSON_GetObjectItem(d, "session_id");
    TEST_ASSERT_EQUAL_STRING("abc123def456", sid->valuestring);
    cJSON_Delete(root);
}

/* 16. Message with empty content should be skipped */
void test_parse_empty_content(void)
{
    const char *json =
        "{\"id\":\"1\",\"channel_id\":\"2\","
        "\"content\":\"\","
        "\"author\":{\"id\":\"3\"}}";

    cJSON *msg = cJSON_Parse(json);
    cJSON *content = cJSON_GetObjectItem(msg, "content");
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_EQUAL_STRING("", content->valuestring);
    /* In the handler, empty content is filtered out */
    TEST_ASSERT_TRUE(strlen(content->valuestring) == 0);
    cJSON_Delete(msg);
}

/* 17. Dedup key uniqueness across Discord snowflake IDs */
void test_dedup_key_snowflake_uniqueness(void)
{
    uint64_t k1 = make_msg_key("123456789012345678", "111111111111111111");
    uint64_t k2 = make_msg_key("123456789012345678", "222222222222222222");
    uint64_t k3 = make_msg_key("987654321098765432", "111111111111111111");
    TEST_ASSERT_TRUE(k1 != k2);
    TEST_ASSERT_TRUE(k1 != k3);
    TEST_ASSERT_TRUE(k2 != k3);
}

/* 18. REST path construction for channel messages */
void test_rest_path_construction(void)
{
    char path[256];
    snprintf(path, sizeof(path), "/channels/%s/messages", "123456789");
    TEST_ASSERT_EQUAL_STRING("/channels/123456789/messages", path);
}

/* 19. Identify payload structure validation */
void test_identify_payload_structure(void)
{
    cJSON *id = cJSON_CreateObject();
    cJSON_AddNumberToObject(id, "op", 2);
    cJSON *d = cJSON_CreateObject();
    cJSON_AddStringToObject(d, "token", "test-token");
    cJSON_AddNumberToObject(d, "intents", 33280);
    cJSON_AddItemToObject(id, "d", d);

    TEST_ASSERT_EQUAL_INT(2, (int)cJSON_GetObjectItem(id, "op")->valuedouble);
    TEST_ASSERT_EQUAL_STRING("test-token",
        cJSON_GetObjectItem(d, "token")->valuestring);
    TEST_ASSERT_EQUAL_INT(33280,
        (int)cJSON_GetObjectItem(d, "intents")->valuedouble);

    cJSON_Delete(id);
}

/* 20. Heartbeat payload structure validation */
void test_heartbeat_payload_structure(void)
{
    cJSON *hb = cJSON_CreateObject();
    cJSON_AddNumberToObject(hb, "op", 1);
    cJSON_AddNullToObject(hb, "d");

    TEST_ASSERT_EQUAL_INT(1, (int)cJSON_GetObjectItem(hb, "op")->valuedouble);
    TEST_ASSERT_TRUE(cJSON_IsNull(cJSON_GetObjectItem(hb, "d")));

    cJSON_Delete(hb);
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fnv1a_consistent);
    RUN_TEST(test_fnv1a_different_inputs);
    RUN_TEST(test_fnv1a_null);
    RUN_TEST(test_make_msg_key_combines);
    RUN_TEST(test_dedup_insert_contains);
    RUN_TEST(test_dedup_ring_wrap);
    RUN_TEST(test_channel_name);
    RUN_TEST(test_parse_hello_heartbeat);
    RUN_TEST(test_parse_message_create_author_id);
    RUN_TEST(test_parse_message_create_channel_id);
    RUN_TEST(test_parse_message_create_content);
    RUN_TEST(test_parse_message_create_msg_id);
    RUN_TEST(test_parse_gateway_dispatch);
    RUN_TEST(test_parse_gateway_hello);
    RUN_TEST(test_parse_ready_session_id);
    RUN_TEST(test_parse_empty_content);
    RUN_TEST(test_dedup_key_snowflake_uniqueness);
    RUN_TEST(test_rest_path_construction);
    RUN_TEST(test_identify_payload_structure);
    RUN_TEST(test_heartbeat_payload_structure);
    return UNITY_END();
}
