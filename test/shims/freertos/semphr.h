#pragma once
#include "freertos/FreeRTOS.h"
#include <stdbool.h>
typedef void *SemaphoreHandle_t;
static inline SemaphoreHandle_t xSemaphoreCreateMutex(void) { return (void*)1; }
static inline SemaphoreHandle_t xSemaphoreCreateBinary(void) { return (void*)2; }
static inline int xSemaphoreTake(SemaphoreHandle_t sem, TickType_t timeout) { (void)sem; (void)timeout; return pdTRUE; }
static inline int xSemaphoreGive(SemaphoreHandle_t sem) { (void)sem; return pdTRUE; }
static inline void vSemaphoreDelete(SemaphoreHandle_t sem) { (void)sem; }
static inline int xSemaphoreTakeRecursive(SemaphoreHandle_t sem, TickType_t timeout) { (void)sem; (void)timeout; return pdTRUE; }
static inline SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(void) { return (void*)3; }
