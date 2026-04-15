#include "unity.h"
#include "cJSON.h"
#include "esp_err.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define MIMI_CHAN_WHATSAPP  "whatsapp"

#define WA_DEDUP_CACHE_SIZE 64

static char s_seen_ids[WA_DEDUP_CACHE_SIZE][64];
static size_t s_seen_idx = 0;

static void dedup_reset(void)
{
    memset(s_seen_ids, 0, sizeof(s_seen_ids));
    s_seen_idx = 0;
}

static bool dedup_contains(const char *msg_id)
{
    for (size_t i = 0; i < WA_DEDUP_CACHE_SIZE; i++) {
        if (s_seen_ids[i][0] != '\0' && strcmp(s_seen_ids[i], msg_id) == 0) {
            return true;
        }
    }
    return false;
}

static void dedup_insert(const char *msg_id)
{
    strncpy(s_seen_ids[s_seen_idx], msg_id, sizeof(s_seen_ids[s_seen_idx]) - 1);
    s_seen_ids[s_seen_idx][sizeof(s_seen_ids[s_seen_idx]) - 1] = '\0';
    s_seen_idx = (s_seen_idx + 1) % WA_DEDUP_CACHE_SIZE;
}

typedef struct {
    char channel[16];
    char chat_id[96];
    char *content;
    char user_id[64];
    char message_id[64];
} test_msg_t;

static test_msg_t g_last_msg;
static int g_push_count;

static esp_err_t fake_bus_push(const test_msg_t *msg)
{
    if (g_push_count > 0) free(g_last_msg.content);
    memset(&g_last_msg, 0, sizeof(g_last_msg));
    strncpy(g_last_msg.channel, msg->channel, sizeof(g_last_msg.channel) - 1);
    strncpy(g_last_msg.chat_id, msg->chat_id, sizeof(g_last_msg.chat_id) - 1);
    strncpy(g_last_msg.message_id, msg->message_id, sizeof(g_last_msg.message_id) - 1);
    g_last_msg.content = msg->content ? strdup(msg->content) : NULL;
    g_push_count++;
    return ESP_OK;
}

static void parse_wa_messages(const char *json_str)
{
    cJSON *root = cJSON_Parse(json_str);
    if (!root) return;

    cJSON *entry_arr = cJSON_GetObjectItem(root, "entry");
    if (!cJSON_IsArray(entry_arr)) { cJSON_Delete(root); return; }

    cJSON *entry;
    cJSON_ArrayForEach(entry, entry_arr) {
        cJSON *changes = cJSON_GetObjectItem(entry, "changes");
        if (!cJSON_IsArray(changes)) continue;

        cJSON *change;
        cJSON_ArrayForEach(change, changes) {
            cJSON *value = cJSON_GetObjectItem(change, "value");
            if (!value) continue;

            cJSON *messages = cJSON_GetObjectItem(value, "messages");
            if (!cJSON_IsArray(messages)) continue;

            cJSON *msg;
            cJSON_ArrayForEach(msg, messages) {
                cJSON *id_obj = cJSON_GetObjectItem(msg, "id");
                if (!id_obj || !cJSON_IsString(id_obj)) continue;
                const char *msg_id = id_obj->valuestring;

                if (dedup_contains(msg_id)) continue;
                dedup_insert(msg_id);

                cJSON *from_obj = cJSON_GetObjectItem(msg, "from");
                cJSON *type_obj = cJSON_GetObjectItem(msg, "type");
                if (!from_obj || !cJSON_IsString(from_obj)) continue;
                if (!type_obj || !cJSON_IsString(type_obj)) continue;

                const char *text = NULL;
                if (strcmp(type_obj->valuestring, "text") == 0) {
                    cJSON *text_obj = cJSON_GetObjectItem(msg, "text");
                    if (text_obj) {
                        cJSON *body = cJSON_GetObjectItem(text_obj, "body");
                        if (body && cJSON_IsString(body)) {
                            text = body->valuestring;
                        }
                    }
                }
                if (!text) continue;

                test_msg_t out = {0};
                strncpy(out.channel, MIMI_CHAN_WHATSAPP, sizeof(out.channel) - 1);
                strncpy(out.chat_id, from_obj->valuestring, sizeof(out.chat_id) - 1);
                strncpy(out.message_id, msg_id, sizeof(out.message_id) - 1);
                out.content = strdup(text);
                fake_bus_push(&out);
                free(out.content);
            }
        }
    }
    cJSON_Delete(root);
}

static char *build_send_payload(const char *phone, const char *text)
{
    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "messaging_product", "whatsapp");
    cJSON_AddStringToObject(body, "to", phone);
    cJSON_AddStringToObject(body, "type", "text");

    cJSON *text_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(text_obj, "body", text);
    cJSON_AddItemToObject(body, "text", text_obj);

    char *json_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    return json_str;
}

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

void setUp(void)
{
    dedup_reset();
    g_push_count = 0;
    memset(&g_last_msg, 0, sizeof(g_last_msg));
}

void tearDown(void) {}

/* Test 1: Channel identifier constant */
void test_wa_channel_constant(void)
{
    TEST_ASSERT_EQUAL_STRING("whatsapp", MIMI_CHAN_WHATSAPP);
    TEST_ASSERT_EQUAL(8, strlen(MIMI_CHAN_WHATSAPP));
}

/* Test 2: Dedup ring buffer - no duplicates initially */
void test_wa_dedup_empty(void)
{
    TEST_ASSERT_FALSE(dedup_contains("wamid_001"));
    TEST_ASSERT_FALSE(dedup_contains(""));
}

/* Test 3: Dedup insert and detect */
void test_wa_dedup_insert_detect(void)
{
    dedup_insert("wamid_001");
    TEST_ASSERT_TRUE(dedup_contains("wamid_001"));
    TEST_ASSERT_FALSE(dedup_contains("wamid_002"));
}

/* Test 4: Dedup ring buffer wrap */
void test_wa_dedup_ring_wrap(void)
{
    for (int i = 0; i < WA_DEDUP_CACHE_SIZE + 5; i++) {
        char id[32];
        snprintf(id, sizeof(id), "wamid_%03d", i);
        dedup_insert(id);
    }
    TEST_ASSERT_TRUE(dedup_contains("wamid_005"));
    TEST_ASSERT_FALSE(dedup_contains("wamid_000"));
}

/* Test 5: Parse simple incoming message */
void test_wa_parse_simple_message(void)
{
    const char *json = "{"
        "\"entry\":[{"
            "\"id\":\"1234\","
            "\"changes\":[{"
                "\"value\":{"
                    "\"messages\":[{"
                        "\"id\":\"wamid_abc123\","
                        "\"from\":\"15551234567\","
                        "\"type\":\"text\","
                        "\"text\":{\"body\":\"Hello MimiClaw\"}"
                    "}]"
                "}"
            "}]"
        "}]"
    "}";

    parse_wa_messages(json);
    TEST_ASSERT_EQUAL(1, g_push_count);
    TEST_ASSERT_EQUAL_STRING("whatsapp", g_last_msg.channel);
    TEST_ASSERT_EQUAL_STRING("15551234567", g_last_msg.chat_id);
    TEST_ASSERT_EQUAL_STRING("wamid_abc123", g_last_msg.message_id);
    TEST_ASSERT_NOT_NULL(g_last_msg.content);
    TEST_ASSERT_EQUAL_STRING("Hello MimiClaw", g_last_msg.content);
}

/* Test 6: Parse multiple entries and changes */
void test_wa_parse_multiple_entries(void)
{
    const char *json = "{"
        "\"entry\":["
            "{"
                "\"changes\":[{"
                    "\"value\":{"
                        "\"messages\":[{"
                            "\"id\":\"wamid_001\","
                            "\"from\":\"11111111111\","
                            "\"type\":\"text\","
                            "\"text\":{\"body\":\"msg1\"}"
                        "}]"
                    "}"
                "}]"
            "},"
            "{"
                "\"changes\":[{"
                    "\"value\":{"
                        "\"messages\":[{"
                            "\"id\":\"wamid_002\","
                            "\"from\":\"22222222222\","
                            "\"type\":\"text\","
                            "\"text\":{\"body\":\"msg2\"}"
                        "}]"
                    "}"
                "}]"
            "}"
        "]"
    "}";

    parse_wa_messages(json);
    TEST_ASSERT_EQUAL(2, g_push_count);
}

/* Test 7: Parse skips duplicate message IDs */
void test_wa_parse_dedup_skip(void)
{
    const char *json = "{"
        "\"entry\":[{"
            "\"changes\":[{"
                "\"value\":{"
                    "\"messages\":["
                        "{\"id\":\"wamid_dup\",\"from\":\"111\",\"type\":\"text\",\"text\":{\"body\":\"first\"}},"
                        "{\"id\":\"wamid_dup\",\"from\":\"111\",\"type\":\"text\",\"text\":{\"body\":\"dup\"}}"
                    "]"
                "}"
            "}]"
        "}]"
    "}";

    parse_wa_messages(json);
    TEST_ASSERT_EQUAL(1, g_push_count);
    TEST_ASSERT_EQUAL_STRING("first", g_last_msg.content);
}

/* Test 8: Parse skips non-text messages */
void test_wa_parse_skips_non_text(void)
{
    const char *json = "{"
        "\"entry\":[{"
            "\"changes\":[{"
                "\"value\":{"
                    "\"messages\":[{"
                        "\"id\":\"wamid_img\","
                        "\"from\":\"15551234567\","
                        "\"type\":\"image\","
                        "\"image\":{\"id\":\"media_123\"}"
                    "}]"
                "}"
            "}]"
        "}]"
    "}";

    parse_wa_messages(json);
    TEST_ASSERT_EQUAL(0, g_push_count);
}

/* Test 9: Parse handles empty entry array */
void test_wa_parse_empty_entry(void)
{
    const char *json = "{\"entry\":[]}";
    parse_wa_messages(json);
    TEST_ASSERT_EQUAL(0, g_push_count);
}

/* Test 10: Parse handles invalid JSON */
void test_wa_parse_invalid_json(void)
{
    parse_wa_messages("not json at all");
    TEST_ASSERT_EQUAL(0, g_push_count);

    parse_wa_messages(NULL);
    TEST_ASSERT_EQUAL(0, g_push_count);
}

/* Test 11: Send payload JSON structure */
void test_wa_send_payload_structure(void)
{
    char *payload = build_send_payload("15551234567", "Hello from MimiClaw");
    TEST_ASSERT_NOT_NULL(payload);

    cJSON *root = cJSON_Parse(payload);
    TEST_ASSERT_NOT_NULL(root);

    cJSON *mp = cJSON_GetObjectItem(root, "messaging_product");
    TEST_ASSERT_TRUE(cJSON_IsString(mp));
    TEST_ASSERT_EQUAL_STRING("whatsapp", mp->valuestring);

    cJSON *to = cJSON_GetObjectItem(root, "to");
    TEST_ASSERT_TRUE(cJSON_IsString(to));
    TEST_ASSERT_EQUAL_STRING("15551234567", to->valuestring);

    cJSON *type = cJSON_GetObjectItem(root, "type");
    TEST_ASSERT_TRUE(cJSON_IsString(type));
    TEST_ASSERT_EQUAL_STRING("text", type->valuestring);

    cJSON *text = cJSON_GetObjectItem(root, "text");
    TEST_ASSERT_NOT_NULL(text);
    cJSON *body = cJSON_GetObjectItem(text, "body");
    TEST_ASSERT_TRUE(cJSON_IsString(body));
    TEST_ASSERT_EQUAL_STRING("Hello from MimiClaw", body->valuestring);

    cJSON_Delete(root);
    free(payload);
}

/* Test 12: Send payload contains all required fields */
void test_wa_send_payload_field_count(void)
{
    char *payload = build_send_payload("12345", "test");
    TEST_ASSERT_NOT_NULL(payload);

    cJSON *root = cJSON_Parse(payload);
    TEST_ASSERT_NOT_NULL(root);

    int count = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, root) { count++; }
    TEST_ASSERT_EQUAL(4, count);

    cJSON_Delete(root);
    free(payload);
}

/* Test 13: Phone number extraction from "from" field */
void test_wa_phone_number_extraction(void)
{
    const char *json = "{"
        "\"entry\":[{"
            "\"changes\":[{"
                "\"value\":{"
                    "\"messages\":[{"
                        "\"id\":\"wamid_ph1\","
                        "\"from\":\"+15551234567\","
                        "\"type\":\"text\","
                        "\"text\":{\"body\":\"test\"}"
                    "}]"
                "}"
            "}]"
        "}]"
    "}";

    parse_wa_messages(json);
    TEST_ASSERT_EQUAL_STRING("+15551234567", g_last_msg.chat_id);
}

/* Test 14: FNV-1a hash consistency */
void test_wa_fnv1a_consistency(void)
{
    uint64_t h1 = fnv1a64("wamid_abc123");
    uint64_t h2 = fnv1a64("wamid_abc123");
    TEST_ASSERT_EQUAL(h1, h2);

    uint64_t h3 = fnv1a64("wamid_different");
    TEST_ASSERT_TRUE(h1 != h3);
}

/* Test 15: Credential struct validation */
void test_wa_credential_fields(void)
{
    struct {
        char access_token[256];
        char phone_number_id[64];
        char verify_token[128];
    } creds = {0};

    strncpy(creds.access_token, "EAA_test_token_123", sizeof(creds.access_token) - 1);
    strncpy(creds.phone_number_id, "1234567890", sizeof(creds.phone_number_id) - 1);
    strncpy(creds.verify_token, "my_verify_token", sizeof(creds.verify_token) - 1);

    TEST_ASSERT_EQUAL_STRING("EAA_test_token_123", creds.access_token);
    TEST_ASSERT_EQUAL_STRING("1234567890", creds.phone_number_id);
    TEST_ASSERT_EQUAL_STRING("my_verify_token", creds.verify_token);
    TEST_ASSERT_EQUAL(256, sizeof(creds.access_token));
    TEST_ASSERT_EQUAL(64, sizeof(creds.phone_number_id));
    TEST_ASSERT_EQUAL(128, sizeof(creds.verify_token));
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_wa_channel_constant);
    RUN_TEST(test_wa_dedup_empty);
    RUN_TEST(test_wa_dedup_insert_detect);
    RUN_TEST(test_wa_dedup_ring_wrap);
    RUN_TEST(test_wa_parse_simple_message);
    RUN_TEST(test_wa_parse_multiple_entries);
    RUN_TEST(test_wa_parse_dedup_skip);
    RUN_TEST(test_wa_parse_skips_non_text);
    RUN_TEST(test_wa_parse_empty_entry);
    RUN_TEST(test_wa_parse_invalid_json);
    RUN_TEST(test_wa_send_payload_structure);
    RUN_TEST(test_wa_send_payload_field_count);
    RUN_TEST(test_wa_phone_number_extraction);
    RUN_TEST(test_wa_fnv1a_consistency);
    RUN_TEST(test_wa_credential_fields);
    return UNITY_END();
}
