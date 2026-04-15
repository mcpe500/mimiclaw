#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <curl/curl.h>
#include "esp_log.h"
#include "esp_err.h"
#include "host/config/config.h"
#include "host/setup/setup_wizard.h"
#include "bus/message_bus.h"
#include "tools/tool_registry.h"
#include "memory/memory_store.h"
#include "memory/session_mgr.h"
#include "channels/telegram/telegram_bot.h"
#include "channels/discord/discord_bot.h"
#include "channels/slack/slack_bot.h"
#include "channels/whatsapp/whatsapp_bot.h"
#include "agent/agent_loop.h"

#define MIMI_BANNER \
    "\n  __  __ _       _ ____  _       _ \n" \
    " |  \\/  (_)_ __ (_)  _ \\(_)_ __ | |\n" \
    " | |\\/| | | '_ \\| | |_) | | '_ \\| |\n" \
    " | |  | | | | | | |  _ <| | | | | |\n" \
    " |_|  |_|_|_| |_|_|_| \\_\\_|_| |_|_|\n\n"

static const char *TAG = "mimi-core";
static volatile bool s_running = true;

static void signal_handler(int sig) {
    (void)sig;
    ESP_LOGI(TAG, "Shutting down...");
    s_running = false;
}

static void outbound_dispatch_task(void *arg) {
    (void)arg;
    ESP_LOGI(TAG, "Outbound dispatch started");
    while (s_running) {
        mimi_msg_t msg;
        memset(&msg, 0, sizeof(msg));
        esp_err_t err = message_bus_pop_outbound(&msg, 1000);
        if (err != 0) continue;
        if (!msg.content) continue;

        ESP_LOGI(TAG, "[OUT] %s/%s: %.80s", msg.channel, msg.chat_id, msg.content);

        if (strcmp(msg.channel, "telegram") == 0) {
            telegram_send_message(msg.chat_id, msg.content);
        } else if (strcmp(msg.channel, "discord") == 0) {
            discord_send_message(msg.chat_id, msg.content);
        } else if (strcmp(msg.channel, "slack") == 0) {
            slack_send_message(msg.chat_id, msg.content);
        } else if (strcmp(msg.channel, "whatsapp") == 0) {
            whatsapp_send_message(msg.chat_id, msg.content);
        } else {
            ESP_LOGW(TAG, "Unknown outbound channel: %s", msg.channel);
        }

        free(msg.content);
    }
    ESP_LOGI(TAG, "Outbound dispatch stopped");
}

int main(int argc, char **argv) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf(MIMI_BANNER);
    printf("  MimiClaw Core v0.2 -- Lightweight AI Agent\n\n");

    curl_global_init(CURL_GLOBAL_ALL);

    char config_path[256];
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(config_path, sizeof(config_path), "%s/.mimiclaw/config.json", home);

    esp_err_t err = config_init(config_path);
    if (err != 0) {
        ESP_LOGE(TAG, "Failed to init config: %d", err);
        return 1;
    }

    if (!config_exists()) {
        ESP_LOGI(TAG, "No configuration found. Starting setup wizard...\n");
        err = setup_wizard_run();
        if (err != 0) {
            ESP_LOGE(TAG, "Setup wizard failed");
            return 1;
        }
    } else {
        err = config_load();
        if (err != 0) {
            ESP_LOGE(TAG, "Failed to load config");
            return 1;
        }
    }

    const mimi_config_t *cfg = config_get();
    ESP_LOGI(TAG, "Config loaded from %s", config_path);

    const char *data_dir = config_data_dir();
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s/sessions %s/memory %s/config %s/skills",
             data_dir, data_dir, data_dir, data_dir);
    system(cmd);

    err = message_bus_init();
    if (err != 0) {
        ESP_LOGE(TAG, "Failed to init message bus");
        return 1;
    }
    ESP_LOGI(TAG, "Message bus initialized");

    err = tool_registry_init();
    if (err != 0) {
        ESP_LOGE(TAG, "Failed to init tool registry");
        return 1;
    }
    ESP_LOGI(TAG, "Tool registry initialized (%d tools)", 0);

    err = memory_store_init();
    if (err != 0) {
        ESP_LOGE(TAG, "Failed to init memory store");
        return 1;
    }
    ESP_LOGI(TAG, "Memory store initialized");

    err = session_mgr_init();
    if (err != 0) {
        ESP_LOGE(TAG, "Failed to init session manager");
        return 1;
    }
    ESP_LOGI(TAG, "Session manager initialized");

    ESP_LOGI(TAG, "Starting channels...");

    if (cfg->telegram.enabled && cfg->telegram.token[0]) {
        telegram_set_token(cfg->telegram.token);
        telegram_bot_init();
        telegram_bot_start();
        ESP_LOGI(TAG, "  [Telegram] started");
    }

    if (cfg->discord.enabled && cfg->discord.token[0]) {
        discord_set_token(cfg->discord.token);
        discord_bot_init();
        discord_bot_start();
        ESP_LOGI(TAG, "  [Discord] started");
    }

    if (cfg->slack.enabled) {
        slack_bot_init();
        slack_bot_start();
        ESP_LOGI(TAG, "  [Slack] started");
    }

    if (cfg->whatsapp.enabled) {
        whatsapp_bot_init();
        whatsapp_bot_start();
        ESP_LOGI(TAG, "  [WhatsApp] started");
    }

    err = agent_loop_init();
    if (err != 0) {
        ESP_LOGE(TAG, "Failed to init agent loop");
        return 1;
    }
    agent_loop_start();
    ESP_LOGI(TAG, "Agent loop started");

    pthread_t dispatch_thread;
    pthread_create(&dispatch_thread, NULL, (void *(*)(void *))outbound_dispatch_task, NULL);
    pthread_detach(dispatch_thread);

    ESP_LOGI(TAG, "\nMimiClaw ready. Press Ctrl+C to stop.\n");

    while (s_running) {
        usleep(100000);
    }

    ESP_LOGI(TAG, "Cleaning up...");
    message_bus_deinit();
    curl_global_cleanup();

    ESP_LOGI(TAG, "Goodbye!");
    return 0;
}
