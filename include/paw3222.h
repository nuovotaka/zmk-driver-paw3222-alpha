/*
 * Copyright 2024 Google LLC
 * Modifications Copyright 2025 nuovotaka
 * Modifications Copyright 2025 sekigon-gonnoc
 * Original source code: https://github.com/zephyrproject-rtos/zephyr/blob/19c6240b6865bcb28e1d786d4dcadfb3a02067a0/include/zephyr/input/input_paw32xx.h
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INPUT_PAW32XX_H_
#define ZEPHYR_INCLUDE_INPUT_PAW32XX_H_

#include <zephyr/device.h>
#include <stdbool.h>
#include <stdint.h>

#define INPUT_EVENT_SCROLL_LAYER_CHANGED 0x1000

/**
 * @brief Set resolution on a paw32xx device
 *
 * @param dev paw32xx device.
 * @param res_cpi CPI resolution, e.g. 200 to 3200.
 */
int paw32xx_set_resolution(const struct device *dev, uint16_t res_cpi);

/**
 * @brief Set force awake mode on a paw32xx device
 *
 * @param dev paw32xx device.
 * @param enable whether to enable or disable force awake mode.
 */
int paw32xx_force_awake(const struct device *dev, bool enable);

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
    int16_t res_cpi;
    int16_t snipe_cpi;
    bool force_awake;
    bool scroll_enabled;
    bool snipe_enabled;
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
    int16_t last_x;
    int16_t last_y;
    int16_t scroll_delta_x;
    int16_t scroll_delta_y;
    int16_t current_cpi;
    enum { SCROLL_UNLOCKED, SCROLL_LOCKED_X, SCROLL_LOCKED_Y } scroll_lock;
    int64_t scroll_lock_expire_time;
    bool just_unlocked;
};

#endif /* ZEPHYR_INCLUDE_INPUT_PAW32XX_H_ */
