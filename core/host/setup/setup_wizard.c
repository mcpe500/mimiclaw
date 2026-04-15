#include "setup_wizard.h"
#include "config.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static const char *TAG = "setup";

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

static void trim_whitespace(char *s) {
    trim_newline(s);
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
}

static bool read_line(char *buf, size_t size) {
    if (!fgets(buf, (int)size, stdin)) return false;
    trim_whitespace(buf);
    return true;
}

static int read_choice(int min, int max) {
    char buf[16];
    if (!read_line(buf, sizeof(buf))) return min;
    int val = atoi(buf);
    if (val < min || val > max) return min;
    return val;
}

static bool read_yes_no(void) {
    char buf[16];
    if (!read_line(buf, sizeof(buf))) return false;
    return buf[0] == 'y' || buf[0] == 'Y';
}

static void read_required(char *buf, size_t size, const char *prompt) {
    while (1) {
        printf("  %s", prompt);
        fflush(stdout);
        if (!read_line(buf, size)) continue;
        if (buf[0] != '\0') break;
        printf("  (required, please enter a value)\n");
    }
}

static void configure_channel(mimi_channel_config_t *ch, const char *name) {
    printf("\n  Configure %s? (y/n): ", name);
    fflush(stdout);
    if (!read_yes_no()) {
        ch->enabled = false;
        return;
    }
    ch->enabled = true;
    char prompt[128];
    snprintf(prompt, sizeof(prompt), "%s Token: ", name);
    read_required(ch->token, sizeof(ch->token), prompt);
}

static void step_llm_provider(mimi_config_t *cfg) {
    printf("\nStep 1: LLM Provider\n");
    printf("  1) Anthropic (Claude)\n");
    printf("  2) OpenAI (GPT)\n");
    printf("  3) Custom\n");
    printf("  Choice [1]: ");
    fflush(stdout);

    int choice = read_choice(1, 3);
    switch (choice) {
        case 1:
            strncpy(cfg->llm.provider, "anthropic",
                    sizeof(cfg->llm.provider) - 1);
            strncpy(cfg->llm.model, "claude-sonnet-4-5",
                    sizeof(cfg->llm.model) - 1);
            break;
        case 2:
            strncpy(cfg->llm.provider, "openai",
                    sizeof(cfg->llm.provider) - 1);
            strncpy(cfg->llm.model, "gpt-4o",
                    sizeof(cfg->llm.model) - 1);
            break;
        case 3:
            read_required(cfg->llm.provider, sizeof(cfg->llm.provider),
                          "Provider name: ");
            break;
    }
}

static void step_api_key(mimi_config_t *cfg) {
    printf("\nStep 2: API Key\n");
    read_required(cfg->llm.api_key, sizeof(cfg->llm.api_key),
                  "Enter API key: ");
}

static void step_model(mimi_config_t *cfg) {
    printf("\nStep 3: Model\n");
    if (strcmp(cfg->llm.provider, "anthropic") == 0) {
        printf("  1) claude-sonnet-4-5 (recommended)\n");
        printf("  2) claude-opus-4-5\n");
        printf("  3) claude-haiku-3-5\n");
        printf("  4) Custom\n");
        printf("  Choice [1]: ");
        fflush(stdout);
        int choice = read_choice(1, 4);
        switch (choice) {
            case 1:
                strncpy(cfg->llm.model, "claude-sonnet-4-5",
                        sizeof(cfg->llm.model) - 1);
                break;
            case 2:
                strncpy(cfg->llm.model, "claude-opus-4-5",
                        sizeof(cfg->llm.model) - 1);
                break;
            case 3:
                strncpy(cfg->llm.model, "claude-haiku-3-5",
                        sizeof(cfg->llm.model) - 1);
                break;
            case 4:
                read_required(cfg->llm.model, sizeof(cfg->llm.model),
                              "Model name: ");
                break;
        }
    } else if (strcmp(cfg->llm.provider, "openai") == 0) {
        printf("  1) gpt-4o (recommended)\n");
        printf("  2) gpt-4o-mini\n");
        printf("  3) gpt-4-turbo\n");
        printf("  4) Custom\n");
        printf("  Choice [1]: ");
        fflush(stdout);
        int choice = read_choice(1, 4);
        switch (choice) {
            case 1:
                strncpy(cfg->llm.model, "gpt-4o",
                        sizeof(cfg->llm.model) - 1);
                break;
            case 2:
                strncpy(cfg->llm.model, "gpt-4o-mini",
                        sizeof(cfg->llm.model) - 1);
                break;
            case 3:
                strncpy(cfg->llm.model, "gpt-4-turbo",
                        sizeof(cfg->llm.model) - 1);
                break;
            case 4:
                read_required(cfg->llm.model, sizeof(cfg->llm.model),
                              "Model name: ");
                break;
        }
    } else {
        read_required(cfg->llm.model, sizeof(cfg->llm.model),
                      "Model name: ");
    }
}

static void step_channels(mimi_config_t *cfg) {
    printf("\nStep 4: Channels\n");
    configure_channel(&cfg->telegram, "Telegram");
    configure_channel(&cfg->discord, "Discord");
    configure_channel(&cfg->slack, "Slack");
    configure_channel(&cfg->whatsapp, "WhatsApp");
}

static void print_summary(const mimi_config_t *cfg) {
    printf("\n");
    printf("  Configuration Summary\n");
    printf("  ---------------------\n");
    printf("  Provider:  %s\n", cfg->llm.provider);
    printf("  Model:     %s\n", cfg->llm.model);
    printf("  API Key:   %s\n",
           cfg->llm.api_key[0] ? "***configured***" : "(none)");
    printf("  Channels:\n");
    printf("    Telegram:  %s\n",
           cfg->telegram.enabled ? "enabled" : "disabled");
    printf("    Discord:   %s\n",
           cfg->discord.enabled ? "enabled" : "disabled");
    printf("    Slack:     %s\n",
           cfg->slack.enabled ? "enabled" : "disabled");
    printf("    WhatsApp:  %s\n",
           cfg->whatsapp.enabled ? "enabled" : "disabled");
    printf("  WebSocket: port %d\n", cfg->websocket.port);
    printf("  Admin:     port %d\n", cfg->admin.port);
    printf("  Timezone:  %s\n", cfg->timezone);
    printf("\n");
}

esp_err_t setup_wizard_run(void) {
    printf("\n");
    printf("  MimiClaw Setup Wizard\n");
    printf("  =====================\n");

    mimi_config_t cfg;
    config_defaults(&cfg);

    step_llm_provider(&cfg);
    step_api_key(&cfg);
    step_model(&cfg);
    step_channels(&cfg);

    printf("\nStep 5: Save & Start\n");

    esp_err_t err = config_set(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to set config: %s", esp_err_to_name(err));
        return err;
    }

    err = config_save();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to save config: %s", esp_err_to_name(err));
        return err;
    }

    printf("  Config saved to %s\n", config_exists() ? "" : "(unknown path)");

    const mimi_config_t *saved = config_get();
    print_summary(saved);

    printf("  Setup complete! Run 'mimiclaw start' to launch.\n\n");
    return ESP_OK;
}
