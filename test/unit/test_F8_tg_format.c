#include "unity.h"
#include "channels/telegram/tg_format.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_tg_format_html_null(void)
{
    char *result = tg_format_html(NULL);
    TEST_ASSERT_NULL(result);
}

void test_tg_format_html_basic(void)
{
    char *result = tg_format_html("hello");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "hello") != NULL);
    tg_format_free(result);
}

void test_tg_format_html_escapes_amp(void)
{
    char *result = tg_format_html("a & b");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "&amp;") != NULL);
    tg_format_free(result);
}

void test_tg_format_html_escapes_brackets(void)
{
    char *result = tg_format_html("<test>");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "&lt;") != NULL);
    TEST_ASSERT_TRUE(strstr(result, "&gt;") != NULL);
    tg_format_free(result);
}

void test_tg_format_markdownv2_null(void)
{
    char *result = tg_format_markdownv2(NULL);
    TEST_ASSERT_NULL(result);
}

void test_tg_format_markdownv2_basic(void)
{
    char *result = tg_format_markdownv2("hello");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "hello") != NULL);
    tg_format_free(result);
}

void test_tg_format_markdownv2_escapes_brackets(void)
{
    char *result = tg_format_markdownv2("[test]");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "\\[") != NULL);
    tg_format_free(result);
}

void test_tg_format_free_null_safe(void)
{
    tg_format_free(NULL);
    TEST_ASSERT_TRUE(true);
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_tg_format_html_null);
    RUN_TEST(test_tg_format_html_basic);
    RUN_TEST(test_tg_format_html_escapes_amp);
    RUN_TEST(test_tg_format_html_escapes_brackets);
    RUN_TEST(test_tg_format_markdownv2_null);
    RUN_TEST(test_tg_format_markdownv2_basic);
    RUN_TEST(test_tg_format_markdownv2_escapes_brackets);
    RUN_TEST(test_tg_format_free_null_safe);
    return UNITY_END();
}