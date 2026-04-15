CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -g -O2 -D_GNU_SOURCE
CFLAGS += -Ishims -I../main -Ivendor -Ihost -Ivendor/cJSON
CFLAGS += -DMIMI_SPIFFS_BASE=\"./data\" -DMIMI_SPIFFS_SESSION_DIR=\"./data/sessions\"
CFLAGS += -DMIMI_SPIFFS_MEMORY_DIR=\"./data/memory\" -DMIMI_SPIFFS_CONFIG_DIR=\"./data/config\"
CFLAGS += -DMIMI_MEMORY_FILE=\"./data/memory/MEMORY.md\"
CFLAGS += -DMIMI_CONTEXT_BUF_SIZE=16384 -DMIMI_SESSION_MAX_MSGS=20 -DMIMI_MEMORY_RECENT_DAYS=7
CFLAGS += -DMIMI_AGENT_MAX_HISTORY=20 -DMIMI_AGENT_MAX_TOOL_ITER=10 -DMIMI_MAX_TOOL_CALLS=4
CFLAGS += -DMIMI_LLM_STREAM_BUF_SIZE=32768
CFLAGS += -DMIMI_BUS_QUEUE_LEN=64
CFLAGS += -DMIMI_WS_PORT=18789 -DMIMI_WS_MAX_CLIENTS=8
CFLAGS += -DMIMI_ADMIN_PORT=8080
CFLAGS += -DMIMI_CHAN_TELEGRAM=\"telegram\" -DMIMI_CHAN_DISCORD=\"discord\"
CFLAGS += -DMIMI_CHAN_SLACK=\"slack\" -DMIMI_CHAN_WHATSAPP=\"whatsapp\"
CFLAGS += -DMIMI_CHAN_FEISHU=\"feishu\" -DMIMI_CHAN_WEBSOCKET=\"websocket\"
CFLAGS += -DMIMI_CHAN_CLI=\"cli\" -DMIMI_CHAN_SYSTEM=\"system\"
CFLAGS += -DMIMI_LLM_DEFAULT_MODEL=\"claude-sonnet-4-5\" -DMIMI_LLM_PROVIDER_DEFAULT=\"anthropic\"
CFLAGS += -DMIMI_LLM_MAX_TOKENS=4096
CFLAGS += -DMIMI_LLM_API_URL=\"https://api.anthropic.com/v1/messages\"
CFLAGS += -DMIMI_LLM_API_VERSION=\"2023-06-01\"
CFLAGS += -DMIMI_OPENAI_API_URL=\"https://api.openai.com/v1/chat/completions\"
CFLAGS += -DMIMI_NVS_WIFI=\"wifi_config\" -DMIMI_NVS_TG=\"tg_config\"
CFLAGS += -DMIMI_NVS_DISCORD=\"discord_config\" -DMIMI_NVS_SLACK=\"slack_config\"
CFLAGS += -DMIMI_NVS_WHATSAPP=\"whatsapp_config\"
CFLAGS += -DMIMI_TG_MAX_MSG_LEN=4096 -DMIMI_TG_POLL_TIMEOUT_S=30
CFLAGS += -DMIMI_TIMEZONE=\"UTC0\"
CFLAGS += -DMIMI_AGENT_SEND_WORKING_STATUS=1 -DMIMI_AGENT_PRIO=6 -DMIMI_AGENT_CORE=1
CFLAGS += -DMIMI_AGENT_STACK=24576
CFLAGS += -DMIMI_GPIO_CONFIG_SECTION=1
CFLAGS += -DMIMI_SUBTASK_MAX_JOBS=32
CFLAGS += -DMIMI_SKILLS_PREFIX=\"./data/skills/\"
CFLAGS += -DMIMI_SOUL_FILE=\"./data/config/SOUL.md\"
CFLAGS += -DMIMI_USER_FILE=\"./data/config/USER.md\"
CFLAGS += -DMIMI_CRON_FILE=\"./data/cron.json\" -DMIMI_CRON_MAX_JOBS=16
CFLAGS += -DMIMI_HEARTBEAT_FILE=\"./data/HEARTBEAT.md\"
CFLAGS += -DMIMI_TG_POLL_STACK=12288 -DMIMI_TG_POLL_PRIO=5 -DMIMI_TG_POLL_CORE=0
CFLAGS += -DMIMI_DISCORD_GW_STACK=12288 -DMIMI_DISCORD_GW_PRIO=5
CFLAGS += -DMIMI_WHATSAPP_POLL_STACK=12288 -DMIMI_WHATSAPP_POLL_PRIO=5 -DMIMI_WHATSAPP_POLL_CORE=0
CFLAGS += -DMIMI_OUTBOUND_STACK=12288 -DMIMI_OUTBOUND_PRIO=5 -DMIMI_OUTBOUND_CORE=0
CFLAGS += -DMIMI_SECRET_DISCORD_TOKEN=\"\" -DMIMI_SECRET_TG_TOKEN=\"\"

LDFLAGS = -lcurl -lsqlite3 -lpthread -lm

VENDOR_DIR = vendor
HOST_SRCS  = $(shell find host -name "*.c")
SHIM_SRCS  = shims/esp_http_client.c shims/nvs.c
MAIN_SRCS  = $(shell find ../main -name "*.c" ! -name "mimi.c" ! -name "message_bus.c" ! -path "*/wifi*" ! -path "*/onboard*" ! -path "*/cli*" ! -path "*/display*")
VENDOR_SRCS = $(VENDOR_DIR)/cJSON/cJSON.c
SRCS       = main.c $(HOST_SRCS) $(SHIM_SRCS) $(MAIN_SRCS)

TARGET = mimiclaw

.PHONY: all clean setup run

all: setup $(TARGET)

setup:
	@mkdir -p $(VENDOR_DIR)/cJSON
	@command -v curl > /dev/null 2>&1 || { echo "curl command required"; exit 1; }
	@if [ ! -f $(VENDOR_DIR)/cJSON/cJSON.h ]; then \
		echo "Downloading cJSON..."; \
		curl -sL https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.h -o $(VENDOR_DIR)/cJSON/cJSON.h; \
		curl -sL https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.c -o $(VENDOR_DIR)/cJSON/cJSON.c; \
	fi
	@mkdir -p data/sessions data/memory data/config data/skills

$(TARGET): $(SRCS) $(VENDOR_SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(VENDOR_SRCS) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
	rm -rf data
