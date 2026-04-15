#pragma once

#include "esp_err.h"
#include <stddef.h>

/**
 * Initialize session manager.
 */
esp_err_t session_mgr_init(void);

/**
 * Append a message to a session file (JSONL format).
 * Session is isolated by both chat_id AND user_id.
 * @param chat_id Session identifier (e.g., "12345")
 * @param user_id User identifier for session isolation
 * @param role "user" or "assistant"
 * @param content Message text
 */
esp_err_t session_append(const char *chat_id, const char *user_id, const char *role, const char *content);

/**
 * Load session history as a JSON array string suitable for LLM messages.
 * Returns the last max_msgs messages as:
 * [{"role":"user","content":"..."},{"role":"assistant","content":"..."},...]
 *
 * @param chat_id Session identifier
 * @param user_id User identifier for session isolation
 * @param buf Output buffer (caller allocates)
 * @param size Buffer size
 * @param max_msgs Maximum number of messages to return
 */
esp_err_t session_get_history_json(const char *chat_id, const char *user_id, char *buf, size_t size, int max_msgs);

/**
 * Clear a session (delete the file).
 */
esp_err_t session_clear(const char *chat_id, const char *user_id);

/**
 * List all session files (prints to log).
 */
void session_list(void);