#pragma once
#include "FreeRTOS.h"

typedef void *QueueHandle_t;

static inline QueueHandle_t xQueueCreate(uint32_t uxQueueLength, uint32_t uxItemSize) { return (QueueHandle_t)1; }
static inline BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait) { return pdPASS; }
static inline BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait) { return pdPASS; }
static inline BaseType_t xQueueGenericSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait, uint32_t uxItemSize) { return pdPASS; }
static inline BaseType_t xQueueGenericReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait, uint32_t uxItemSize) { return pdPASS; }
