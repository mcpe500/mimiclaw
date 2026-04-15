#include "unity.h"
#include "provider.h"
#include <string.h>
#include <stdlib.h>

static provider_t *test_prov = NULL;

void setUp(void)
{
}

void tearDown(void)
{
    if (test_prov && test_prov->vtable->free) {
        test_prov->vtable->free(test_prov);
    }
    test_prov = NULL;
}

/* Test F1.1: Provider struct exists */
void test_provider_struct_exists(void)
{
    provider_t prov = {0};
    TEST_ASSERT_NOT_NULL(&prov);
    TEST_ASSERT_NULL(prov.vtable);
}

/* Test F1.2: Provider config struct exists */
void test_provider_config_exists(void)
{
    provider_config_t cfg = {0};
    TEST_ASSERT_NOT_NULL(cfg.provider_id);
    TEST_ASSERT_NOT_NULL(cfg.base_url);
    TEST_ASSERT_EQUAL(PROVIDER_OPENAI_COMPATIBLE, cfg.api_style);
}

/* Test F1.3: Provider API styles enum */
void test_provider_api_styles(void)
{
    TEST_ASSERT_EQUAL(0, PROVIDER_OPENAI_COMPATIBLE);
    TEST_ASSERT_EQUAL(1, PROVIDER_ANTHROPIC_NATIVE);
}

/* Test F1.4: Provider capabilities struct */
void test_provider_caps(void)
{
    provider_caps_t caps = {
        .supports_tools = true,
        .supports_vision = false,
        .supports_streaming = false
    };
    TEST_ASSERT_TRUE(caps.supports_tools);
    TEST_ASSERT_FALSE(caps.supports_vision);
}

/* Test F1.5: Anthropic provider creation */
void test_provider_anthropic_create(void)
{
    test_prov = provider_anthropic_create();
    TEST_ASSERT_NOT_NULL(test_prov);
    TEST_ASSERT_NOT_NULL(test_prov->vtable);
    TEST_ASSERT_EQUAL_STRING("anthropic", test_prov->config.provider_id);
    TEST_ASSERT_EQUAL(PROVIDER_ANTHROPIC_NATIVE, test_prov->config.api_style);
    TEST_ASSERT_TRUE(test_prov->config.caps.supports_tools);
    TEST_ASSERT_TRUE(test_prov->config.caps.supports_vision);
}

/* Test F1.6: OpenAI provider creation */
void test_provider_openai_create(void)
{
    test_prov = provider_openai_create();
    TEST_ASSERT_NOT_NULL(test_prov);
    TEST_ASSERT_NOT_NULL(test_prov->vtable);
    TEST_ASSERT_EQUAL_STRING("openai", test_prov->config.provider_id);
    TEST_ASSERT_EQUAL(PROVIDER_OPENAI_COMPATIBLE, test_prov->config.api_style);
    TEST_ASSERT_TRUE(test_prov->config.caps.supports_tools);
    TEST_ASSERT_FALSE(test_prov->config.caps.supports_vision);
}

/* Test F1.7: Provider interface has required methods */
void test_provider_vtable_methods(void)
{
    test_prov = provider_anthropic_create();
    TEST_ASSERT_NOT_NULL(test_prov->vtable->init);
    TEST_ASSERT_NOT_NULL(test_prov->vtable->chat);
    TEST_ASSERT_NOT_NULL(test_prov->vtable->free);
}

/* Test F1.8: Provider can be initialized */
void test_provider_init(void)
{
    test_prov = provider_anthropic_create();
    provider_config_t cfg = {0};
    strncpy(cfg.api_key, "test-key", sizeof(cfg.api_key) - 1);
    strncpy(cfg.model, "claude-3", sizeof(cfg.model) - 1);
    
    esp_err_t err = test_prov->vtable->init(test_prov, &cfg);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

/* Test F1.9: Provider can be freed */
void test_provider_free(void)
{
    test_prov = provider_anthropic_create();
    esp_err_t err = test_prov->vtable->free(test_prov);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    test_prov = NULL;
}

void test_provider_anthropic_chat(void)
{
    test_prov = provider_anthropic_create();
    char *out_text = NULL;
    int tool_count = 0;
    
    esp_err_t err = test_prov->vtable->chat(test_prov, "system", "[]", NULL, &out_text, &tool_count);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_NOT_NULL(out_text);
    TEST_ASSERT_EQUAL(0, tool_count);
    free(out_text);
}

void test_provider_openai_chat(void)
{
    test_prov = provider_openai_create();
    char *out_text = NULL;
    int tool_count = 0;
    
    esp_err_t err = test_prov->vtable->chat(test_prov, "system", "[]", NULL, &out_text, &tool_count);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_NOT_NULL(out_text);
    TEST_ASSERT_EQUAL(0, tool_count);
    free(out_text);
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_provider_struct_exists);
    RUN_TEST(test_provider_config_exists);
    RUN_TEST(test_provider_api_styles);
    RUN_TEST(test_provider_caps);
    RUN_TEST(test_provider_anthropic_create);
    RUN_TEST(test_provider_openai_create);
    RUN_TEST(test_provider_vtable_methods);
    RUN_TEST(test_provider_init);
    RUN_TEST(test_provider_free);
    RUN_TEST(test_provider_anthropic_chat);
    RUN_TEST(test_provider_openai_chat);
    return UNITY_END();
}