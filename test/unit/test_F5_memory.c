#include "unity.h"
#include "tools/tool_memory.h"
#include <string.h>
#include <stdlib.h>

void setUp(void) {}
void tearDown(void) {}

void test_tool_memory_init(void)
{
    esp_err_t err = tool_memory_init();
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_tool_memory_struct_exists(void)
{
    mimi_tool_t tool = {0};
    TEST_ASSERT_NOT_NULL(&tool);
}

void test_tool_memory_read_signature(void)
{
    char output[256];
    esp_err_t err = tool_memory_read(NULL, output, sizeof(output));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_tool_memory_write_invalid_input(void)
{
    char output[256];
    esp_err_t err = tool_memory_write(NULL, output, sizeof(output));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);

    err = tool_memory_write("{}", output, sizeof(output));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_tool_memory_append_invalid_input(void)
{
    char output[256];
    esp_err_t err = tool_memory_append(NULL, output, sizeof(output));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);

    err = tool_memory_append("{}", output, sizeof(output));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_tool_memory_recall_default_days(void)
{
    char output[4096];
    esp_err_t err = tool_memory_recall(NULL, output, sizeof(output));
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_tool_memory_recall_with_days(void)
{
    char output[4096];
    char input[64];
    snprintf(input, sizeof(input), "{\"days\":7}");
    esp_err_t err = tool_memory_recall(input, output, sizeof(output));
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_tool_memory_read_with_purpose(void)
{
    char output[4096];
    char input[64];
    snprintf(input, sizeof(input), "{\"purpose\":\"test\"}");
    esp_err_t err = tool_memory_read(input, output, sizeof(output));
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_tool_memory_init);
    RUN_TEST(test_tool_memory_struct_exists);
    RUN_TEST(test_tool_memory_read_signature);
    RUN_TEST(test_tool_memory_write_invalid_input);
    RUN_TEST(test_tool_memory_append_invalid_input);
    RUN_TEST(test_tool_memory_recall_default_days);
    RUN_TEST(test_tool_memory_recall_with_days);
    RUN_TEST(test_tool_memory_read_with_purpose);
    return UNITY_END();
}