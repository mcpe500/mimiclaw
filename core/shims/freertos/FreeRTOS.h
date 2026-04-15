#pragma once
#include <stdint.h>
#include <unistd.h>

typedef void *TaskHandle_t;
typedef uint32_t TickType_t;
typedef int BaseType_t;

#define pdTRUE  1
#define pdFALSE 0
#define portMAX_DELAY UINT32_MAX

#define vTaskDelay(ms) usleep((ms) * 1000)

static inline TickType_t xTaskGetTickCount(void) { return 0; }
