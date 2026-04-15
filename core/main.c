#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "mimi-core";
static bool s_running = true;

static void signal_handler(int sig) {
    (void)sig;
    ESP_LOGI(TAG, "Shutting down...");
    s_running = false;
}

int main(int argc, char **argv) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    ESP_LOGI(TAG, "MimiClaw Core v0.1 starting...");

    // 1. Init NVS (file-based on host)
    nvs_flash_init();

    // 2. Init message bus
    // message_bus_init(); // TODO: implement host queue

    // 3. Init tools
    // tool_registry_init(); // TODO

    // 4. Init memory
    // memory_store_init(); // TODO

    // 5. Init channels based on config
    // telegram_bot_init(); // TODO
    // discord_bot_init(); // TODO
    // slack_bot_init(); // TODO
    // whatsapp_bot_init(); // TODO
    // ws_server_start(); // TODO

    // 6. Start agent loop
    // agent_loop_init(); // TODO

    ESP_LOGI(TAG, "All modules initialized. Running...");

    while (s_running) {
        // Main event loop placeholder
        usleep(100000); // 100ms
    }

    ESP_LOGI(TAG, "Goodbye!");
    return 0;
}
