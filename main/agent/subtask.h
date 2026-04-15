#pragma once
#include "esp_err.h"
#include <stddef.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    JOB_PENDING,
    JOB_RUNNING,
    JOB_COMPLETED,
    JOB_FAILED,
    JOB_CANCELLED
} job_status_t;

typedef struct {
    char job_id[64];
    char description[256];
    job_status_t status;
    int progress;
    char result[1024];
    char error[256];
} subtask_job_t;

esp_err_t subtask_init(void);
esp_err_t subtask_submit(const char *description, char *job_id_out, size_t id_size);
esp_err_t subtask_get_status(const char *job_id, subtask_job_t *out);
esp_err_t subtask_cancel(const char *job_id);
esp_err_t subtask_set_result(const char *job_id, const char *result);
esp_err_t subtask_set_error(const char *job_id, const char *error);
int subtask_active_count(void);
void subtask_deinit(void);

#ifdef __cplusplus
}
#endif
