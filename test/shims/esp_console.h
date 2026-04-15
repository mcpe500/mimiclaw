#pragma once
#include "esp_err.h"
#include <stddef.h>
#include <string.h>
typedef struct { const char *name; const char *help; const char *hint; void (*func)(int argc, char **argv); void *argtable; } esp_console_cmd_t;
typedef struct { int max_cmdline_length; int max_cmdline_args; const char *prompt; const char *history_save_path; } esp_console_repl_config_t;
typedef struct { int channel_id; int baud_rate; } esp_console_dev_uart_config_t;
typedef struct { int channel_id; } esp_console_dev_usb_serial_jtag_config_t;
typedef struct { int channel_id; } esp_console_dev_usb_cdc_config_t;
typedef struct { void *repl; } esp_console_repl_t;
#define ESP_CONSOLE_REPL_CONFIG_DEFAULT() { 256, 32, "mimi> ", "/tmp/cmdline_history" }
#define ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT() { 0, 115200 }
#define ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT() { 0 }
static inline esp_err_t esp_console_new_repl_uart(const esp_console_dev_uart_config_t *dev, const esp_console_repl_config_t *repl, esp_console_repl_t **ret) { (void)dev; (void)repl; if(ret) *ret = NULL; return 0; }
static inline esp_err_t esp_console_new_repl_usb_serial_jtag(const esp_console_dev_usb_serial_jtag_config_t *dev, const esp_console_repl_config_t *repl, esp_console_repl_t **ret) { (void)dev; (void)repl; if(ret) *ret = NULL; return 0; }
static inline esp_err_t esp_console_new_repl_usb_cdc(const esp_console_dev_usb_cdc_config_t *dev, const esp_console_repl_config_t *repl, esp_console_repl_t **ret) { (void)dev; (void)repl; if(ret) *ret = NULL; return 0; }
static inline esp_err_t esp_console_register_help_command(void) { return 0; }
static inline esp_err_t esp_console_cmd_register(const esp_console_cmd_t *cmd) { (void)cmd; return 0; }
static inline esp_err_t esp_console_start_repl(esp_console_repl_t *repl) { (void)repl; return 0; }
static inline void esp_console_deinit(void *repl) { (void)repl; }
