#pragma once
#include "FreeRTOS.h"

typedef int EventGroupHandle_t;

static inline EventGroupHandle_t xEventGroupCreate(void) { return (EventGroupHandle_t)1; }
static inline BaseType_t xEventGroupSetBits(EventGroupHandle_t xEventGroup, const uint32_t uxBitsToSet) { (void)xEventGroup; (void)uxBitsToSet; return pdPASS; }
static inline BaseType_t xEventGroupWaitBits(EventGroupHandle_t xEventGroup, const uint32_t uxBitsToWaitFor, BaseType_t uxWaitAll, BaseType_t uxClearOnExit, TickType_t xTicksToWait) { (void)xEventGroup; (void)uxBitsToWaitFor; (void)uxWaitAll; (void)uxClearOnExit; (void)xTicksToWait; return 0; }
