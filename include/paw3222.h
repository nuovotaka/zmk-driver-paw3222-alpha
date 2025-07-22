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
#include <zmk/keymap.h>
#include <zmk/events/keycode_state_changed.h>

#define INPUT_EVENT_SCROLL_LAYER_CHANGED 0x1000
#define PAW32XX_TOGGLE_ACCEL_KEYCODE CONFIG_PAW32XX_TOGGLE_ACCEL_KEYCODE // 加速度カーブ切り替えキーコード
#define PAW32XX_TOGGLE_ACCEL_DEVICE_SELECT_KEYCODE CONFIG_PAW32XX_TOGGLE_ACCEL_DEVICE_SELECT_KEYCODE // センサー選択モード切り替えキーコード

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
 * @brief Toggle acceleration curve for a paw32xx device
 *
 * @param dev paw32xx device.
 * @param enable_move Whether to enable acceleration for cursor movement.
 * @param enable_scroll Whether to enable acceleration for scrolling.
 */
int paw32xx_toggle_acceleration(const struct device *dev, bool enable_move, bool enable_scroll);

/**
 * @brief Process key event for paw32xx devices
 *
 * @param event Key event to process.
 * @return true if the event was handled, false otherwise.
 */
bool paw32xx_process_key_event(const struct zmk_keycode_state_changed *event);


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

    // Accel config
    const int32_t *accel_thresholds;   // しきい値配列
    size_t accel_thresholds_len;
    const int32_t *accel_factors;      // 倍率配列（1000=1.0倍, 1500=1.5倍など）
    size_t accel_factors_len;
    
    bool accel_move_enable;    // カーソル移動への加速度有効/無効
    bool accel_scroll_enable;  // スクロール移動への加速度有効/無効

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
    int64_t prev_time_move;
    int64_t prev_time_scroll;
    bool accel_move_enabled;    // 実行時に変更可能な加速度カーブ有効フラグ（移動）
    bool accel_scroll_enabled;  // 実行時に変更可能な加速度カーブ有効フラグ（スクロール）
    uint8_t device_id;          // 複数センサー対応のためのデバイスID
};

#endif /* ZEPHYR_INCLUDE_INPUT_PAW32XX_H_ */

// 最大センサー数の定義
#ifndef CONFIG_PAW32XX_MAX_DEVICES
#define CONFIG_PAW32XX_MAX_DEVICES 4
#endif