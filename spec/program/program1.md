# mimiclaw-dev

Autonomous feature development loop. Goal: bring MimiClaw to OpenClaw parity while staying lightweight. Tests first, implement second.

## Dual-Target Architecture

MimiClaw compiles for **two targets**:
1. **ESP32** — Embedded firmware via ESP-IDF (`idf.py build`)
2. **Host** — Native C binary via gcc (`core/Makefile`)

Both targets share the same business logic in `main/`. Platform differences are resolved via **shim headers** in `core/shims/` (for host) and ESP-IDF components (for ESP32).

### Platform Abstraction Layer

| ESP-IDF API | Host Shim | Implementation |
|-------------|-----------|----------------|
| `esp_err.h` | `core/shims/esp_err.h` | `typedef int esp_err_t` |
| `esp_log.h` | `core/shims/esp_log.h` | `printf` macros |
| `esp_http_client.h` | `core/shims/esp_http_client.c` | libcurl wrapper |
| `freertos/queue.h` | `core/shims/freertos/queue.h` | pthread mutex + cond |
| `nvs.h` | `core/shims/nvs.c` | JSON file store |
| SPIFFS | Host filesystem | `./data/` directory |

### Build Commands

```bash
# ESP32 firmware
idf.py build

# Host binary
cd core && make

# Docker
cd core && docker-compose up -d

# Termux
bash scripts/build_termux.sh
```

## Development Server

| Field | Value |
|-------|-------|
| Host | `192.168.18.158` |
| Port | `8022` |
| Username | `u0_a262` |
| Password | `pass1234` |
| Working dir | `~/mimiclaw/` |

## Preflight

```bash
cd ~/mimiclaw
pwd && uname -a && whoami
git status --short --branch
command -v gcc || echo "gcc: MISSING"
command -v make || echo "make: MISSING"
command -v cppcheck || echo "cppcheck: MISSING"
```

### Tool availability

| Tool | Linux/Docker | Termux |
|------|-------------|--------|
| git, make, gcc, gcov | YES | YES |
| libcurl, sqlite3 | YES | YES |
| cppcheck | YES | MAYBE |
| Docker | YES | NO |
| ESP-IDF (idf.py) | YES | BLOCKED |

## Setup

1. `cd ~/mimiclaw && git status`
2. Tag: today's date (e.g. `apr15`)
3. Create branch: `git checkout -b dev/apr15`
4. Push: `git push -u origin dev/apr15`
5. Setup test deps: `cd test && make setup`
6. Verify: `cd test && make test`
7. Build host: `cd core && make setup && make`

## Feature Backlog (F1-F23) — ALL COMPLETE

### Tier 1 — Foundation ✅
**F1**: Provider abstraction — `main/llm/provider.h`, `provider_anthropic.c`, `provider_openai.c`
**F2**: Unified message schema — extended `mimi_msg_t` with user_id, media, metadata
**F3**: Channel abstraction — `main/channels/channel.h`, `channel_manager.c`
**F4**: Pairing + allowlist + group policy — `main/auth/pairing.h/c`

### Tier 2 — Core Features ✅
**F5**: Memory tools — `main/tools/tool_memory.h/c`
**F6**: File tools — `main/tools/tool_files.h/c`
**F7**: Session isolation — user_id in session path
**F8**: HTML formatting — `main/channels/telegram/tg_format.h/c`
**F9**: Bootstrap files — `spiffs/AGENTS.md`, `spiffs/TOOLS.md`
**F10**: Memory lookback — `MIMI_MEMORY_RECENT_DAYS=7`

### Tier 3 — Media ✅
**F11**: Telegram media — photo/document/voice/video in `telegram_bot.c`

### Tier 4 — Multi-channel ✅
**F12**: Discord — `main/channels/discord/discord_bot.c` (Gateway WS + REST)
**F13**: WhatsApp — `main/channels/whatsapp/whatsapp_bot.c` (Cloud API polling)
**F14**: Slack — `main/channels/slack/slack_bot.c` (Socket Mode WS + REST)

### Tier 5 — Platform ✅
**F15**: mimi-core — `core/main.c` + shims + host Makefile
**F16**: Docker — `core/Dockerfile` + `core/docker-compose.yml`
**F17**: Termux — `scripts/build_termux.sh` + `scripts/setup_termux.sh`
**F18**: Web admin — `main/admin/admin_server.c` + admin HTML
**F19**: WebSocket chat — `spiffs/chat/index.html`
**F20**: Plugin registry — dynamic tool registration
**F21**: Skill packaging — `main/skills/skill_manifest.c`
**F22**: Subtask/background — `main/agent/subtask.c`
**F23**: SQLite storage — `main/storage/db.c`

## Test Infrastructure

```bash
cd ~/mimiclaw/test
make setup      # Download Unity + cJSON
make test       # Run all unit tests
make analyze    # cppcheck static analysis
make asan       # AddressSanitizer
make coverage   # Line coverage >80%
make full       # All gates
```

### Test Files

| Test | Feature | Tests |
|------|---------|-------|
| test_F1_provider.c | Provider abstraction | 15 |
| test_F2_message.c | Message schema | 10 |
| test_F3_channel.c | Channel abstraction | 8 |
| test_F4_pairing.c | Auth/pairing | 20 |
| test_F5_memory.c | Memory tools | 8 |
| test_F7_session.c | Session isolation | 7 |
| test_F8_tg_format.c | HTML formatting | 8 |
| test_F11_tg_media.c | Telegram media | 15 |
| test_F12_discord.c | Discord channel | 20 |
| test_F13_whatsapp.c | WhatsApp channel | 15 |
| test_F14_slack.c | Slack channel | 15 |
| test_F18_admin.c | Admin server | 12 |
| test_F20_plugin.c | Plugin registry | 15 |
| test_F21_skill_pack.c | Skill packaging | 12 |
| test_F22_subtask.c | Subtask system | 15 |
| test_F23_sqlite.c | SQLite storage | 12 |

## Development Loop

```
LOOP for each feature:
  0. Preflight: git status, verify tools
  1. Pick next unfinished feature
  2. Read spec + source files
  3. git pull && git checkout dev/<tag>
  4. Write tests FIRST (must FAIL)
  5. ANALYZE: make analyze → 0 warnings
  6. Implement in main/
  7. ASAN: make asan → 0 errors
  8. COVERAGE: make coverage → >80%
  9. TEST: make test → all pass
  10. REGRESSION: make full → all pass
  11. Commit + push
```

## Timeout

Per feature: ~10-30 min. If >2 hours, mark `crash` or `blocked` and move on.

**NEVER STOP** once loop begins.