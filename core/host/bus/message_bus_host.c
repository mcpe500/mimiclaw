#include "message_bus_host.h"
#include "esp_log.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

static const char *TAG = "msg_bus_host";

#define BUS_QUEUE_LEN 64

typedef struct {
    char channel[16];
    char chat_id[96];
    char *content;
    char user_id[64];
    char message_id[64];
    int media_count;
    char media_paths[4][64];
    char reply_to[64];
    char metadata[256];
} mimi_msg_t;

typedef struct {
    mimi_msg_t *buf[BUS_QUEUE_LEN];
    int head;
    int tail;
    int count;
    pthread_mutex_t mtx;
    pthread_cond_t cond;
} msg_queue_t;

static msg_queue_t s_inbound;
static msg_queue_t s_outbound;
static int s_initialized = 0;

static void queue_init(msg_queue_t *q) {
    memset(q, 0, sizeof(*q));
    pthread_mutex_init(&q->mtx, NULL);
    pthread_cond_init(&q->cond, NULL);
}

static void queue_deinit(msg_queue_t *q) {
    pthread_mutex_lock(&q->mtx);
    for (int i = 0; i < q->count; i++) {
        int idx = (q->head + i) % BUS_QUEUE_LEN;
        if (q->buf[idx]) {
            free(q->buf[idx]->content);
            free(q->buf[idx]);
            q->buf[idx] = NULL;
        }
    }
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    pthread_mutex_unlock(&q->mtx);
    pthread_mutex_destroy(&q->mtx);
    pthread_cond_destroy(&q->cond);
}

static esp_err_t queue_push(msg_queue_t *q, const mimi_msg_t *msg) {
    if (!msg) return ESP_ERR_INVALID_ARG;

    mimi_msg_t *copy = calloc(1, sizeof(mimi_msg_t));
    if (!copy) return ESP_ERR_NO_MEM;

    memcpy(copy, msg, sizeof(mimi_msg_t));
    if (msg->content) {
        copy->content = strdup(msg->content);
        if (!copy->content) {
            free(copy);
            return ESP_ERR_NO_MEM;
        }
    } else {
        copy->content = NULL;
    }

    pthread_mutex_lock(&q->mtx);

    if (q->count >= BUS_QUEUE_LEN) {
        pthread_mutex_unlock(&q->mtx);
        free(copy->content);
        free(copy);
        ESP_LOGW(TAG, "queue full, dropping message");
        return ESP_ERR_NO_MEM;
    }

    q->buf[q->tail] = copy;
    q->tail = (q->tail + 1) % BUS_QUEUE_LEN;
    q->count++;

    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mtx);
    return ESP_OK;
}

static esp_err_t queue_pop(msg_queue_t *q, mimi_msg_t *msg, uint32_t timeout_ms) {
    if (!msg) return ESP_ERR_INVALID_ARG;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += ts.tv_nsec / 1000000000L;
        ts.tv_nsec %= 1000000000L;
    }

    pthread_mutex_lock(&q->mtx);

    while (q->count == 0) {
        int rc = pthread_cond_timedwait(&q->cond, &q->mtx, &ts);
        if (rc == ETIMEDOUT) {
            pthread_mutex_unlock(&q->mtx);
            return ESP_ERR_TIMEOUT;
        }
        if (rc != 0) {
            pthread_mutex_unlock(&q->mtx);
            return ESP_FAIL;
        }
    }

    mimi_msg_t *front = q->buf[q->head];
    q->head = (q->head + 1) % BUS_QUEUE_LEN;
    q->count--;

    pthread_mutex_unlock(&q->mtx);

    if (msg->content) {
        free(msg->content);
        msg->content = NULL;
    }
    memcpy(msg, front, sizeof(mimi_msg_t));
    free(front);

    return ESP_OK;
}

esp_err_t message_bus_init(void) {
    if (s_initialized) return ESP_OK;
    queue_init(&s_inbound);
    queue_init(&s_outbound);
    s_initialized = 1;
    ESP_LOGI(TAG, "message bus initialized (pthread, queue=%d)", BUS_QUEUE_LEN);
    return ESP_OK;
}

esp_err_t message_bus_push_inbound(const void *msg) {
    return queue_push(&s_inbound, (const mimi_msg_t *)msg);
}

esp_err_t message_bus_pop_inbound(void *msg, uint32_t timeout_ms) {
    return queue_pop(&s_inbound, (mimi_msg_t *)msg, timeout_ms);
}

esp_err_t message_bus_push_outbound(const void *msg) {
    return queue_push(&s_outbound, (const mimi_msg_t *)msg);
}

esp_err_t message_bus_pop_outbound(void *msg, uint32_t timeout_ms) {
    return queue_pop(&s_outbound, (mimi_msg_t *)msg, timeout_ms);
}

void message_bus_deinit(void) {
    if (!s_initialized) return;
    queue_deinit(&s_inbound);
    queue_deinit(&s_outbound);
    s_initialized = 0;
    ESP_LOGI(TAG, "message bus deinitialized");
}
