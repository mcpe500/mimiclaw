# MimiClaw Tools

Available tools for the LLM agent to call.

## Memory Tools

### memory_read
Read long-term memory (MEMORY.md). Use when user asks what the robot knows, remembers, or has stored.
- Input: `{"purpose": "description of why you need the memory"}`
- Returns: Content of MEMORY.md

### memory_write
Write content to long-term memory (MEMORY.md). Use when user asks to remember, store, or save information.
- Input: `{"content": "the information to store"}`
- Returns: Success confirmation

### memory_append
Append a note to today's daily memory file. Use for timestamped event logging.
- Input: `{"note": "the note to append"}`
- Returns: Success confirmation

### memory_recall
Read recent daily memories from the last N days. Use when user asks what happened recently.
- Input: `{"days": 3}` (optional, default 3, max 30)
- Returns: Recent memory entries

## File Tools

### read_file
Read a file from SPIFFS storage.
- Input: `{"path": "/spiffs/..."}`
- Returns: File contents

### write_file
Write/overwrite a file on SPIFFS storage.
- Input: `{"path": "/spiffs/...", "content": "..."}`
- Returns: Success confirmation

### edit_file
Find-and-replace edit a file on SPIFFS storage.
- Input: `{"path": "/spiffs/...", "old_string": "...", "new_string": "..."}`
- Returns: Success confirmation

### list_dir
List files on SPIFFS, optionally filtered by path prefix.
- Input: `{"prefix": "/spiffs/..."}` (optional)
- Returns: List of files

## Web Search Tools

### web_search
Search the web using Brave Search API.
- Input: `{"query": "search terms", "count": 5}` (count optional, default 5)
- Returns: Search results with titles, URLs, and descriptions

## Time Tools

### get_time
Get current time information.
- Input: `{}`
- Returns: Current time in ISO 8601 format and unix timestamp

## GPIO Tools

### gpio_read
Read GPIO pin state.
- Input: `{"pin": 0}` (pin number)
- Returns: Pin state (0 or 1)

### gpio_write
Write GPIO pin state.
- Input: `{"pin": 0, "state": 1}` (pin and state)
- Returns: Success confirmation

### gpio_list
List configured GPIO pins and their states.
- Input: `{}`
- Returns: List of GPIO pins with states

## Cron Tools

### cron_list
List all scheduled cron jobs.
- Input: `{}`
- Returns: List of scheduled jobs

### cron_add
Add a new cron job.
- Input: `{"name": "job1", "schedule": "* * * * *", "command": "..."}`
- Returns: Success confirmation

### cron_remove
Remove a cron job.
- Input: `{"name": "job1"}`
- Returns: Success confirmation