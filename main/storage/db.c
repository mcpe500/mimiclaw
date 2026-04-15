#include "db.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"

#ifdef __ESP32__
#include "sqlite3.h"
#else
#include <sqlite3.h>
#endif

static const char *TAG = "db";
static sqlite3 *s_db = NULL;

static int callback_one(void *data, int argc, char **argv, char **col_names)
{
    (void)col_names;
    if (argc > 0 && argv[0]) {
        char *result = (char *)data;
        strncpy(result, argv[0], 1023);
        result[1023] = '\0';
    }
    return 0;
}

static int callback_exec(void *data, int argc, char **argv, char **col_names)
{
    (void)data; (void)argc; (void)argv; (void)col_names;
    return 0;
}

static esp_err_t create_tables(void)
{
    const char *schema[] = {
        "CREATE TABLE IF NOT EXISTS memory ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  key TEXT UNIQUE NOT NULL,"
        "  value TEXT,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");",
        "CREATE TABLE IF NOT EXISTS sessions ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  chat_id TEXT NOT NULL,"
        "  user_id TEXT NOT NULL,"
        "  role TEXT NOT NULL,"
        "  content TEXT,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");",
        "CREATE TABLE IF NOT EXISTS tools_log ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  tool_name TEXT NOT NULL,"
        "  input_json TEXT,"
        "  output TEXT,"
        "  success INTEGER DEFAULT 1,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");",
        "CREATE TABLE IF NOT EXISTS events ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  event_type TEXT NOT NULL,"
        "  source TEXT,"
        "  data TEXT,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");",
        NULL
    };

    for (int i = 0; schema[i] != NULL; i++) {
        char *err_msg = NULL;
        int rc = sqlite3_exec(s_db, schema[i], callback_exec, NULL, &err_msg);
        if (rc != SQLITE_OK) {
            ESP_LOGE(TAG, "Schema error: %s", err_msg);
            sqlite3_free(err_msg);
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "Database tables created");
    return ESP_OK;
}

esp_err_t db_init(const char *path)
{
    if (!path) return ESP_ERR_INVALID_ARG;
    if (s_db) {
        ESP_LOGW(TAG, "Database already open");
        return ESP_OK;
    }

    int rc = sqlite3_open(path, &s_db);
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "Cannot open database: %s", sqlite3_errmsg(s_db));
        sqlite3_close(s_db);
        s_db = NULL;
        return ESP_FAIL;
    }

    sqlite3_exec(s_db, "PRAGMA journal_mode=WAL;", callback_exec, NULL, NULL);
    sqlite3_exec(s_db, "PRAGMA foreign_keys=ON;", callback_exec, NULL, NULL);

    ESP_LOGI(TAG, "Database opened: %s", path);
    return create_tables();
}

esp_err_t db_exec(const char *sql)
{
    if (!s_db || !sql) return ESP_ERR_INVALID_ARG;

    char *err_msg = NULL;
    int rc = sqlite3_exec(s_db, sql, callback_exec, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "SQL error: %s", err_msg);
        sqlite3_free(err_msg);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t db_query_one(const char *sql, char *result, size_t size)
{
    if (!s_db || !sql || !result || size == 0) return ESP_ERR_INVALID_ARG;

    result[0] = '\0';

    char tmp[1024] = {0};
    char *err_msg = NULL;
    int rc = sqlite3_exec(s_db, sql, callback_one, tmp, &err_msg);
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "Query error: %s", err_msg);
        sqlite3_free(err_msg);
        return ESP_FAIL;
    }

    strncpy(result, tmp, size - 1);
    result[size - 1] = '\0';
    return ESP_OK;
}

void db_close(void)
{
    if (s_db) {
        sqlite3_close(s_db);
        s_db = NULL;
        ESP_LOGI(TAG, "Database closed");
    }
}
