#pragma once
#include "esp_err.h"

typedef int gpio_num_t;
typedef int gpio_mode_t;
typedef int gpio_pull_mode_t;
typedef int gpio_int_type_t;

#define GPIO_MODE_INPUT 0
#define GPIO_MODE_OUTPUT 1
#define GPIO_MODE_INPUT_OUTPUT 2

static inline esp_err_t gpio_config(void *conf) { (void)conf; return ESP_OK; }
static inline esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode) { return ESP_OK; }
static inline esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level) { return ESP_OK; }
static inline int gpio_get_level(gpio_num_t gpio_num) { return 0; }
