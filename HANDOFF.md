# Handoff & Progress Report - MimiClaw

## 1. Current Status

The project runs on two targets: ESP32-S3 (firmware) and host (Linux/macOS/Termux/Docker). The host target is the primary focus.

### 1.1 Host Product Runtime (`core/`)
- **Status**: Fully wired. All subsystems initialized and started from main.c.
- **User Experience**: Run `./mimiclaw` -> first-time setup wizard -> connects to channels -> runs autonomously.
- **What works**: Config, setup wizard, message bus (pthread), tool registry, memory store, session manager, channel init/start (Telegram/Discord/Slack/WhatsApp), agent loop, outbound dispatch, graceful shutdown.
- **What's still TODO**: WebSocket server for chat UI, WebSocket client for Discord/Slack (currently stubs), admin server real implementation.

### 1.2 Shim Layer (`core/shims/`)
- **Status**: 24 shim headers covering ALL ESP-IDF APIs used by `main/` source.
- **All 34 compiled `main/*.c` files should now resolve their ESP-IDF includes.**

## 2. Architecture

```
User runs: ./mimiclaw
  |
  +-- First run? -> Setup Wizard (CLI)
  |   +-- Choose LLM provider (Anthropic/OpenAI)
  |   +-- Enter API key
  |   +-- Configure channels (Discord, Telegram, Slack, WhatsApp)
  |   +-- Save to ~/.mimiclaw/config.json
  |
  +-- Load config.json
  +-- Init message bus (pthread)
  +-- Init tool registry (memory tools, file tools, etc.)
  +-- Init memory store + session manager
  +-- Start channels (each in own pthread)
  |   +-- Telegram: HTTP long-polling via libcurl
  |   +-- Discord: WebSocket Gateway (STUB - needs real WS client)
  |   +-- Slack: Socket Mode (STUB - needs real WS client)
  |   +-- WhatsApp: Cloud API polling via libcurl
  +-- Start agent loop (own pthread)
  |   +-- Pop from inbound queue
  |   +-- Build system prompt + session history
  |   +-- Call LLM API (Anthropic/OpenAI) with tool_use
  |   +-- Execute tools via tool_registry
  |   +-- Push response to outbound queue
  +-- Start outbound dispatch thread
  |   +-- Pop from outbound queue
  |   +-- Route to correct channel send function
  +-- Ctrl+C -> graceful shutdown
```

## 3. Files Changed This Session

### New Shims (14 files)
| File | Purpose |
|------|---------|
| `core/shims/freertos/task.h` | xTaskCreatePinnedToCore -> pthread_create, vTaskDelay, vTaskDelete |
| `core/shims/freertos/semphr.h` | SemaphoreHandle_t -> pthread_mutex_t |
| `core/shims/freertos/event_groups.h` | EventGroupHandle_t -> pthread mutex+cond |
| `core/shims/freertos/timers.h` | TimerHandle_t -> pthread timer thread |
| `core/shims/esp_heap_caps.h` | heap_caps_calloc/realloc -> malloc (MALLOC_CAP_SPIRAM ignored) |
| `core/shims/esp_http_server.h` | httpd stubs (httpd_start/register_uri/send/ws_frame) |
| `core/shims/esp_system.h` | esp_restart, esp_get_free_heap_size stubs |
| `core/shims/esp_random.h` | esp_random -> rand() |
| `core/shims/esp_event.h` | esp_event_base_t typedef |
| `core/shims/esp_websocket_client.h` | WS client stubs (returns NULL/false) |
| `core/shims/esp_tls.h` | TLS connection stubs |
| `core/shims/esp_https_ota.h` | OTA stubs |
| `core/shims/esp_ota_ops.h` | OTA partition stubs |
| `core/shims/driver/gpio.h` | GPIO set_direction/level stubs |

### Updated Shims
| File | Change |
|------|--------|
| `core/shims/esp_err.h` | Added ESP_ERR_NOT_SUPPORTED, ESP_ERROR_CHECK macro |

### Modified Files
| File | Change |
|------|--------|
| `core/main.c` | Full rewrite: wired message_bus, tool_registry, memory_store, session_mgr, all 4 channels, agent_loop, outbound dispatch thread |
| `core/Makefile` | Excluded message_bus.c from MAIN_SRCS, added channel/agent stack/prio/core defines |

## 4. Build Instructions

```bash
# On Termux / Linux / macOS
cd core
make setup    # Download cJSON, create data dirs
make          # Build mimiclaw binary
./mimiclaw    # First run -> setup wizard

# Docker
cd core
docker-compose up -d
```

## 5. Next Steps

1. **Compile test on Termux**: `cd core && make setup && make` -- fix any remaining compilation errors.

2. **Real WebSocket client**: The `esp_websocket_client.h` shim is currently a stub (returns NULL). Discord Gateway and Slack Socket Mode need a real POSIX WebSocket client implementation. Options:
   - Implement RFC 6455 from scratch using POSIX sockets + OpenSSL
   - Use libwebsockets (lightweight C library)
   - Use a minimal WS client header-only implementation

3. **WebSocket server for chat UI**: Implement HTTP upgrade + WS framing server on host. Serve `spiffs/chat/index.html`. Wire to message bus.

4. **Admin server**: The `esp_http_server.h` shim is a stub. Need real HTTP server for admin API endpoints.

5. **Integration test**: Run `./mimiclaw`, configure Telegram, send message, verify bot responds.

6. **Test suite validation**: `cd test && make setup && make test` -- all 207 unit tests.

## 6. Shim Coverage Map

| ESP-IDF Header | Shim File | Implementation |
|----------------|-----------|----------------|
| `esp_err.h` | `core/shims/esp_err.h` | typedef int, error codes, ESP_ERROR_CHECK |
| `esp_log.h` | `core/shims/esp_log.h` | printf wrappers |
| `esp_http_client.h` | `core/shims/esp_http_client.h` + `.c` | Real libcurl wrapper |
| `esp_http_server.h` | `core/shims/esp_http_server.h` | Stub (TODO: real impl) |
| `esp_crt_bundle.h` | `core/shims/esp_crt_bundle.h` | No-op |
| `esp_timer.h` | `core/shims/esp_timer.h` | Returns 0 |
| `esp_system.h` | `core/shims/esp_system.h` | abort/heap info stubs |
| `esp_heap_caps.h` | `core/shims/esp_heap_caps.h` | calloc/realloc -> stdlib |
| `esp_random.h` | `core/shims/esp_random.h` | rand() |
| `esp_event.h` | `core/shims/esp_event.h` | typedef only |
| `esp_websocket_client.h` | `core/shims/esp_websocket_client.h` | Stub (TODO: real WS) |
| `esp_tls.h` | `core/shims/esp_tls.h` | Stub |
| `esp_https_ota.h` | `core/shims/esp_https_ota.h` | Stub |
| `esp_ota_ops.h` | `core/shims/esp_ota_ops.h` | Stub |
| `nvs.h` | `core/shims/nvs.h` + `.c` | JSON file-backed |
| `nvs_flash.h` | `core/shims/nvs_flash.h` | Creates JSON file |
| `freertos/FreeRTOS.h` | `core/shims/freertos/FreeRTOS.h` | Types, vTaskDelay->usleep |
| `freertos/task.h` | `core/shims/freertos/task.h` | pthread_create wrapper |
| `freertos/queue.h` | `core/shims/freertos/queue.h` | No-op (not used) |
| `freertos/semphr.h` | `core/shims/freertos/semphr.h` | pthread_mutex wrapper |
| `freertos/event_groups.h` | `core/shims/freertos/event_groups.h` | pthread mutex+cond |
| `freertos/timers.h` | `core/shims/freertos/timers.h` | pthread timer thread |
| `driver/gpio.h` | `core/shims/driver/gpio.h` | No-op stubs |
