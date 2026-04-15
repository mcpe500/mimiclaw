#include "unity.h"
#include "tools/tool_registry.h"
#include <string.h>
#include <stdlib.h>

void setUp(void) {}
void tearDown(void) {}

static esp_err_t dummy_tool_exec(const char *input_json, char *output, size_t output_size)
{
    (void)input_json;
    strncpy(output, "dummy_result", output_size);
    return ESP_OK;
}

void test_tool_register_external_exists(void)
{
    mimi_tool_t tool = {
        .name = "test_ext",
        .description = "External test tool",
        .input_schema_json = "{}",
        .execute = dummy_tool_exec,
    };
    esp_err_t ret = tool_register_external(&tool);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    tool_unregister("test_ext");
}

void test_register_beyond_builtin(void)
{
    mimi_tool_t tool = {
        .name = "ext_tool_1",
        .description = "Extension tool 1",
        .input_schema_json = "{}",
        .execute = dummy_tool_exec,
    };
    esp_err_t ret = tool_register_external(&tool);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    tool_unregister("ext_tool_1");
}

void test_max_sixteen_tools(void)
{
    mimi_tool_t tools[16];
    for (int i = 0; i < 16; i++) {
        char name[32];
        snprintf(name, sizeof(name), "max_tool_%d", i);
        tools[i].name = strdup(name);
        tools[i].description = "max test";
        tools[i].input_schema_json = "{}";
        tools[i].execute = dummy_tool_exec;
        esp_err_t ret = tool_register_external(&tools[i]);
        TEST_ASSERT_EQUAL(ESP_OK, ret);
    }

    mimi_tool_t overflow = {
        .name = "overflow_tool",
        .description = "should fail",
        .input_schema_json = "{}",
        .execute = dummy_tool_exec,
    };
    esp_err_t ret = tool_register_external(&overflow);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);

    for (int i = 0; i < 16; i++) {
        char name[32];
        snprintf(name, sizeof(name), "max_tool_%d", i);
        tool_unregister(name);
        free((void *)tools[i].name);
    }
}

void test_lookup_external_tool(void)
{
    mimi_tool_t tool = {
        .name = "lookup_test",
        .description = "Lookup test tool",
        .input_schema_json = "{}",
        .execute = dummy_tool_exec,
    };
    tool_register_external(&tool);

    char output[256] = {0};
    esp_err_t ret = tool_registry_execute("lookup_test", "{}", output, sizeof(output));
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING("dummy_result", output);

    tool_unregister("lookup_test");
}

void test_unregister_removes_tool(void)
{
    mimi_tool_t tool = {
        .name = "temp_tool",
        .description = "Temporary",
        .input_schema_json = "{}",
        .execute = dummy_tool_exec,
    };
    tool_register_external(&tool);
    tool_unregister("temp_tool");

    char output[256] = {0};
    esp_err_t ret = tool_registry_execute("temp_tool", "{}", output, sizeof(output));
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, ret);
}

void test_reregister_after_unregister(void)
{
    mimi_tool_t tool = {
        .name = "rereg_tool",
        .description = "Rereg test",
        .input_schema_json = "{}",
        .execute = dummy_tool_exec,
    };
    tool_register_external(&tool);
    tool_unregister("rereg_tool");

    mimi_tool_t tool2 = {
        .name = "rereg_tool",
        .description = "Rereg test v2",
        .input_schema_json = "{}",
        .execute = dummy_tool_exec,
    };
    esp_err_t ret = tool_register_external(&tool2);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    char output[256] = {0};
    ret = tool_registry_execute("rereg_tool", "{}", output, sizeof(output));
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    tool_unregister("rereg_tool");
}

void test_unregister_unknown_fails(void)
{
    esp_err_t ret = tool_unregister("nonexistent_tool");
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
}

void test_register_null_tool_fails(void)
{
    esp_err_t ret = tool_register_external(NULL);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
}

void test_register_null_name_fails(void)
{
    mimi_tool_t tool = {0};
    esp_err_t ret = tool_register_external(&tool);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
}

void test_register_duplicate_name_fails(void)
{
    mimi_tool_t tool = {
        .name = "dup_tool",
        .description = "dup",
        .input_schema_json = "{}",
        .execute = dummy_tool_exec,
    };
    tool_register_external(&tool);
    esp_err_t ret = tool_register_external(&tool);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
    tool_unregister("dup_tool");
}

void test_external_tool_execute_output(void)
{
    mimi_tool_t tool = {
        .name = "output_test",
        .description = "output test",
        .input_schema_json = "{}",
        .execute = dummy_tool_exec,
    };
    tool_register_external(&tool);

    char output[256] = {0};
    tool_registry_execute("output_test", "{\"key\":\"val\"}", output, sizeof(output));
    TEST_ASSERT_EQUAL_STRING("dummy_result", output);

    tool_unregister("output_test");
}

void test_external_tool_with_large_input(void)
{
    mimi_tool_t tool = {
        .name = "large_input",
        .description = "large input test",
        .input_schema_json = "{}",
        .execute = dummy_tool_exec,
    };
    tool_register_external(&tool);

    char big_input[2048];
    memset(big_input, 'a', sizeof(big_input) - 1);
    big_input[sizeof(big_input) - 1] = '\0';

    char output[256] = {0};
    esp_err_t ret = tool_registry_execute("large_input", big_input, output, sizeof(output));
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    tool_unregister("large_input");
}

void test_registry_init_ok(void)
{
    esp_err_t ret = tool_registry_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

void test_builtin_tools_registered(void)
{
    tool_registry_init();
    const char *json = tool_registry_get_tools_json();
    TEST_ASSERT_NOT_NULL(json);
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_tool_register_external_exists);
    RUN_TEST(test_register_beyond_builtin);
    RUN_TEST(test_max_sixteen_tools);
    RUN_TEST(test_lookup_external_tool);
    RUN_TEST(test_unregister_removes_tool);
    RUN_TEST(test_reregister_after_unregister);
    RUN_TEST(test_unregister_unknown_fails);
    RUN_TEST(test_register_null_tool_fails);
    RUN_TEST(test_register_null_name_fails);
    RUN_TEST(test_register_duplicate_name_fails);
    RUN_TEST(test_external_tool_execute_output);
    RUN_TEST(test_external_tool_with_large_input);
    RUN_TEST(test_registry_init_ok);
    RUN_TEST(test_builtin_tools_registered);
    RUN_TEST(test_max_sixteen_tools);
    return UNITY_END();
}
