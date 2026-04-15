#include "tool_memory.h"
#include "mimi_config.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "tool_memory";

#define TOOL_MEMORY_SCHEMA_READ "{\"type\":\"object\",\"properties\":{\"purpose\":{\"type\":\"string\",\"description\":\"What to use the memory for\"}},\"required\":[\"purpose\"]}"
#define TOOL_MEMORY_SCHEMA_WRITE "{\"type\":\"object\",\"properties\":{\"content\":{\"type\":\"string\",\"description\":\"Content to write to long-term memory\"}},\"required\":[\"content\"]}"
#define TOOL_MEMORY_SCHEMA_APPEND "{\"type\":\"object\",\"properties\":{\"note\":{\"type\":\"string\",\"description\":\"Note to append to today's daily memory\"}},\"required\":[\"note\"]}"
#define TOOL_MEMORY_SCHEMA_RECALL "{\"type\":\"object\",\"properties\":{\"days\":{\"type\":\"integer\",\"description\":\"Number of days to look back (default 7)\"}},\"required\":[]}"

static mimi_tool_t s_memory_tools[] = {
    {
        .name = "memory_read",
        .description = "Read long-term memory (MEMORY.md). Use when user asks what the robot knows, remembers, or has stored.",
        .input_schema_json = TOOL_MEMORY_SCHEMA_READ,
        .execute = tool_memory_read
    },
    {
        .name = "memory_write",
        .description = "Write content to long-term memory (MEMORY.md). Use when user asks to remember, store, or save information.",
        .input_schema_json = TOOL_MEMORY_SCHEMA_WRITE,
        .execute = tool_memory_write
    },
    {
        .name = "memory_append",
        .description = "Append a note to today's daily memory file. Use for timestamped event logging.",
        .input_schema_json = TOOL_MEMORY_SCHEMA_APPEND,
        .execute = tool_memory_append
    },
    {
        .name = "memory_recall",
        .description = "Read recent daily memories from the last N days. Use when user asks what happened recently.",
        .input_schema_json = TOOL_MEMORY_SCHEMA_RECALL,
        .execute = tool_memory_recall
    }
};

esp_err_t tool_memory_init(void)
{
    ESP_LOGI(TAG, "Memory tools initialized with %d tools", (int) (sizeof(s_memory_tools) / sizeof(s_memory_tools[0])));
    return ESP_OK;
}

esp_err_t tool_memory_read(const char *input_json, char *output, size_t output_size)
{
    (void)input_json;

    char buf[4096];
    esp_err_t err = memory_read_long_term(buf, sizeof(buf));

    if (err == ESP_ERR_NOT_FOUND) {
        snprintf(output, output_size, "No long-term memory found.");
        return ESP_OK;
    } else if (err != ESP_OK) {
        snprintf(output, output_size, "Error reading memory: %d", err);
        return err;
    }

    snprintf(output, output_size, "%s", buf);
    return ESP_OK;
}

esp_err_t tool_memory_write(const char *input_json, char *output, size_t output_size)
{
    if (input_json == NULL) {
        snprintf(output, output_size, "Error: null input");
        return ESP_ERR_INVALID_ARG;
    }

    char content[4096] = {0};

    if (strstr(input_json, "\"content\"") != NULL) {
        const char *start = strstr(input_json, "\"content\"");
        start = strchr(start, ':');
        if (start) {
            start++;
            while (*start == ' ' || *start == '"') start++;
            const char *end = start;
            while (*end && *end != '"' && *end != '\\') end++;
            size_t len = (size_t)(end - start);
            if (len >= sizeof(content)) len = sizeof(content) - 1;
            strncpy(content, start, len);
            content[len] = '\0';
        }
    } else {
        snprintf(output, output_size, "Error: content field not found in input");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = memory_write_long_term(content);
    if (err != ESP_OK) {
        snprintf(output, output_size, "Error writing memory: %d", err);
        return err;
    }

    snprintf(output, output_size, "Memory saved successfully.");
    return ESP_OK;
}

esp_err_t tool_memory_append(const char *input_json, char *output, size_t output_size)
{
    if (input_json == NULL) {
        snprintf(output, output_size, "Error: null input");
        return ESP_ERR_INVALID_ARG;
    }

    char note[4096] = {0};

    if (strstr(input_json, "\"note\"") != NULL) {
        const char *start = strstr(input_json, "\"note\"");
        start = strchr(start, ':');
        if (start) {
            start++;
            while (*start == ' ' || *start == '"') start++;
            const char *end = start;
            while (*end && *end != '"' && *end != '\\') end++;
            size_t len = (size_t)(end - start);
            if (len >= sizeof(note)) len = sizeof(note) - 1;
            strncpy(note, start, len);
            note[len] = '\0';
        }
    } else {
        snprintf(output, output_size, "Error: note field not found in input");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = memory_append_today(note);
    if (err != ESP_OK) {
        snprintf(output, output_size, "Error appending to memory: %d", err);
        return err;
    }

    snprintf(output, output_size, "Note appended to today's memory.");
    return ESP_OK;
}

esp_err_t tool_memory_recall(const char *input_json, char *output, size_t output_size)
{
    int days = MIMI_MEMORY_RECENT_DAYS;

    if (input_json != NULL && strstr(input_json, "\"days\"") != NULL) {
        const char *start = strstr(input_json, "\"days\"");
        start = strchr(start, ':');
        if (start) {
            start++;
            while (*start == ' ') start++;
            days = atoi(start);
            if (days <= 0) days = 3;
            if (days > 30) days = 30;
        }
    }

    char buf[4096];
    esp_err_t err = memory_read_recent(buf, sizeof(buf), days);

    if (err == ESP_ERR_NOT_FOUND) {
        snprintf(output, output_size, "No recent memories found.");
        return ESP_OK;
    } else if (err != ESP_OK) {
        snprintf(output, output_size, "Error reading recent memories: %d", err);
        return err;
    }

    snprintf(output, output_size, "%s", buf);
    return ESP_OK;
}