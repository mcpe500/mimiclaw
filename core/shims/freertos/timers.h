#pragma once
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

typedef void (*TimerCallback_t)(void *xTimer);

typedef struct {
    pthread_t thread;
    TimerCallback_t cb;
    uint32_t period_ms;
    bool running;
    bool one_shot;
    void *arg;
} host_timer_t;

typedef host_timer_t *TimerHandle_t;

static void *timer_thread(void *arg) {
    host_timer_t *t = (host_timer_t *)arg;
    while (t->running) {
        usleep(t->period_ms * 1000);
        if (!t->running) break;
        if (t->cb) t->cb(t);
        if (t->one_shot) break;
    }
    return NULL;
}

#define pdTRUE 1
#define pdFALSE 0
#define portMAX_DELAY UINT32_MAX

static inline TimerHandle_t xTimerCreate(
    const char *name, uint32_t period_ticks, bool one_shot,
    void *timer_id, TimerCallback_t cb)
{
    (void)name; (void)timer_id;
    host_timer_t *t = calloc(1, sizeof(host_timer_t));
    if (t) { t->cb = cb; t->period_ms = period_ticks; t->one_shot = one_shot; t->arg = timer_id; }
    return t;
}

static inline int xTimerStart(TimerHandle_t t, uint32_t timeout) {
    (void)timeout;
    if (!t) return 0;
    t->running = true;
    pthread_create(&t->thread, NULL, timer_thread, t);
    pthread_detach(t->thread);
    return pdTRUE;
}

static inline int xTimerStop(TimerHandle_t t, uint32_t timeout) {
    (void)timeout;
    if (!t) return 0;
    t->running = false;
    return pdTRUE;
}

static inline void xTimerDelete(TimerHandle_t t, uint32_t timeout) {
    (void)timeout;
    if (t) { t->running = false; free(t); }
}
