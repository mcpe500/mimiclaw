#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t TickType_t;
typedef void *TaskHandle_t;

#define pdTRUE 1
#define pdFALSE 0
#define pdPASS 1
#define pdFAIL 0
#define portTICK_PERIOD_MS 10

typedef int BaseType_t;
