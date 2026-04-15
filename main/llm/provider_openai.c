#include "provider.h"
#include "mimi_config.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "provider_openai";

typedef struct {
    char *response_text;
    int tool_call_count;
} openai_ctx_t;

static esp_err_t openai_init(provider_t *prov, const provider_config_t *cfg)
{
    (void)prov;
    (void)cfg;
    return ESP_OK;
}

static esp_err_t openai_chat(provider_t *prov,
                             const char *system_prompt,
                             const char *messages_json,
                             const char *tools_json,
                             char **out_text,
                             int *out_tool_count)
{
    openai_ctx_t *ctx = (openai_ctx_t *)prov->context;
    if (!ctx) {
        ctx = calloc(1, sizeof(openai_ctx_t));
        if (!ctx) return ESP_ERR_NO_MEM;
        prov->context = ctx;
    }

    ESP_LOGI(TAG, "Calling OpenAI-compatible API...");

    /* TODO: Implement actual HTTP call */
    *out_text = strdup("OpenAI provider ready. Implement HTTP call.");
    *out_tool_count = 0;

    return ESP_OK;
}

static esp_err_t openai_free(provider_t *prov)
{
    openai_ctx_t *ctx = (openai_ctx_t *)prov->context;
    if (ctx) {
        free(ctx->response_text);
        free(ctx);
        prov->context = NULL;
    }
    return ESP_OK;
}

static provider_vtable_t openai_vtable = {
    .init = openai_init,
    .chat = openai_chat,
    .free = openai_free
};

provider_t *provider_openai_create(void)
{
    provider_t *prov = calloc(1, sizeof(provider_t));
    if (!prov) return NULL;

    prov->vtable = &openai_vtable;
    strncpy(prov->config.provider_id, "openai", sizeof(prov->config.provider_id) - 1);
    strncpy(prov->config.base_url, "https://api.openai.com/v1", sizeof(prov->config.base_url) - 1);
    prov->config.api_style = PROVIDER_OPENAI_COMPATIBLE;
    prov->config.caps.supports_tools = true;
    prov->config.caps.supports_vision = false;
    prov->config.caps.supports_streaming = false;

    return prov;
}