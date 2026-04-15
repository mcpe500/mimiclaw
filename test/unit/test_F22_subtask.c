#include "unity.h"
#include "agent/subtask.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_subtask_init(void)
{
    esp_err_t ret = subtask_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    subtask_deinit();
}

void test_subtask_submit(void)
{
    subtask_init();
    char job_id[64] = {0};
    esp_err_t ret = subtask_submit("test job", job_id, sizeof(job_id));
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(strlen(job_id) > 0);
    subtask_deinit();
}

void test_subtask_get_status(void)
{
    subtask_init();
    char job_id[64] = {0};
    subtask_submit("status test", job_id, sizeof(job_id));

    subtask_job_t job = {0};
    esp_err_t ret = subtask_get_status(job_id, &job);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(JOB_PENDING, job.status);
    TEST_ASSERT_EQUAL_STRING("status test", job.description);
    subtask_deinit();
}

void test_subtask_cancel(void)
{
    subtask_init();
    char job_id[64] = {0};
    subtask_submit("cancel test", job_id, sizeof(job_id));

    esp_err_t ret = subtask_cancel(job_id);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    subtask_job_t job = {0};
    subtask_get_status(job_id, &job);
    TEST_ASSERT_EQUAL(JOB_CANCELLED, job.status);
    subtask_deinit();
}

void test_subtask_set_result(void)
{
    subtask_init();
    char job_id[64] = {0};
    subtask_submit("result test", job_id, sizeof(job_id));

    esp_err_t ret = subtask_set_result(job_id, "done successfully");
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    subtask_job_t job = {0};
    subtask_get_status(job_id, &job);
    TEST_ASSERT_EQUAL(JOB_COMPLETED, job.status);
    TEST_ASSERT_EQUAL_STRING("done successfully", job.result);
    TEST_ASSERT_EQUAL(100, job.progress);
    subtask_deinit();
}

void test_subtask_set_error(void)
{
    subtask_init();
    char job_id[64] = {0};
    subtask_submit("error test", job_id, sizeof(job_id));

    esp_err_t ret = subtask_set_error(job_id, "something broke");
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    subtask_job_t job = {0};
    subtask_get_status(job_id, &job);
    TEST_ASSERT_EQUAL(JOB_FAILED, job.status);
    TEST_ASSERT_EQUAL_STRING("something broke", job.error);
    subtask_deinit();
}

void test_subtask_active_count(void)
{
    subtask_init();
    TEST_ASSERT_EQUAL(0, subtask_active_count());

    char id1[64], id2[64];
    subtask_submit("job1", id1, sizeof(id1));
    TEST_ASSERT_EQUAL(1, subtask_active_count());

    subtask_submit("job2", id2, sizeof(id2));
    TEST_ASSERT_EQUAL(2, subtask_active_count());

    subtask_set_result(id1, "done");
    TEST_ASSERT_EQUAL(1, subtask_active_count());

    subtask_deinit();
}

void test_subtask_deinit(void)
{
    subtask_init();
    char job_id[64] = {0};
    subtask_submit("deinit test", job_id, sizeof(job_id));
    subtask_deinit();
    TEST_ASSERT_EQUAL(0, subtask_active_count());
}

void test_subtask_lifecycle_pending_to_completed(void)
{
    subtask_init();
    char job_id[64] = {0};
    subtask_submit("lifecycle", job_id, sizeof(job_id));

    subtask_job_t job = {0};
    subtask_get_status(job_id, &job);
    TEST_ASSERT_EQUAL(JOB_PENDING, job.status);

    subtask_set_result(job_id, "finished");
    subtask_get_status(job_id, &job);
    TEST_ASSERT_EQUAL(JOB_COMPLETED, job.status);
    subtask_deinit();
}

void test_subtask_cancel_from_pending(void)
{
    subtask_init();
    char job_id[64] = {0};
    subtask_submit("cancel pending", job_id, sizeof(job_id));

    esp_err_t ret = subtask_cancel(job_id);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    subtask_job_t job = {0};
    subtask_get_status(job_id, &job);
    TEST_ASSERT_EQUAL(JOB_CANCELLED, job.status);
    subtask_deinit();
}

void test_subtask_max_capacity(void)
{
    subtask_init();
    char job_ids[32][64];

    for (int i = 0; i < 32; i++) {
        char desc[32];
        snprintf(desc, sizeof(desc), "max job %d", i);
        esp_err_t ret = subtask_submit(desc, job_ids[i], sizeof(job_ids[i]));
        TEST_ASSERT_EQUAL(ESP_OK, ret);
    }

    char overflow_id[64] = {0};
    esp_err_t ret = subtask_submit("overflow", overflow_id, sizeof(overflow_id));
    TEST_ASSERT_EQUAL(ESP_ERR_NO_MEM, ret);

    subtask_deinit();
}

void test_subtask_double_cancel(void)
{
    subtask_init();
    char job_id[64] = {0};
    subtask_submit("double cancel", job_id, sizeof(job_id));

    subtask_cancel(job_id);
    esp_err_t ret = subtask_cancel(job_id);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
    subtask_deinit();
}

void test_subtask_get_status_unknown(void)
{
    subtask_init();
    subtask_job_t job = {0};
    esp_err_t ret = subtask_get_status("nonexistent", &job);
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, ret);
    subtask_deinit();
}

void test_subtask_set_result_on_cancelled_fails(void)
{
    subtask_init();
    char job_id[64] = {0};
    subtask_submit("cancelled result", job_id, sizeof(job_id));
    subtask_cancel(job_id);

    esp_err_t ret = subtask_set_result(job_id, "too late");
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
    subtask_deinit();
}

void test_subtask_null_args(void)
{
    subtask_init();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, subtask_submit(NULL, NULL, 0));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, subtask_get_status(NULL, NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, subtask_cancel(NULL));
    subtask_deinit();
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_subtask_init);
    RUN_TEST(test_subtask_submit);
    RUN_TEST(test_subtask_get_status);
    RUN_TEST(test_subtask_cancel);
    RUN_TEST(test_subtask_set_result);
    RUN_TEST(test_subtask_set_error);
    RUN_TEST(test_subtask_active_count);
    RUN_TEST(test_subtask_deinit);
    RUN_TEST(test_subtask_lifecycle_pending_to_completed);
    RUN_TEST(test_subtask_cancel_from_pending);
    RUN_TEST(test_subtask_max_capacity);
    RUN_TEST(test_subtask_double_cancel);
    RUN_TEST(test_subtask_get_status_unknown);
    RUN_TEST(test_subtask_set_result_on_cancelled_fails);
    RUN_TEST(test_subtask_null_args);
    return UNITY_END();
}
