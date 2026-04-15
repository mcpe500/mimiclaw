#include "unity.h"
#include "skills/skill_manifest.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_parse_valid_manifest(void)
{
    const char *json = "{"
        "\"name\":\"translator\","
        "\"version\":\"1.0.0\","
        "\"description\":\"Translates text\","
        "\"author\":\"MimiClaw\""
        "}";
    skill_manifest_t m = {0};
    esp_err_t ret = skill_manifest_parse(json, &m);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING("translator", m.name);
    TEST_ASSERT_EQUAL_STRING("1.0.0", m.version);
    TEST_ASSERT_EQUAL_STRING("Translates text", m.description);
    TEST_ASSERT_EQUAL_STRING("MimiClaw", m.author);
}

void test_parse_missing_fields(void)
{
    const char *json = "{\"description\":\"no name or version\"}";
    skill_manifest_t m = {0};
    esp_err_t ret = skill_manifest_parse(json, &m);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING("", m.name);
    TEST_ASSERT_EQUAL_STRING("", m.version);
}

void test_parse_empty_json(void)
{
    const char *json = "{}";
    skill_manifest_t m = {0};
    esp_err_t ret = skill_manifest_parse(json, &m);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING("", m.name);
}

void test_parse_tools_required(void)
{
    const char *json = "{"
        "\"name\":\"skill1\","
        "\"version\":\"2.0\","
        "\"tools_required\":[\"web_search\",\"gpio\"]"
        "}";
    skill_manifest_t m = {0};
    esp_err_t ret = skill_manifest_parse(json, &m);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(2, m.tool_count);
    TEST_ASSERT_EQUAL_STRING("web_search", m.tools_required[0]);
    TEST_ASSERT_EQUAL_STRING("gpio", m.tools_required[1]);
}

void test_parse_dependencies(void)
{
    const char *json = "{"
        "\"name\":\"skill2\","
        "\"version\":\"1.0\","
        "\"dependencies\":[\"base_skill\",\"math_lib\"]"
        "}";
    skill_manifest_t m = {0};
    esp_err_t ret = skill_manifest_parse(json, &m);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(2, m.dep_count);
    TEST_ASSERT_EQUAL_STRING("base_skill", m.dependencies[0]);
    TEST_ASSERT_EQUAL_STRING("math_lib", m.dependencies[1]);
}

void test_parse_version_string(void)
{
    const char *json = "{\"name\":\"v\",\"version\":\"3.2.1-beta\"}";
    skill_manifest_t m = {0};
    esp_err_t ret = skill_manifest_parse(json, &m);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING("3.2.1-beta", m.version);
}

void test_validate_missing_name_fails(void)
{
    skill_manifest_t m = {0};
    m.name[0] = '\0';
    strncpy(m.version, "1.0", sizeof(m.version) - 1);
    esp_err_t ret = skill_manifest_validate(&m);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
}

void test_validate_missing_version_fails(void)
{
    skill_manifest_t m = {0};
    strncpy(m.name, "test", sizeof(m.name) - 1);
    m.version[0] = '\0';
    esp_err_t ret = skill_manifest_validate(&m);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
}

void test_validate_valid_manifest(void)
{
    skill_manifest_t m = {0};
    strncpy(m.name, "valid_skill", sizeof(m.name) - 1);
    strncpy(m.version, "1.0.0", sizeof(m.version) - 1);
    esp_err_t ret = skill_manifest_validate(&m);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

void test_parse_null_json_fails(void)
{
    skill_manifest_t m = {0};
    esp_err_t ret = skill_manifest_parse(NULL, &m);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

void test_parse_null_manifest_fails(void)
{
    esp_err_t ret = skill_manifest_parse("{}", NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

void test_parse_invalid_json_fails(void)
{
    skill_manifest_t m = {0};
    esp_err_t ret = skill_manifest_parse("not json{{{", &m);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
}

void test_validate_null_fails(void)
{
    esp_err_t ret = skill_manifest_validate(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_valid_manifest);
    RUN_TEST(test_parse_missing_fields);
    RUN_TEST(test_parse_empty_json);
    RUN_TEST(test_parse_tools_required);
    RUN_TEST(test_parse_dependencies);
    RUN_TEST(test_parse_version_string);
    RUN_TEST(test_validate_missing_name_fails);
    RUN_TEST(test_validate_missing_version_fails);
    RUN_TEST(test_validate_valid_manifest);
    RUN_TEST(test_parse_null_json_fails);
    RUN_TEST(test_parse_null_manifest_fails);
    RUN_TEST(test_parse_invalid_json_fails);
    return UNITY_END();
}
