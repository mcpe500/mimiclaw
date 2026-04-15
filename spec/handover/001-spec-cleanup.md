# Handover: MimiClaw Development Specs Update

**Date**: 2026-04-15  
**Session**: Spec updates + cleanup SSH

---

## What Was Done

### 1. Spec Files Updated

**spec/program/program1.md** — Development loop program
- Removed SSH/controller complexity
- Simplified to direct Termux workflow
- Added Termux tool availability table
- Kept gate matrix (test, analyze, asan, coverage available; valgrind, fuzz, idf.py blocked)
- Simplified loop steps

**spec/001.test-infrastructure.md** — Test infrastructure spec
- Removed all SSH references
- Simplified to Termux-only testing
- Kept core: mocks structure, gate specs, results format

### 2. Key Changes

| Before | After |
|--------|-------|
| SSH-based workflow | Direct Termux workflow |
| Controller compatibility section | Removed |
| SSH connection strategy | Removed |
| sshpass requirements | Removed |
| Preflight with SSH checks | Simple preflight (git status) |

### 3. Files Modified

- `spec/program/program1.md` — Rewritten ~150 lines
- `spec/001.test-infrastructure.md` — Rewritten ~120 lines
- Created `spec/handover/` directory

---

## Next Steps

### To Start Development Loop

On Termux server, run:

```bash
cd ~/mimiclaw

# 1. Preflight
git status --short --branch

# 2. Create branch
git checkout -b dev/apr15
git push -u origin dev/apr15

# 3. Initialize results.tsv
echo -e "commit\tfeature\ttests_total\ttests_pass\tstatic_warns\tasan_errors\tvalgrind_errs\tfuzz_crashes\tcoverage_pct\tstatus\tdescription" > results.tsv

# 4. Start F1: Provider abstraction
```

### F1: Provider Abstraction

**Task**: Abstract LLM interface so providers are plugins, not hardcoded.

**Files to create**:
- `main/llm/provider.h` — Provider interface struct
- `main/llm/provider_anthropic.c` — Anthropic implementation
- `main/llm/provider_openai.c` — OpenAI-compatible implementation

**Files to modify**:
- `main/llm/llm_proxy.c` — Refactor to use provider interface

**Tests to write**:
- `test/unit/test_F1_provider.c`

---

## Notes

- LSP errors in `main/*.c` are expected — local machine has no ESP-IDF headers
- Tests run on Termux where ESP-IDF or mocks are available
- SSH removed due to authentication issues on Windows controller
- User will test the specs directly on Termux

---

*End of handover*