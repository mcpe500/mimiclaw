#pragma once
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>

typedef uint32_t EventBits_t;

typedef struct {
    EventBits_t bits;
    pthread_mutex_t mtx;
    pthread_cond_t cond;
} event_group_t;

typedef event_group_t *EventGroupHandle_t;

static inline EventGroupHandle_t xEventGroupCreate(void) {
    event_group_t *eg = calloc(1, sizeof(event_group_t));
    if (eg) { pthread_mutex_init(&eg->mtx, NULL); pthread_cond_init(&eg->cond, NULL); }
    return eg;
}

static inline EventBits_t xEventGroupSetBits(EventGroupHandle_t eg, EventBits_t bits) {
    if (!eg) return 0;
    pthread_mutex_lock(&eg->mtx);
    EventBits_t prev = eg->bits;
    eg->bits |= bits;
    pthread_cond_broadcast(&eg->cond);
    pthread_mutex_unlock(&eg->mtx);
    return prev;
}

static inline EventBits_t xEventGroupClearBits(EventGroupHandle_t eg, EventBits_t bits) {
    if (!eg) return 0;
    pthread_mutex_lock(&eg->mtx);
    EventBits_t prev = eg->bits;
    eg->bits &= ~bits;
    pthread_mutex_unlock(&eg->mtx);
    return prev;
}

static inline EventBits_t xEventGroupWaitBits(
    EventGroupHandle_t eg, EventBits_t bits, int clearOnExit, int waitForAll, uint32_t timeout_ms)
{
    (void)clearOnExit; (void)waitForAll; (void)timeout_ms;
    if (!eg) return 0;
    pthread_mutex_lock(&eg->mtx);
    while (!(eg->bits & bits)) {
        pthread_cond_wait(&eg->cond, &eg->mtx);
    }
    EventBits_t result = eg->bits;
    if (clearOnExit) eg->bits &= ~bits;
    pthread_mutex_unlock(&eg->mtx);
    return result;
}

static inline void vEventGroupDelete(EventGroupHandle_t eg) {
    if (eg) { pthread_mutex_destroy(&eg->mtx); pthread_cond_destroy(&eg->cond); free(eg); }
}
