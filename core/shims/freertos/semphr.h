#pragma once
#include <pthread.h>
#include <stdlib.h>

typedef pthread_mutex_t *SemaphoreHandle_t;

static inline SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    pthread_mutex_t *m = calloc(1, sizeof(pthread_mutex_t));
    if (m) pthread_mutex_init(m, NULL);
    return m;
}

static inline int xSemaphoreTake(SemaphoreHandle_t sem, uint32_t timeout_ms) {
    (void)timeout_ms;
    return sem ? pthread_mutex_lock(sem) == 0 : 0;
}

static inline int xSemaphoreGive(SemaphoreHandle_t sem) {
    return sem ? pthread_mutex_unlock(sem) == 0 : 0;
}

static inline void vSemaphoreDelete(SemaphoreHandle_t sem) {
    if (sem) { pthread_mutex_destroy(sem); free(sem); }
}
