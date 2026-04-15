#pragma once
#include <stdint.h>
#include <stddef.h>
typedef void *esp_timer_handle_t;
typedef struct { uint64_t period_us; void (*callback)(void *arg); void *arg; const char *name; } esp_timer_create_args_t;
static inline int64_t esp_timer_get_time(void) { return 0; }
static inline esp_err_t esp_timer_create(const esp_timer_create_args_t *args, esp_timer_handle_t *handle) { (void)args; if (handle) *handle = NULL; return 0; }
static inline esp_err_t esp_timer_start_periodic(esp_timer_handle_t handle, uint64_t period) { (void)handle; (void)period; return 0; }
static inline esp_err_t esp_timer_stop(esp_timer_handle_t handle) { (void)handle; return 0; }
static inline esp_err_t esp_timer_delete(esp_timer_handle_t handle) { (void)handle; return 0; }
