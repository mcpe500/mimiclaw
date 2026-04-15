#pragma once
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

typedef void *QueueHandle_t;

static inline QueueHandle_t xQueueCreate(int len, int size) { (void)len; (void)size; return NULL; }
static inline void vQueueDelete(QueueHandle_t q) { (void)q; }
static inline int xQueueSend(QueueHandle_t q, const void *item, uint32_t timeout) { (void)q; (void)item; (void)timeout; return 1; }
static inline int xQueueReceive(QueueHandle_t q, void *item, uint32_t timeout) { (void)q; (void)item; (void)timeout; return 0; }
