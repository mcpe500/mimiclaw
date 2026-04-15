#include "unity.h"
#include "storage/db.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_db_init_memory(void)
{
    esp_err_t ret = db_init(":memory:");
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    db_close();
}

void test_db_init_null_fails(void)
{
    esp_err_t ret = db_init(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

void test_db_create_table(void)
{
    db_init(":memory:");
    esp_err_t ret = db_exec("CREATE TABLE test1 (id INTEGER PRIMARY KEY, name TEXT)");
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    db_close();
}

void test_db_insert(void)
{
    db_init(":memory:");
    db_exec("CREATE TABLE test2 (id INTEGER PRIMARY KEY, val TEXT)");
    esp_err_t ret = db_exec("INSERT INTO test2 (val) VALUES ('hello')");
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    db_close();
}

void test_db_select(void)
{
    db_init(":memory:");
    db_exec("CREATE TABLE test3 (id INTEGER PRIMARY KEY, val TEXT)");
    db_exec("INSERT INTO test3 (val) VALUES ('world')");

    char result[256] = {0};
    esp_err_t ret = db_query_one("SELECT val FROM test3 WHERE id=1", result, sizeof(result));
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING("world", result);
    db_close();
}

void test_db_close(void)
{
    db_init(":memory:");
    db_close();
    TEST_ASSERT_TRUE(1);
}

void test_db_schema_tables_created(void)
{
    db_init(":memory:");

    char result[64] = {0};
    esp_err_t ret = db_query_one("SELECT count(*) FROM memory", result, sizeof(result));
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING("0", result);

    ret = db_query_one("SELECT count(*) FROM sessions", result, sizeof(result));
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    ret = db_query_one("SELECT count(*) FROM tools_log", result, sizeof(result));
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    ret = db_query_one("SELECT count(*) FROM events", result, sizeof(result));
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    db_close();
}

void test_db_exec_invalid_sql(void)
{
    db_init(":memory:");
    esp_err_t ret = db_exec("NOT VALID SQL AT ALL");
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
    db_close();
}

void test_db_query_one_null_result(void)
{
    db_init(":memory:");
    db_exec("CREATE TABLE test4 (id INTEGER PRIMARY KEY, val TEXT)");

    char result[256] = "unchanged";
    esp_err_t ret = db_query_one("SELECT val FROM test4 WHERE id=999", result, sizeof(result));
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING("", result);
    db_close();
}

void test_db_query_one_null_args(void)
{
    db_init(":memory:");
    esp_err_t ret = db_query_one(NULL, NULL, 0);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    db_close();
}

void test_db_exec_null_sql(void)
{
    db_init(":memory:");
    esp_err_t ret = db_exec(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    db_close();
}

void test_db_double_init(void)
{
    db_init(":memory:");
    esp_err_t ret = db_init(":memory:");
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    db_close();
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_db_init_memory);
    RUN_TEST(test_db_init_null_fails);
    RUN_TEST(test_db_create_table);
    RUN_TEST(test_db_insert);
    RUN_TEST(test_db_select);
    RUN_TEST(test_db_close);
    RUN_TEST(test_db_schema_tables_created);
    RUN_TEST(test_db_exec_invalid_sql);
    RUN_TEST(test_db_query_one_null_result);
    RUN_TEST(test_db_query_one_null_args);
    RUN_TEST(test_db_exec_null_sql);
    RUN_TEST(test_db_double_init);
    return UNITY_END();
}
