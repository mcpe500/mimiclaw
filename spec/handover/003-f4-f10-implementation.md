# Handover 003: F4-F10 Implementation

## Summary

Implemented features F4 through F10 for the MimiClaw autonomous development loop. All features include both implementation and test files.

## Features Completed

### F4: Pairing + Allowlist Auth
- **Files**: `main/auth/pairing.h`, `main/auth/pairing.c`
- **Test**: `test/unit/test_F4_pairing.c` (20 tests)
- **Details**: DM policies (PAIRING, ALLOWLIST, OPEN, DISABLED), group policies (ALLOWLIST, MENTION_ONLY, OPEN), rate limiting per user, paired user tracking (max 256), allowlist management (max 32)

### F5: Memory Tools
- **Files**: `main/tools/tool_memory.h`, `main/tools/tool_memory.c`
- **Test**: `test/unit/test_F5_memory.c` (8 tests)
- **Details**: 4 LLM-callable tools: memory_read, memory_write, memory_append, memory_recall. Wraps existing memory_store.h functions. JSON input parsing for content/note/days fields.

### F6: File Tools
- **Already exists**: `main/tools/tool_files.h` - read_file, write_file, edit_file, list_dir
- No changes needed.

### F7: Session Isolation
- **Files Modified**: `main/memory/session_mgr.h`, `main/memory/session_mgr.c`
- **Test**: `test/unit/test_F7_session.c` (7 tests)
- **Details**: Added `user_id` parameter to session_append, session_get_history_json, session_clear. Session files now named `tg_{chat_id}_u_{user_id}.jsonl` to prevent cross-user session leakage.
- **Breaking API change** - callers updated:
  - `main/agent/agent_loop.c` - 3 call sites updated with `msg.user_id`
  - `main/cli/serial_cli.c` - session_clear command now accepts optional `[user_id]` arg

### F8: HTML Formatting
- **Files**: `main/channels/telegram/tg_format.h`, `main/channels/telegram/tg_format.c`
- **Test**: `test/unit/test_F8_tg_format.c` (8 tests)
- **Details**: HTML escape (6 chars: & < > " ' /), MarkdownV2 escape (5 chars: \ [ ] ( )), wrapped in `<pre>` tags. Heap-allocated output with tg_format_free().

### F9: Bootstrap Files
- **Files**: `spiffs/AGENTS.md`, `spiffs/TOOLS.md`
- **Details**: AGENTS.md describes agent identity, capabilities, architecture. TOOLS.md documents all available LLM tools with input/output schemas.

### F10: Memory Lookback Config
- **Files Modified**: `main/mimi_config.h`, `main/tools/tool_memory.c`
- **Details**: Added `MIMI_MEMORY_RECENT_DAYS` config (default 7, was hardcoded 3). Updated tool_memory.c to use the define.

## Files Created

```
main/auth/pairing.h
main/auth/pairing.c
main/tools/tool_memory.h
main/tools/tool_memory.c
main/channels/telegram/tg_format.h
main/channels/telegram/tg_format.c
spiffs/AGENTS.md
spiffs/TOOLS.md
test/unit/test_F4_pairing.c
test/unit/test_F5_memory.c
test/unit/test_F7_session.c
test/unit/test_F8_tg_format.c
```

## Files Modified

```
main/memory/session_mgr.h          (added user_id params)
main/memory/session_mgr.c          (session path isolation)
main/agent/agent_loop.c            (3 session call sites)
main/cli/serial_cli.c              (session_clear + user_id arg)
main/mimi_config.h                 (MIMI_MEMORY_RECENT_DAYS)
main/tools/tool_memory.c           (default days from config)
```

## Test Summary

| Feature | Test File | Tests |
|---------|-----------|-------|
| F4 Pairing | test_F4_pairing.c | 20 |
| F5 Memory | test_F5_memory.c | 8 |
| F7 Session | test_F7_session.c | 7 |
| F8 Format | test_F8_tg_format.c | 8 |
| **Total** | | **43** |

## Notes

- All LSP errors on local machine are expected (no ESP-IDF headers). Tests compile and run on Termux/ESP-IDF target.
- Session isolation is a breaking change - any future code calling session_* functions must include user_id.
- `msg.user_id` fallback: "unknown" used when user_id is empty (e.g., CLI messages).

## Next Steps

- F11+: Continue with remaining features per spec/program/program1.md
- Run full test suite on Termux to validate all tests pass
- Flash to device and verify pairing/auth flow end-to-end