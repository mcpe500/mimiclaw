#pragma once
#include "freertos/FreeRTOS.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef void (*TaskFunction_t)(void *arg);

typedef struct {
    pthread_t thread;
    TaskFunction_t fn;
    void *arg;
    char name[16];
} task_ctx_t;

static void *task_wrapper(void *arg) {
    task_ctx_t *ctx = (task_ctx_t *)arg;
    ctx->fn(ctx->arg);
    free(ctx);
    return NULL;
}

#define pdMS_TO_TICKS(ms) (ms)
#define pdPASS 1

static inline BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t pvTaskCode, const char * const pcName,
    const uint16_t usStackDepth, void * const pvParameters,
    BaseType_t uxPriority, TaskHandle_t * const pvCreatedTask,
    const BaseType_t xCoreID)
{
    (void)usStackDepth; (void)uxPriority; (void)xCoreID;
    task_ctx_t *ctx = calloc(1, sizeof(task_ctx_t));
    if (!ctx) return 0;
    ctx->fn = pvTaskCode;
    ctx->arg = pvParameters;
    if (pcName) strncpy(ctx->name, pcName, sizeof(ctx->name) - 1);
    pthread_t tid;
    if (pthread_create(&tid, NULL, task_wrapper, ctx) != 0) { free(ctx); return 0; }
    pthread_detach(tid);
    if (pvCreatedTask) *pvCreatedTask = (TaskHandle_t)ctx;
    return pdPASS;
}

static inline BaseType_t xTaskCreate(
    TaskFunction_t pvTaskCode, const char * const pcName,
    const uint16_t usStackDepth, void * const pvParameters,
    BaseType_t uxPriority, TaskHandle_t * const pvCreatedTask)
{
    return xTaskCreatePinnedToCore(pvTaskCode, pcName, usStackDepth,
        pvParameters, uxPriority, pvCreatedTask, 0);
}

static inline void vTaskDelete(TaskHandle_t task) { (void)task; }
static inline BaseType_t xPortGetCoreID(void) { return 0; }
