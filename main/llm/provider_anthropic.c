#include "provider.h"
#include "mimi_config.h"
#include "proxy/http_proxy.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_heap_caps.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "provider_anthropic";

#define MAX_RESPONSE_SIZE (64 * 1024)

typedef struct {
    char *response_text;
    int tool_call_count;
    char tool_names[8][32];
    char tool_inputs[8][512];
} anthropic_ctx_t;

static esp_err_t anthropic_init(provider_t *prov, const provider_config_t *cfg)
{
    (void)prov;
    (void)cfg;
    return ESP_OK;
}

static esp_err_t anthropic_build_request(const char *system_prompt,
                                         const char *messages_json,
                                         const char *tools_json,
                                         char **out_body)
{
    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "model", "claude-3-5-sonnet-20241022");
    cJSON_AddNumberToObject(body, "max_tokens", 1024);

    if (system_prompt && system_prompt[0]) {
        cJSON_AddStringToObject(body, "system", system_prompt);
    }

    if (messages_json) {
        cJSON *msgs = cJSON_Parse(messages_json);
        if (msgs && cJSON_IsArray(msgs)) {
            cJSON_AddItemToObject(body, "messages", msgs);
        } else {
            if (msgs) cJSON_Delete(msgs);
        }
    }

    if (tools_json) {
        cJSON *tools = cJSON_Parse(tools_json);
        if (tools) {
            cJSON_AddItemToObject(body, "tools", tools);
        }
    }

    *out_body = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    return *out_body ? ESP_OK : ESP_ERR_NO_MEM;
}

static esp_err_t anthropic_chat(provider_t *prov,
                                 const char *system_prompt,
                                 const char *messages_json,
                                 const char *tools_json,
                                 char **out_text,
                                 int *out_tool_count)
{
    anthropic_ctx_t *ctx = (anthropic_ctx_t *)prov->context;
    if (!ctx) {
        ctx = calloc(1, sizeof(anthropic_ctx_t));
        if (!ctx) return ESP_ERR_NO_MEM;
        prov->context = ctx;
    }

    char *post_body = NULL;
    esp_err_t err = anthropic_build_request(system_prompt, messages_json, tools_json, &post_body);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "Calling Anthropic API...");

    /* HTTP call - simplified, using direct path */
    char *response = NULL;
    size_t resp_len = 0;
    
    /* TODO: Implement actual HTTP call using existing llm_proxy logic */
    /* For now, placeholder response */
    *out_text = strdup("Provider interface ready. Implement HTTP call.");
    *out_tool_count = 0;

    free(post_body);
    return ESP_OK;
}

static esp_err_t anthropic_free(provider_t *prov)
{
    anthropic_ctx_t *ctx = (anthropic_ctx_t *)prov->context;
    if (ctx) {
        free(ctx->response_text);
        for (int i = 0; i < ctx->tool_call_count; i++) {
            free(ctx->tool_inputs[i]);
        }
        free(ctx);
        prov->context = NULL;
    }
    return ESP_OK;
}

static provider_vtable_t anthropic_vtable = {
    .init = anthropic_init,
    .chat = anthropic_chat,
    .free = anthropic_free
};

provider_t *provider_anthropic_create(void)
{
    provider_t *prov = calloc(1, sizeof(provider_t));
    if (!prov) return NULL;

    prov->vtable = &anthropic_vtable;
    strncpy(prov->config.provider_id, "anthropic", sizeof(prov->config.provider_id) - 1);
    strncpy(prov->config.base_url, "https://api.anthropic.com", sizeof(prov->config.base_url) - 1);
    prov->config.api_style = PROVIDER_ANTHROPIC_NATIVE;
    prov->config.caps.supports_tools = true;
    prov->config.caps.supports_vision = true;
    prov->config.caps.supports_streaming = false;

    return prov;
}