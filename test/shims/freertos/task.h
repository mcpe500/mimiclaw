#pragma once
#include "FreeRTOS.h"

static inline TickType_t xTaskGetTickCount(void) { return 1000; }
static inline BaseType_t xTaskCreatePinnedToCore(void (*task_code)(void *), const char *const pxTaskName, uint32_t usStackDepth, void *pvParameters, uint32_t uxPriority, TaskHandle_t *pxCreatedTask, uint8_t xCoreID) { return pdPASS; }
static inline void vTaskDelay(TickType_t xTicksToDelay) { (void)xTicksToDelay; }
static inline void vTaskDelete(TaskHandle_t xHandle) { (void)xHandle; }
