#pragma once
#include "freertos/FreeRTOS.h"
#include <stddef.h>
typedef void *TimerHandle_t;
typedef void (*TimerCallbackFunction_t)(TimerHandle_t xTimer);
typedef struct { const char *pcTimerName; TickType_t xTimerPeriodInTicks; void *pvTimerID; TimerCallbackFunction_t pxCallbackFunction; } TimerParameters_t;
static inline TimerHandle_t xTimerCreate(const char *name, TickType_t period, int auto_reload, void *id, TimerCallbackFunction_t cb) { (void)name; (void)period; (void)auto_reload; (void)id; (void)cb; return (void*)1; }
static inline int xTimerStart(TimerHandle_t timer, TickType_t timeout) { (void)timer; (void)timeout; return 1; }
static inline int xTimerStop(TimerHandle_t timer, TickType_t timeout) { (void)timer; (void)timeout; return 1; }
static inline int xTimerDelete(TimerHandle_t timer, TickType_t timeout) { (void)timer; (void)timeout; return 1; }
static inline int xTimerReset(TimerHandle_t timer, TickType_t timeout) { (void)timer; (void)timeout; return 1; }
static inline void *pvTimerGetTimerID(TimerHandle_t timer) { (void)timer; return NULL; }
