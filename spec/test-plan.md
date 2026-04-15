# MimiClaw Test Plan

## 1. Overview

This document defines the complete testing strategy for MimiClaw across all 23 features, both compilation targets (ESP32 + host), and all quality gates.

## 2. Test Categories

| Category | Purpose | Automation | Target |
|----------|---------|------------|--------|
| Unit Tests | Individual function/struct validation | `make test` | Host |
| Static Analysis | Code quality / bug detection | `make analyze` | Host |
| AddressSanitizer | Memory safety | `make asan` | Host |
| Coverage | Line coverage ≥80% | `make coverage` | Host |
| Integration | Multi-module interaction | Manual / scripted | Host + ESP32 |
| End-to-End | Full message flow through channels | Manual | Host + ESP32 |
| Platform | Target-specific compilation | Build check | ESP32 / Docker / Termux |
| Visual/UI | HTML interface correctness | Manual browser | Host |

## 3. Phase 1: Infrastructure Validation

Before running any feature tests, verify the test infrastructure itself.

### 3.1 Setup

```bash
cd ~/mimiclaw/test
make setup
```

**Verify:**
- [ ] `vendor/unity/unity.h` exists
- [ ] `vendor/unity/unity.c` exists
- [ ] `vendor/cJSON/cJSON.h` exists
- [ ] `vendor/cJSON/cJSON.c` exists
- [ ] No download errors

### 3.2 Shim Compilation

```bash
make clean
make test 2>&1 | head -50
```

**Verify:**
- [ ] All 16 test files compile without errors
- [ ] `shims/esp_err.h` resolves `esp_err_t`, `ESP_OK`, etc.
- [ ] `shims/esp_log.h` resolves `ESP_LOGI`, etc.
- [ ] `shims/esp_http_client.h` compiles
- [ ] No undefined symbol errors at link time

### 3.3 Unity Framework

**Verify:**
- [ ] `UNITY_BEGIN()` returns
- [ ] `UNITY_END()` prints summary
- [ ] `RUN_TEST()` executes test function
- [ ] Assert macros work: `TEST_ASSERT_EQUAL`, `TEST_ASSERT_TRUE`, `TEST_ASSERT_NULL`, `TEST_ASSERT_NOT_NULL`

## 4. Phase 2: Unit Test Matrix

### F1: Provider Abstraction

**File**: `test/unit/test_F1_provider.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1 | `test_provider_struct_exists` | `llm_provider_t` compiles |
| 2 | `test_provider_vtable_exists` | Function pointers in vtable are non-null after init |
| 3 | `test_anthropic_create` | `provider_anthropic_create()` returns valid provider |
| 4 | `test_anthropic_name` | Provider name is "anthropic" |
| 5 | `test_openai_create` | `provider_openai_create()` returns valid provider |
| 6 | `test_openai_name` | Provider name is "openai" |
| 7 | `test_anthropic_chat` | `chat()` function pointer is callable |
| 8 | `test_openai_chat` | `chat()` function pointer is callable |
| 9 | `test_anthropic_free` | `free()` doesn't crash |
| 10 | `test_openai_free` | `free()` doesn't crash |
| 11 | `test_provider_null_free` | Freeing NULL doesn't crash |
| 12 | `test_anthropic_model_default` | Default model matches config |
| 13 | `test_openai_model_default` | Default model matches config |
| 14 | `test_anthropic_set_model` | Model string can be changed |
| 15 | `test_openai_set_model` | Model string can be changed |

### F2: Unified Message Schema

**File**: `test/unit/test_F2_message.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1 | `test_msg_struct_init` | `mimi_msg_t` zero-initializes correctly |
| 2 | `test_msg_channel_field` | channel[16] field exists and writable |
| 3 | `test_msg_chat_id_field` | chat_id[96] field exists and writable |
| 4 | `test_msg_content_field` | content pointer field exists |
| 5 | `test_msg_user_id_field` | user_id[64] field (F2 addition) |
| 6 | `test_msg_message_id_field` | message_id[64] field |
| 7 | `test_msg_media_count` | media_count field |
| 8 | `test_msg_media_paths` | media_paths[4][64] field |
| 9 | `test_msg_reply_to` | reply_to[64] field |
| 10 | `test_msg_metadata` | metadata[256] field |

### F3: Channel Abstraction

**File**: `test/unit/test_F3_channel.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1 | `test_channel_struct_exists` | `channel_t` compiles |
| 2 | `test_channel_vtable_exists` | `channel_vtable_t` has all function pointers |
| 3 | `test_channel_caps_struct` | `channel_caps_t` boolean fields |
| 4 | `test_channel_telegram_create` | Telegram channel has correct name + caps |
| 5 | `test_channel_feishu_create` | Feishu channel has correct name + caps |
| 6 | `test_channel_websocket_create` | WebSocket channel has correct name + caps |
| 7 | `test_channel_get_returns_null_for_unknown` | Unknown channel returns NULL |
| 8 | `test_channel_manager_max_channels` | Can register up to max channels |

### F4: Pairing + Allowlist

**File**: `test/unit/test_F4_pairing.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1 | `test_auth_struct_exists` | `auth_config_t` compiles |
| 2 | `test_dm_policy_enum` | DM_POLICY_* values correct |
| 3 | `test_group_policy_enum` | GROUP_POLICY_* values correct |
| 4 | `test_auth_init` | Init returns ESP_OK |
| 5 | `test_auth_check_dm_open_policy` | OPEN policy allows all |
| 6 | `test_auth_check_dm_disabled_policy` | DISABLED policy blocks all |
| 7 | `test_auth_check_dm_allowlist_policy` | ALLOWLIST allows listed, blocks others |
| 8 | `test_auth_check_dm_pairing_policy` | PAIRING allows paired, blocks others |
| 9 | `test_auth_check_group_open_policy` | Open group allows all |
| 10 | `test_auth_check_group_allowlist_policy` | Group allowlist filtering |
| 11 | `test_auth_check_group_mention_only_policy` | Mention-only requires mention=true |
| 12 | `test_auth_set_allowlist` | Add to allowlist increments count |
| 13 | `test_auth_remove_allowlist` | Remove from allowlist decrements count |
| 14 | `test_auth_remove_allowlist_not_found` | Remove nonexistent returns error |
| 15 | `test_auth_pair_user` | Pair user, verify is_paired |
| 16 | `test_auth_is_paired_unknown_user` | Unknown user returns false |
| 17 | `test_auth_rate_limit_default_allows` | Rate limit allows first request |
| 18 | `test_auth_set_dm_policy` | Policy setter works |
| 19 | `test_auth_set_group_policy` | Policy setter works |
| 20 | `test_auth_get_config` | Config pointer valid |

### F5: Memory Tools

**File**: `test/unit/test_F5_memory.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1 | `test_tool_memory_init` | Init returns ESP_OK |
| 2 | `test_tool_memory_struct_exists` | `mimi_tool_t` compiles |
| 3 | `test_tool_memory_read_signature` | Read with NULL returns error |
| 4 | `test_tool_memory_write_invalid_input` | Write with NULL/empty returns error |
| 5 | `test_tool_memory_append_invalid_input` | Append with NULL/empty returns error |
| 6 | `test_tool_memory_recall_default_days` | Recall with NULL input works |
| 7 | `test_tool_memory_recall_with_days` | Recall with days param works |
| 8 | `test_tool_memory_read_with_purpose` | Read with purpose param works |

### F7: Session Isolation

**File**: `test/unit/test_F7_session.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1 | `test_session_init` | Init returns ESP_OK |
| 2 | `test_session_append_signature` | Append with user_id works |
| 3 | `test_session_get_history_json_signature` | Get history with user_id works |
| 4 | `test_session_clear_signature` | Clear with user_id works |
| 5 | `test_session_clear_not_found` | Clear nonexistent returns error |
| 6 | `test_session_isolation_different_users` | Two users in same chat have separate sessions |
| 7 | `test_session_list_does_not_crash` | session_list() doesn't crash |

### F8: HTML Formatting

**File**: `test/unit/test_F8_tg_format.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1 | `test_tg_format_html_null` | NULL input returns NULL |
| 2 | `test_tg_format_html_basic` | Basic text wrapped in `<pre>` tags |
| 3 | `test_tg_format_html_escapes_amp` | `&` becomes `&amp;` |
| 4 | `test_tg_format_html_escapes_brackets` | `<` and `>` escaped |
| 5 | `test_tg_format_markdownv2_null` | NULL input returns NULL |
| 6 | `test_tg_format_markdownv2_basic` | Basic text preserved |
| 7 | `test_tg_format_markdownv2_escapes_brackets` | `[` and `]` escaped with backslash |
| 8 | `test_tg_format_free_null_safe` | Free NULL doesn't crash |

### F11: Telegram Media

**File**: `test/unit/test_F11_tg_media.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1 | Photo array parsing (highest res) | Last element selected |
| 2 | Document parsing | File name and ID extracted |
| 3 | Voice message parsing | File ID extracted, content="[voice]" |
| 4 | Video message parsing | File ID extracted, content="[video]" |
| 5 | File download URL construction | `getFile` → download URL format |
| 6 | Photo send payload | JSON has chat_id, photo, caption |
| 7 | media_count=1 for single media | Correct count |
| 8 | media_paths populated | File ID stored in paths[0] |
| 9 | Caption extraction | Photo caption read from update |
| 10 | Highest res photo selection | Largest photo_size array element |
| 11 | NULL input for download | Returns ESP_ERR_INVALID_ARG |
| 12 | NULL input for send_photo | Returns ESP_ERR_INVALID_ARG |
| 13 | Media without text | Non-text message still creates msg |
| 14 | Document without file_name | Falls back to "unknown" |
| 15 | Mixed update with text+photo | Text handled first, photo handled separately |

### F12: Discord Channel

**File**: `test/unit/test_F12_discord.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1-5 | Struct validation | discord_bot types compile |
| 6-10 | FNV-1a hash | Consistent hash output |
| 11-13 | Gateway event parsing | Hello, MESSAGE_CREATE, READY parsed correctly |
| 14-16 | Heartbeat/identify payload | Correct opcodes and structure |
| 17-19 | REST path construction | URL format for channel messages |
| 20 | Channel name "discord" | Correct constant |

### F13: WhatsApp Channel

**File**: `test/unit/test_F13_whatsapp.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1 | Channel constant | MIMI_CHAN_WHATSAPP = "whatsapp" |
| 2-3 | Dedup empty/insert | Ring buffer works |
| 4 | Dedup ring wrap | Circular buffer wraps correctly |
| 5-7 | Incoming JSON parsing | Message body, phone, ID extracted |
| 8 | Multi-entry parsing | Multiple entries in response |
| 9 | Dedup skip | Duplicate message skipped |
| 10 | Non-text skip | Non-text message type ignored |
| 11-12 | Empty/invalid JSON | Returns gracefully |
| 13 | Send payload structure | Correct JSON fields |
| 14 | Phone number extraction | From field parsed |
| 15 | Credential struct | Access token + phone number ID |

### F14: Slack Channel

**File**: `test/unit/test_F14_slack.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1-3 | Struct validation | slack_bot types compile |
| 4-5 | FNV-1a hash | Consistent hashing |
| 6-8 | Event parsing | Valid/invalid/missing fields |
| 9 | Subtype filtering | Bot messages with subtype skipped |
| 10-11 | Channel/user extraction | IDs parsed correctly |
| 12 | Ack JSON generation | Correct envelope_id in ack |
| 13 | Disconnect detection | {"type":"disconnect"} recognized |
| 14 | Hello detection | {"type":"hello"} recognized |
| 15 | Channel name "slack" | Correct constant |

### F18: Admin Server

**File**: `test/unit/test_F18_admin.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1-4 | Server lifecycle | Init/start/stop |
| 5-8 | API endpoint format | /api/status, /api/config response structure |
| 9-10 | Session listing | /api/sessions returns list |
| 11-12 | Config update | POST /api/config persists |

### F20: Plugin Registry

**File**: `test/unit/test_F20_plugin.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1-4 | External registration | Register tool outside built-in set |
| 5-8 | Lookup by name | External tools findable |
| 9-11 | Max capacity (16) | 17th registration fails |
| 12-13 | Unregister | Remove works, lookup fails after |
| 14-15 | Re-register | Can add back after remove |

### F21: Skill Packaging

**File**: `test/unit/test_F21_skill_pack.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1-3 | Valid manifest parsing | name, version, description extracted |
| 4-5 | Missing fields | Graceful handling |
| 6 | Empty JSON | Returns error |
| 7-8 | tools_required array | Parsed correctly |
| 9-10 | dependencies array | Parsed correctly |
| 11 | version string | Extracted |
| 12 | Validate missing name | Returns ESP_ERR_INVALID_ARG |

### F22: Subtask System

**File**: `test/unit/test_F22_subtask.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1 | Init | Returns ESP_OK |
| 2 | Submit | Returns job_id |
| 3 | Get status | Returns PENDING initially |
| 4 | Set result | Status → COMPLETED |
| 5 | Set error | Status → FAILED |
| 6 | Cancel | Status → CANCELLED |
| 7 | Active count | Correct number |
| 8 | Lifecycle PENDING→RUNNING→COMPLETED | Full state machine |
| 9 | Cancel from PENDING | Works |
| 10 | Max capacity (32) | 33rd submit fails |
| 11 | Double cancel | Second returns error |
| 12 | Get status nonexistent | Returns error |
| 13 | Set result for running | Works |
| 14 | Progress update | Progress field updated |
| 15 | Deinit | Cleanup doesn't crash |

### F23: SQLite Storage

**File**: `test/unit/test_F23_sqlite.c`

| # | Test | What it validates |
|---|------|-------------------|
| 1 | Init with :memory: | In-memory DB works |
| 2 | Create table | CREATE TABLE succeeds |
| 3 | Insert row | INSERT succeeds |
| 4 | Select row | SELECT returns correct data |
| 5 | Close | Cleanup succeeds |
| 6 | Schema creation | Default tables created |
| 7 | Error on invalid SQL | Returns ESP_FAIL |
| 8 | query_one returns value | Single value extraction |
| 9 | Empty result | Returns ESP_ERR_NOT_FOUND |
| 10 | Multiple inserts | All stored correctly |
| 11 | Update row | UPDATE modifies data |
| 12 | Delete row | DELETE removes data |

## 5. Phase 3: Quality Gates

### 5.1 Static Analysis

```bash
cd test && make analyze
```

- [ ] cppcheck exits with code 0
- [ ] 0 warnings on `main/` source files
- [ ] 0 warnings on `test/` source files

### 5.2 AddressSanitizer

```bash
cd test && make asan
```

- [ ] All 16 test binaries run with ASAN
- [ ] 0 heap buffer overflows
- [ ] 0 use-after-free
- [ ] 0 memory leaks
- [ ] 0 stack buffer overflows

### 5.3 Coverage

```bash
cd test && make coverage
```

- [ ] Line coverage ≥80% across all source files
- [ ] Branch coverage ≥70%
- [ ] No 0%-covered functions in core modules (auth, session, tools)

### 5.4 Full Regression

```bash
cd test && make full
```

- [ ] All 4 gates pass: test + analyze + asan + coverage
- [ ] Exit code 0

## 6. Phase 4: Integration Tests

### 6.1 Channel Integration

Test that channels correctly push messages to the bus and receive responses.

| Test | Steps | Expected |
|------|-------|----------|
| TG receive → bus | Mock TG update, verify bus message | `msg.channel == "telegram"` |
| TG send → API | Call `telegram_send_message()`, verify HTTP call | POST to `sendMessage` |
| Discord GW → bus | Mock MESSAGE_CREATE, verify bus message | `msg.channel == "discord"` |
| Discord send → REST | Call `discord_send_message()`, verify HTTP call | POST to `/messages` |
| Slack event → bus | Mock events_api message, verify bus message | `msg.channel == "slack"` |
| Slack send → REST | Call `slack_send_message()`, verify HTTP call | POST to `chat.postMessage` |
| WhatsApp poll → bus | Mock Cloud API response, verify bus message | `msg.channel == "whatsapp"` |
| WhatsApp send → REST | Call `whatsapp_send_message()`, verify HTTP call | POST to `/messages` |

### 6.2 Agent Loop Integration

| Test | Steps | Expected |
|------|-------|----------|
| Message → session | Push message, verify session file created | File at sessions/tg_{id}_u_{uid}.jsonl |
| Message → auth check | Push with DM_POLICY_PAIRING, unpaired user | Message dropped |
| Message → LLM → response | Mock LLM response, verify outbound bus | Response in outbound queue |
| Tool call execution | LLM returns tool_use, verify tool_registry_execute called | Tool output captured |
| Session isolation | Two users in same chat, verify separate sessions | No cross-contamination |

### 6.3 Auth Flow Integration

| Test | Steps | Expected |
|------|-------|----------|
| Pairing flow | New user sends /start → bot replies with pair prompt → user confirms → auth_pair_user called | User can now message |
| Allowlist flow | Admin adds user_id → user can send DMs | Non-listed users blocked |
| Rate limiting | Send 61 messages in 1 minute (limit=60) | 61st rejected |
| Group mention | Send message in group without @bot → no response | MENTION_ONLY works |

### 6.4 Memory Integration

| Test | Steps | Expected |
|------|-------|----------|
| Write → read | Call memory_write with content → memory_read returns same content | Content matches |
| Daily append | Append note → verify today's file has note | File exists with content |
| Recent recall | Append to 3 daily files → recall(7) returns all | 3 entries returned |
| Memory tool chain | LLM calls memory_write → verify MEMORY.md updated | File modified |

## 7. Phase 5: Platform Tests

### 7.1 ESP32 Target

```bash
idf.py build
```

- [ ] Compiles without errors
- [ ] Binary size fits partition (check `size` report)
- [ ] Flash succeeds: `idf.py flash`
- [ ] Serial monitor shows init logs: `idf.py monitor`

### 7.2 Host Target

```bash
cd core && make setup && make
./mimiclaw
```

- [ ] Compiles with gcc on Linux
- [ ] Compiles with gcc on macOS
- [ ] Binary runs, prints startup logs
- [ ] Signal handling works (Ctrl+C graceful shutdown)
- [ ] Data directory created

### 7.3 Docker Target

```bash
cd core && docker-compose build
docker-compose up -d
docker-compose logs
```

- [ ] Docker build succeeds (multi-stage)
- [ ] Container starts
- [ ] Logs show startup
- [ ] Port 18789 accessible
- [ ] Volume mount persists data across restarts
- [ ] `docker-compose down` stops cleanly

### 7.4 Termux Target

```bash
bash scripts/setup_termux.sh
bash scripts/build_termux.sh
cd core && ./mimiclaw
```

- [ ] Setup installs all deps (gcc, make, curl, sqlite, libcurl)
- [ ] Build completes without errors
- [ ] Binary runs on ARM64 Termux
- [ ] Test suite runs: `cd test && make test`

## 8. Phase 6: Visual / Manual Tests

### 8.1 WebSocket Chat UI (F19)

1. Open `spiffs/chat/index.html` in browser
2. Connect to `ws://<device_ip>:18789/`
3. Type message, press Enter
4. Verify response appears in chat area

- [ ] Dark theme renders correctly
- [ ] Connection status dot shows green when connected
- [ ] Messages auto-scroll to bottom
- [ ] Mobile layout works (narrow viewport)
- [ ] Disconnect shows red dot

### 8.2 Web Admin UI (F18)

1. Open `http://<device_ip>:8080/` in browser
2. Check status page loads
3. Navigate to config page
4. Update a config value
5. Verify persistence

- [ ] Dashboard shows device status
- [ ] Config form loads current values
- [ ] Save button persists changes
- [ ] Session list shows active sessions
- [ ] Clear session button works

### 8.3 Onboarding Portal

1. Connect to MimiClaw-XXXX WiFi AP
2. Open `http://192.168.4.1/`
3. Scan WiFi networks
4. Enter SSID + password
5. Enter LLM API key
6. Save & restart

- [ ] WiFi scan returns AP list
- [ ] All config sections expandable
- [ ] Save triggers device restart
- [ ] Device connects to configured WiFi

## 9. Phase 7: End-to-End Scenarios

### 9.1 Telegram Full Flow

1. Device boots → WiFi connects → TG bot starts polling
2. User sends "Hello" → received via getUpdates → pushed to inbound bus
3. Auth check (DM_POLICY_OPEN) → passes
4. Agent loop picks up message → loads session history → builds system prompt
5. LLM API call → response text "Hi there!"
6. Session saved: session_append(chat_id, user_id, "user", "Hello") + session_append(chat_id, user_id, "assistant", "Hi there!")
7. Response pushed to outbound bus → telegram_send_message() called
8. User receives "Hi there!" in Telegram

- [ ] Full round-trip completes in <5 seconds
- [ ] Session file created with 2 entries
- [ ] Next message includes previous context

### 9.2 Discord Full Flow

1. Discord bot connects to Gateway → receives Hello → sends Identify
2. Heartbeat loop starts at received interval
3. User sends message in #general → MESSAGE_CREATE received
4. Message pushed to inbound bus → agent processes → LLM responds
5. discord_send_message(channel_id, response) → REST POST
6. Response appears in Discord channel

- [ ] Gateway stays connected (heartbeat maintained)
- [ ] Reconnect on disconnect
- [ ] Bot responds within 10 seconds

### 9.3 Multi-Channel Simultaneous

1. Start Telegram + Discord + Slack + WebSocket simultaneously
2. Send message from each channel
3. Verify each channel receives its own response (no cross-channel leakage)

- [ ] Sessions isolated per channel
- [ ] No message routing errors
- [ ] All channels respond independently

## 10. Test Execution Order

```
Phase 1: Infrastructure (5 min)
  └── make setup
  └── make test (smoke: just compile, don't need all pass)

Phase 2: Unit Tests (10 min)
  └── make test (all 207 tests must pass)

Phase 3: Quality Gates (15 min)
  └── make analyze
  └── make asan
  └── make coverage

Phase 4: Integration (30 min, manual)
  └── Channel mock tests
  └── Auth flow tests
  └── Memory chain tests

Phase 5: Platform (20 min per target)
  └── ESP32: idf.py build flash monitor
  └── Host: cd core && make && ./mimiclaw
  └── Docker: docker-compose up
  └── Termux: bash scripts/setup_termux.sh

Phase 6: Visual (15 min)
  └── Chat UI in browser
  └── Admin UI in browser
  └── Onboarding portal

Phase 7: End-to-End (30 min per channel)
  └── Telegram full flow
  └── Discord full flow
  └── Multi-channel simultaneous
```

**Total estimated time: 2.5 hours**

## 11. Pass/Fail Criteria

| Gate | Criteria |
|------|----------|
| Unit Tests | 207/207 pass |
| Static Analysis | 0 warnings |
| ASAN | 0 errors |
| Coverage | ≥80% line coverage |
| Integration | All mock tests pass |
| ESP32 Build | Compiles + flashes |
| Host Build | Compiles + runs |
| Docker Build | Image builds + starts |
| Termux Build | Compiles + runs |
| Chat UI | Messages send/receive |
| Admin UI | Config read/write |
| E2E Telegram | Full round-trip <5s |
| E2E Discord | Full round-trip <10s |
| E2E Multi-channel | No cross-channel leakage |

## 12. Results Tracking

Use `results.tsv` format per the development loop spec:

```
commit	feature	tests_total	tests_pass	static_warns	asan_errors	coverage_pct	status	description
abc1234	F1-F10	43	43	0	0	85%	PASS	All foundation features
def5678	F11-F23	207	207	0	0	82%	PASS	All remaining features + infra
```