CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -g -O2 -Ishims -I../main -Ivendor
LDFLAGS = -lcurl -lsqlite3 -lpthread -lm

VENDOR_DIR = vendor
SHIM_SRCS  = shims/esp_http_client.c shims/nvs.c
MAIN_SRCS  = main.c
SRCS       = $(MAIN_SRCS) $(SHIM_SRCS)

TARGET = mimiclaw

.PHONY: all clean setup

all: setup $(TARGET)

setup:
	@mkdir -p $(VENDOR_DIR)/cJSON
	@command -v curl > /dev/null || { echo "curl required"; exit 1; }
	@if [ ! -f $(VENDOR_DIR)/cJSON/cJSON.h ]; then \
		curl -sL https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.h -o $(VENDOR_DIR)/cJSON/cJSON.h; \
		curl -sL https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.c -o $(VENDOR_DIR)/cJSON/cJSON.c; \
	fi
	@mkdir -p data/sessions data/memory data/config data/skills

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(VENDOR_DIR)/cJSON/cJSON.c $(LDFLAGS)

clean:
	rm -f $(TARGET)
	rm -rf data
