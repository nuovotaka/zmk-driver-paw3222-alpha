/*
 * Copyright 2024 Google LLC
 * Modifications Copyright 2025 sekigon-gonnoc
 * Modifications Copyright 2025 nuovotaka
 * Original source code: https://github.com/zephyrproject-rtos/zephyr/blob/19c6240b6865bcb28e1d786d4dcadfb3a02067a0/include/zephyr/input/input_paw32xx.h
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INPUT_PAW32XX_H_
#define ZEPHYR_INCLUDE_INPUT_PAW32XX_H_

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <stdbool.h>
#include <stdint.h>

/* These functions are declared in paw3222_power.h */

/**
 * @brief paw32xx configuration struct
 *
 * This struct should be defined in the driver source, but you may want to
 * reference it for overlay or advanced configuration.
 */
struct paw32xx_config {
    struct spi_dt_spec spi;
    struct gpio_dt_spec irq_gpio;
    struct gpio_dt_spec power_gpio;
    size_t scroll_layers_len;
    int32_t *scroll_layers;
    size_t snipe_layers_len;
    int32_t *snipe_layers;
    size_t scroll_horizontal_layers_len;
    int32_t *scroll_horizontal_layers;
    int16_t res_cpi;
    int16_t snipe_cpi;
    uint8_t snipe_divisor; // Additional precision divisor for snipe mode (default: 2)
    bool force_awake;
    uint8_t rotation;      // Rotation angle (0, 90, 180, 270)
    uint8_t scroll_tick;   // Scroll tick threshold
};

/**
 * @brief paw32xx runtime data struct
 *
 * This struct is typically only used internally by the driver.
 */
struct paw32xx_data {
    const struct device *dev;
    struct k_work motion_work;
    struct gpio_callback motion_cb;
    struct k_timer motion_timer;
    int16_t current_cpi;
    int16_t scroll_accumulator;  // Accumulator for smooth scrolling (reduced from int32_t)
};

#endif /* ZEPHYR_INCLUDE_INPUT_PAW32XX_H_ */
