#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Provider API styles */
typedef enum {
    PROVIDER_OPENAI_COMPATIBLE,
    PROVIDER_ANTHROPIC_NATIVE
} provider_api_style_t;

/* Provider capabilities */
typedef struct {
    bool supports_tools;
    bool supports_vision;
    bool supports_streaming;
} provider_caps_t;

/* Provider configuration */
typedef struct {
    char provider_id[32];
    char base_url[128];
    provider_api_style_t api_style;
    char api_key[320];
    char model[64];
    provider_caps_t caps;
} provider_config_t;

/* Forward declaration */
typedef struct provider provider_t;

/* Provider interface (virtual table) */
typedef struct {
    esp_err_t (*init)(provider_t *prov, const provider_config_t *cfg);
    esp_err_t (*chat)(provider_t *prov, const char *system_prompt, 
                      const char *messages_json, const char *tools_json,
                      char **out_text, int *out_tool_call_count);
    esp_err_t (*free)(provider_t *prov);
} provider_vtable_t;

/* Provider instance */
struct provider {
    const provider_vtable_t *vtable;
    provider_config_t config;
    void *context;
};

/* Provider registration */
typedef provider_t* (*provider_create_fn)(void);

esp_err_t provider_register(const char *id, provider_create_fn create_fn);
provider_t *provider_get(const char *id);
esp_err_t provider_list(const char **ids, int *count);

/* Standard providers */
provider_t *provider_anthropic_create(void);
provider_t *provider_openai_create(void);

/* Helper macros */
#define PROVIDER_CALL(prov, method, ...) \
    ((prov) && (prov)->vtable->method ? (prov)->vtable->method((prov), ##__VA_ARGS__) : ESP_ERR_INVALID_ARG)

#ifdef __cplusplus
}
#endif