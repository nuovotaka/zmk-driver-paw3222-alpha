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
 * @brief Toggle acceleration for cursor movement (MOVE/SNIPE) at runtime
 *
 * @param dev paw32xx device.
 */
void paw32xx_toggle_accel_move_enable(const struct device *dev);

/**
 * @brief Toggle acceleration for scroll movement at runtime
 *
 * @param dev paw32xx device.
 */
void paw32xx_toggle_accel_scroll_enable(const struct device *dev);

/**
 * @brief Set acceleration for cursor movement (MOVE/SNIPE) at runtime
 *
 * @param dev paw32xx device.
 * @param enable true to enable, false to disable
 */
void paw32xx_set_accel_move_enable(const struct device *dev, bool enable);

/**
 * @brief Set acceleration for scroll movement at runtime
 *
 * @param dev paw32xx device.
 * @param enable true to enable, false to disable
 */
void paw32xx_set_accel_scroll_enable(const struct device *dev, bool enable);

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

    // Acceleration curve
    const int32_t *accel_thresholds;
    size_t accel_thresholds_len;
    const int32_t *accel_factors; // fixed-point: 1.5→15, 2.5→25 (小数点1桁)
    size_t accel_factors_len;
    bool accel_move_enable;
    bool accel_scroll_enable;

        // Layer config
    const int32_t *scroll_layers;
    size_t scroll_layers_len;
    const int32_t *snipe_layers;
    size_t snipe_layers_len;
    const int32_t *scroll_horizontal_layers;
    size_t scroll_horizontal_layers_len;

    // Sensor config
    uint16_t res_cpi;
    uint16_t snipe_cpi;
    bool force_awake;
    bool scroll_enabled;
    bool snipe_enabled;
    uint8_t rotation;      // 回転角度（0, 90, 180, 270）
    uint8_t scroll_tick;   // スクロールtick
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
    int64_t scroll_unlock_time;
    // Acceleration runtime flags
    bool accel_move_enable_runtime;
    bool accel_scroll_enable_runtime;
};

#endif /* ZEPHYR_INCLUDE_INPUT_PAW32XX_H_ */
