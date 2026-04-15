#pragma once
#include <stdbool.h>

typedef int gpio_num_t;

#define GPIO_MODE_INPUT        1
#define GPIO_MODE_OUTPUT       2
#define GPIO_MODE_INPUT_OUTPUT 3

static inline int gpio_set_direction(gpio_num_t gpio, int mode) {
    (void)gpio; (void)mode;
    return 0;
}

static inline int gpio_set_level(gpio_num_t gpio, int level) {
    (void)gpio; (void)level;
    return 0;
}

static inline int gpio_get_level(gpio_num_t gpio) {
    (void)gpio;
    return 0;
}

#define GPIO_IS_VALID_GPIO(gpio) ((gpio) >= 0 && (gpio) <= 48)
