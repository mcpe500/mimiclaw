#include "subtask.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "subtask";

#define MAX_JOBS 32

static subtask_job_t s_jobs[MAX_JOBS];
static bool s_jobs_used[MAX_JOBS];
static int s_job_counter = 0;
static SemaphoreHandle_t s_mutex = NULL;
static bool s_initialized = false;

static int find_job_index(const char *job_id)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        if (s_jobs_used[i] && strcmp(s_jobs[i].job_id, job_id) == 0) {
            return i;
        }
    }
    return -1;
}

esp_err_t subtask_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Subtask already initialized");
        return ESP_OK;
    }

    memset(s_jobs, 0, sizeof(s_jobs));
    memset(s_jobs_used, 0, sizeof(s_jobs_used));
    s_job_counter = 0;

    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Subtask system initialized (max %d jobs)", MAX_JOBS);
    return ESP_OK;
}

esp_err_t subtask_submit(const char *description, char *job_id_out, size_t id_size)
{
    if (!s_initialized || !description || !job_id_out || id_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    int slot = -1;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!s_jobs_used[i]) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        xSemaphoreGive(s_mutex);
        ESP_LOGE(TAG, "Max jobs reached (%d)", MAX_JOBS);
        return ESP_ERR_NO_MEM;
    }

    s_job_counter++;
    snprintf(s_jobs[slot].job_id, sizeof(s_jobs[slot].job_id), "job_%d", s_job_counter);
    strncpy(s_jobs[slot].description, description, sizeof(s_jobs[slot].description) - 1);
    s_jobs[slot].status = JOB_PENDING;
    s_jobs[slot].progress = 0;
    s_jobs[slot].result[0] = '\0';
    s_jobs[slot].error[0] = '\0';
    s_jobs_used[slot] = true;

    strncpy(job_id_out, s_jobs[slot].job_id, id_size - 1);
    job_id_out[id_size - 1] = '\0';

    xSemaphoreGive(s_mutex);

    ESP_LOGI(TAG, "Job submitted: %s - %s", s_jobs[slot].job_id, description);
    return ESP_OK;
}

esp_err_t subtask_get_status(const char *job_id, subtask_job_t *out)
{
    if (!s_initialized || !job_id || !out) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    int idx = find_job_index(job_id);
    if (idx < 0) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    memcpy(out, &s_jobs[idx], sizeof(subtask_job_t));
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t subtask_cancel(const char *job_id)
{
    if (!s_initialized || !job_id) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    int idx = find_job_index(job_id);
    if (idx < 0) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (s_jobs[idx].status == JOB_COMPLETED || s_jobs[idx].status == JOB_FAILED) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    s_jobs[idx].status = JOB_CANCELLED;
    xSemaphoreGive(s_mutex);

    ESP_LOGI(TAG, "Job cancelled: %s", job_id);
    return ESP_OK;
}

esp_err_t subtask_set_result(const char *job_id, const char *result)
{
    if (!s_initialized || !job_id || !result) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    int idx = find_job_index(job_id);
    if (idx < 0) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (s_jobs[idx].status == JOB_CANCELLED) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    strncpy(s_jobs[idx].result, result, sizeof(s_jobs[idx].result) - 1);
    s_jobs[idx].progress = 100;
    s_jobs[idx].status = JOB_COMPLETED;
    xSemaphoreGive(s_mutex);

    ESP_LOGI(TAG, "Job completed: %s", job_id);
    return ESP_OK;
}

esp_err_t subtask_set_error(const char *job_id, const char *error)
{
    if (!s_initialized || !job_id || !error) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    int idx = find_job_index(job_id);
    if (idx < 0) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (s_jobs[idx].status == JOB_CANCELLED) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    strncpy(s_jobs[idx].error, error, sizeof(s_jobs[idx].error) - 1);
    s_jobs[idx].status = JOB_FAILED;
    xSemaphoreGive(s_mutex);

    ESP_LOGI(TAG, "Job failed: %s - %s", job_id, error);
    return ESP_OK;
}

int subtask_active_count(void)
{
    if (!s_initialized) return 0;

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    int count = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (s_jobs_used[i] &&
            (s_jobs[i].status == JOB_PENDING || s_jobs[i].status == JOB_RUNNING)) {
            count++;
        }
    }
    xSemaphoreGive(s_mutex);
    return count;
}

void subtask_deinit(void)
{
    if (!s_initialized) return;

    if (s_mutex) {
        vSemaphoreDelete(s_mutex);
        s_mutex = NULL;
    }

    memset(s_jobs, 0, sizeof(s_jobs));
    memset(s_jobs_used, 0, sizeof(s_jobs_used));
    s_job_counter = 0;
    s_initialized = false;

    ESP_LOGI(TAG, "Subtask system deinitialized");
}
