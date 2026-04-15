CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -g -O0
CFLAGS += -D_GNU_SOURCE
CFLAGS += -I$(SHIM_DIR) -I$(SHIM_DIR)/argtable3 -I$(MAIN_DIR) -I$(VENDOR_DIR) -I$(VENDOR_DIR)/unity -I$(VENDOR_DIR)/cJSON
CFLAGS += -DMIMI_SPIFFS_BASE=\"/tmp/mimiclaw\" -DMIMI_SPIFFS_SESSION_DIR=\"/tmp/mimiclaw/sessions\"
CFLAGS += -DMIMI_SPIFFS_MEMORY_DIR=\"/tmp/mimiclaw/memory\" -DMIMI_SPIFFS_CONFIG_DIR=\"/tmp/mimiclaw/config\"
CFLAGS += -DMIMI_MEMORY_FILE=\"/tmp/mimiclaw/memory/MEMORY.md\"
CFLAGS += -DMIMI_CONTEXT_BUF_SIZE=16384 -DMIMI_SESSION_MAX_MSGS=20 -DMIMI_MEMORY_RECENT_DAYS=7
CFLAGS += -DMIMI_AGENT_MAX_HISTORY=20 -DMIMI_AGENT_MAX_TOOL_ITER=10 -DMIMI_MAX_TOOL_CALLS=4
CFLAGS += -DMIMI_LLM_STREAM_BUF_SIZE=32768
CFLAGS += -DMIMI_WIFI_MAX_RETRY=10 -DMIMI_TG_MAX_MSG_LEN=4096 -DMIMI_TG_POLL_TIMEOUT_S=30
CFLAGS += -DMIMI_WS_PORT=18789 -DMIMI_WS_MAX_CLIENTS=4 -DMIMI_CHAN_TELEGRAM=\"telegram\"
CFLAGS += -DMIMI_CHAN_FEISHU=\"feishu\" -DMIMI_CHAN_WEBSOCKET=\"websocket\"
CFLAGS += -DMIMI_CHAN_CLI=\"cli\" -DMIMI_CHAN_SYSTEM=\"system\"
CFLAGS += -DMIMI_BUS_QUEUE_LEN=16 -DMIMI_GPIO_CONFIG_SECTION=1
CFLAGS += -DMIMI_CRON_FILE=\"/tmp/mimiclaw/cron.json\" -DMIMI_CRON_MAX_JOBS=16
CFLAGS += -DMIMI_HEARTBEAT_FILE=\"/tmp/mimiclaw/HEARTBEAT.md\"
CFLAGS += -DMIMI_SKILLS_PREFIX=\"/tmp/mimiclaw/skills/\"
CFLAGS += -DMIMI_SOUL_FILE=\"/tmp/mimiclaw/config/SOUL.md\"
CFLAGS += -DMIMI_USER_FILE=\"/tmp/mimiclaw/config/USER.md\"
CFLAGS += -DMIMI_LLM_DEFAULT_MODEL=\"claude-opus-4-5\" -DMIMI_LLM_PROVIDER_DEFAULT=\"anthropic\"
CFLAGS += -DMIMI_LLM_MAX_TOKENS=4096 -DMIMI_LLM_API_URL=\"https://api.anthropic.com/v1/messages\"
CFLAGS += -DMIMI_OPENAI_API_URL=\"https://api.openai.com/v1/chat/completions\"
CFLAGS += -DMIMI_LLM_API_VERSION=\"2023-06-01\" -DMIMI_LLM_LOG_VERBOSE_PAYLOAD=0 -DMIMI_LLM_LOG_PREVIEW_BYTES=160
CFLAGS += -DMIMI_TIMEZONE=\"UTC0\"
CFLAGS += -DMIMI_NVS_WIFI=\"wifi_config\" -DMIMI_NVS_TG=\"tg_config\" -DMIMI_NVS_FEISHU=\"feishu_config\"
CFLAGS += -DMIMI_NVS_LLM=\"llm_config\" -DMIMI_NVS_PROXY=\"proxy_config\" -DMIMI_NVS_SEARCH=\"search_config\"
CFLAGS += -DMIMI_FEISHU_MAX_MSG_LEN=4096 -DMIMI_FEISHU_WEBHOOK_PORT=18790
CFLAGS += -DMIMI_ONBOARD_AP_PREFIX=\"MimiClaw-\" -DMIMI_ONBOARD_HTTP_PORT=80
CFLAGS += -DMIMI_ONBOARD_MAX_SCAN=20 -DMIMI_AGENT_SEND_WORKING_STATUS=1
CFLAGS += -DMIMI_CHAN_DISCORD=\"discord\" -DMIMI_CHAN_SLACK=\"slack\" -DMIMI_CHAN_WHATSAPP=\"whatsapp\"
CFLAGS += -DMIMI_NVS_DISCORD=\"discord_config\" -DMIMI_NVS_SLACK=\"slack_config\" -DMIMI_NVS_WHATSAPP=\"whatsapp_config\"
CFLAGS += -DMIMI_SECRET_DISCORD_TOKEN=\"\" -DMIMI_SECRET_SLACK_APP_TOKEN=\"\" -DMIMI_SECRET_SLACK_BOT_TOKEN=\"\"
CFLAGS += -DMIMI_SECRET_WHATSAPP_TOKEN=\"\" -DMIMI_SECRET_WHATSAPP_PHONE_ID=\"\"
CFLAGS += -DMIMI_DISCORD_GW_STACK=12288 -DMIMI_AGENT_CORE=1 -DMIMI_AGENT_STACK=8192
CFLAGS += -DMIMI_SUBTASK_MAX_JOBS=32 -DMIMI_SUBTASK_STACK=4096 -DMIMI_SUBTASK_WORKERS=2
CFLAGS += -DMIMI_ADMIN_PORT=8080

LDFLAGS = -lm -lsqlite3 -lpthread

MAIN_DIR   = ../main
SHIM_DIR   = shims
VENDOR_DIR = vendor
BUILD_DIR  = build

TEST_SRCS = $(wildcard unit/test_*.c)
TESTS     = $(patsubst unit/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))

VENDOR_SRCS = $(VENDOR_DIR)/unity/unity.c $(VENDOR_DIR)/cJSON/cJSON.c
MAIN_SRCS   = $(shell find $(MAIN_DIR) -name "*.c" ! -name "mimi.c")
SHIM_SRCS   = $(shell find $(SHIM_DIR) -name "*.c")

.PHONY: all test clean setup analyze asan coverage full

all: test

setup:
	@echo "== Downloading Unity test framework =="
	@mkdir -p $(VENDOR_DIR)/unity
	@curl -sL https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src/unity.h -o $(VENDOR_DIR)/unity/unity.h
	@curl -sL https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src/unity.c -o $(VENDOR_DIR)/unity/unity.c
	@curl -sL https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src/unity_internals.h -o $(VENDOR_DIR)/unity/unity_internals.h
	@echo "== Downloading cJSON =="
	@mkdir -p $(VENDOR_DIR)/cJSON
	@curl -sL https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.h -o $(VENDOR_DIR)/cJSON/cJSON.h
	@curl -sL https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.c -o $(VENDOR_DIR)/cJSON/cJSON.c
	@echo "== Done. Run 'make test' to build and run tests. =="

$(BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)/test_%: unit/test_%.c $(VENDOR_SRCS) $(MAIN_SRCS) $(SHIM_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(VENDOR_SRCS) $(MAIN_SRCS) $(SHIM_SRCS) $(LDFLAGS)

test: $(TESTS)
	@echo ""
	@echo "========== RUNNING ALL TESTS =========="
	@pass=0; fail=0; total=0; \
	for t in $(TESTS); do \
		total=$$((total + 1)); \
		echo "--- $$t ---"; \
		if ./$$t > /dev/null 2>&1; then \
			echo "  PASS"; pass=$$((pass + 1)); \
		else \
			echo "  FAIL"; ./$$t; fail=$$((fail + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "========== RESULTS: $$pass/$$total passed, $$fail failed =========="

analyze:
	@echo "Static analysis (cppcheck)..."
	@command -v cppcheck > /dev/null 2>&1 || { echo "cppcheck not found, skipping"; exit 0; }
	cppcheck --enable=all --suppress=missingInclude --error-exitcode=0 ../main/ unit/

asan: CFLAGS += -fsanitize=address -fno-omit-frame-pointer
asan: clean $(TESTS)
	@echo "Running with AddressSanitizer..."
	@for t in $(TESTS); do ./$$t || exit 1; done

coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: clean $(TESTS)
	@echo "Running tests with coverage..."
	@for t in $(TESTS); do ./$$t || exit 1; done
	@echo "Coverage report:"
	@command -v gcov > /dev/null 2>&1 || { echo "gcov not found, skipping report"; exit 0; }
	@gcov -n -b unit/test_*.c > /dev/null 2>&1
	@echo "Done. See *.gcov files for details."

full: analyze asan coverage test

clean:
	@rm -rf $(BUILD_DIR)
	@rm -f *.gcno *.gcda *.gcov
