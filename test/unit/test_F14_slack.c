#include "unity.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define MIMI_CHAN_SLACK "slack"

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

typedef struct {
    char user[64];
    char channel[64];
    char text[512];
    bool valid;
} slack_msg_t;

static bool slack_parse_event(const char *json, slack_msg_t *out)
{
    memset(out, 0, sizeof(*out));
    cJSON *root = cJSON_Parse(json);
    if (!root) return false;

    cJSON *type = cJSON_GetObjectItem(root, "type");
    bool found = false;

    if (type && cJSON_IsString(type) && strcmp(type->valuestring, "events_api") == 0) {
        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        if (payload) {
            cJSON *event = cJSON_GetObjectItem(payload, "event");
            if (event) {
                cJSON *et = cJSON_GetObjectItem(event, "type");
                if (et && cJSON_IsString(et) && strcmp(et->valuestring, "message") == 0) {
                    cJSON *subtype = cJSON_GetObjectItem(event, "subtype");
                    if (subtype && cJSON_IsString(subtype)) {
                        cJSON_Delete(root);
                        return false;
                    }
                    cJSON *u = cJSON_GetObjectItem(event, "user");
                    cJSON *c = cJSON_GetObjectItem(event, "channel");
                    cJSON *t = cJSON_GetObjectItem(event, "text");
                    if (u && cJSON_IsString(u)) strncpy(out->user, u->valuestring, sizeof(out->user) - 1);
                    if (c && cJSON_IsString(c)) strncpy(out->channel, c->valuestring, sizeof(out->channel) - 1);
                    if (t && cJSON_IsString(t)) strncpy(out->text, t->valuestring, sizeof(out->text) - 1);
                    out->valid = true;
                    found = true;
                }
            }
        }
    }

    cJSON_Delete(root);
    return found;
}

static char *slack_build_ack(const char *envelope_id)
{
    cJSON *ack = cJSON_CreateObject();
    cJSON_AddStringToObject(ack, "type", "ack");
    cJSON_AddStringToObject(ack, "envelope_id", envelope_id);
    char *result = cJSON_PrintUnformatted(ack);
    cJSON_Delete(ack);
    return result;
}

static bool slack_is_disconnect(const char *json)
{
    cJSON *root = cJSON_Parse(json);
    if (!root) return false;
    cJSON *type = cJSON_GetObjectItem(root, "type");
    bool is_disconnect = type && cJSON_IsString(type) && strcmp(type->valuestring, "disconnect") == 0;
    cJSON_Delete(root);
    return is_disconnect;
}

static bool slack_is_hello(const char *json)
{
    cJSON *root = cJSON_Parse(json);
    if (!root) return false;
    cJSON *type = cJSON_GetObjectItem(root, "type");
    bool is_hello = type && cJSON_IsString(type) && strcmp(type->valuestring, "hello") == 0;
    cJSON_Delete(root);
    return is_hello;
}

void setUp(void) {}
void tearDown(void) {}

void test_slack_msg_struct_init(void)
{
    slack_msg_t msg = {0};
    TEST_ASSERT_EQUAL_STRING("", msg.user);
    TEST_ASSERT_EQUAL_STRING("", msg.channel);
    TEST_ASSERT_EQUAL_STRING("", msg.text);
    TEST_ASSERT_FALSE(msg.valid);
}

void test_slack_fnv_hash_deterministic(void)
{
    uint64_t h1 = fnv1a64("test123");
    uint64_t h2 = fnv1a64("test123");
    TEST_ASSERT_EQUAL(h1, h2);
    TEST_ASSERT_NOT_EQUAL(0, h1);
}

void test_slack_fnv_hash_different_inputs(void)
{
    uint64_t h1 = fnv1a64("hello");
    uint64_t h2 = fnv1a64("world");
    TEST_ASSERT_NOT_EQUAL(h1, h2);
}

void test_slack_parse_valid_message_event(void)
{
    const char *json = "{\"type\":\"events_api\",\"payload\":{"
                       "\"event\":{\"type\":\"message\","
                       "\"user\":\"U12345\",\"channel\":\"C67890\","
                       "\"text\":\"hello world\",\"ts\":\"1234567890.000001\"}}}";
    slack_msg_t msg = {0};
    bool ok = slack_parse_event(json, &msg);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(msg.valid);
    TEST_ASSERT_EQUAL_STRING("U12345", msg.user);
    TEST_ASSERT_EQUAL_STRING("C67890", msg.channel);
    TEST_ASSERT_EQUAL_STRING("hello world", msg.text);
}

void test_slack_parse_non_message_event(void)
{
    const char *json = "{\"type\":\"events_api\",\"payload\":{"
                       "\"event\":{\"type\":\"app_mention\","
                       "\"user\":\"U123\",\"channel\":\"C456\"}}}";
    slack_msg_t msg = {0};
    bool ok = slack_parse_event(json, &msg);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_FALSE(msg.valid);
}

void test_slack_parse_invalid_json(void)
{
    slack_msg_t msg = {0};
    bool ok = slack_parse_event("{not valid}", &msg);
    TEST_ASSERT_FALSE(ok);
}

void test_slack_parse_missing_user(void)
{
    const char *json = "{\"type\":\"events_api\",\"payload\":{"
                       "\"event\":{\"type\":\"message\","
                       "\"channel\":\"C67890\",\"text\":\"hello\"}}}";
    slack_msg_t msg = {0};
    bool ok = slack_parse_event(json, &msg);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("", msg.user);
    TEST_ASSERT_EQUAL_STRING("C67890", msg.channel);
}

void test_slack_parse_message_with_subtype_skipped(void)
{
    const char *json = "{\"type\":\"events_api\",\"payload\":{"
                       "\"event\":{\"type\":\"message\",\"subtype\":\"bot_message\","
                       "\"user\":\"U123\",\"channel\":\"C456\",\"text\":\"bot msg\"}}}";
    slack_msg_t msg = {0};
    bool ok = slack_parse_event(json, &msg);
    TEST_ASSERT_FALSE(ok);
}

void test_slack_extract_channel_id(void)
{
    const char *json = "{\"type\":\"events_api\",\"payload\":{"
                       "\"event\":{\"type\":\"message\","
                       "\"user\":\"U111\",\"channel\":\"CABCDEF\","
                       "\"text\":\"test\",\"ts\":\"111.222\"}}}";
    slack_msg_t msg = {0};
    slack_parse_event(json, &msg);
    TEST_ASSERT_EQUAL_STRING("CABCDEF", msg.channel);
}

void test_slack_extract_user_id(void)
{
    const char *json = "{\"type\":\"events_api\",\"payload\":{"
                       "\"event\":{\"type\":\"message\","
                       "\"user\":\"UZYXWVU\",\"channel\":\"C111\","
                       "\"text\":\"test\",\"ts\":\"111.222\"}}}";
    slack_msg_t msg = {0};
    slack_parse_event(json, &msg);
    TEST_ASSERT_EQUAL_STRING("UZYXWVU", msg.user);
}

void test_slack_build_ack_response(void)
{
    char *ack = slack_build_ack("env_12345");
    TEST_ASSERT_NOT_NULL(ack);
    TEST_ASSERT_TRUE(strstr(ack, "\"type\":\"ack\"") != NULL ||
                     strstr(ack, "\"type\":\"ack\"") != NULL);
    TEST_ASSERT_TRUE(strstr(ack, "env_12345") != NULL);
    free(ack);
}

void test_slack_build_ack_contains_envelope_id(void)
{
    char *ack = slack_build_ack("abc-xyz-789");
    TEST_ASSERT_NOT_NULL(ack);
    cJSON *root = cJSON_Parse(ack);
    TEST_ASSERT_NOT_NULL(root);
    cJSON *eid = cJSON_GetObjectItem(root, "envelope_id");
    TEST_ASSERT_NOT_NULL(eid);
    TEST_ASSERT_EQUAL_STRING("abc-xyz-789", eid->valuestring);
    cJSON *t = cJSON_GetObjectItem(root, "type");
    TEST_ASSERT_EQUAL_STRING("ack", t->valuestring);
    cJSON_Delete(root);
    free(ack);
}

void test_slack_detect_disconnect_event(void)
{
    const char *json = "{\"type\":\"disconnect\",\"reason\":\"warning\"}";
    TEST_ASSERT_TRUE(slack_is_disconnect(json));
}

void test_slack_non_disconnect_event(void)
{
    const char *json = "{\"type\":\"events_api\"}";
    TEST_ASSERT_FALSE(slack_is_disconnect(json));
}

void test_slack_detect_hello_event(void)
{
    const char *json = "{\"type\":\"hello\"}";
    TEST_ASSERT_TRUE(slack_is_hello(json));
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_slack_msg_struct_init);
    RUN_TEST(test_slack_fnv_hash_deterministic);
    RUN_TEST(test_slack_fnv_hash_different_inputs);
    RUN_TEST(test_slack_parse_valid_message_event);
    RUN_TEST(test_slack_parse_non_message_event);
    RUN_TEST(test_slack_parse_invalid_json);
    RUN_TEST(test_slack_parse_missing_user);
    RUN_TEST(test_slack_parse_message_with_subtype_skipped);
    RUN_TEST(test_slack_extract_channel_id);
    RUN_TEST(test_slack_extract_user_id);
    RUN_TEST(test_slack_build_ack_response);
    RUN_TEST(test_slack_build_ack_contains_envelope_id);
    RUN_TEST(test_slack_detect_disconnect_event);
    RUN_TEST(test_slack_non_disconnect_event);
    RUN_TEST(test_slack_detect_hello_event);
    return UNITY_END();
}
