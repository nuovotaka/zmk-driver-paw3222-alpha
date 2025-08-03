/*
 * Copyright 2024 Google LLC
 * Modifications Copyright 2025 sekigon-gonnoc
 * Modifications Copyright 2025 nuovotaka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PAW3222_INPUT_H_
#define PAW3222_INPUT_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "paw3222_regs.h"

/**
 * @brief Get the input mode for the current active layer
 *
 * @param dev PAW3222 device
 * @return Current input mode based on active ZMK layer
 */
enum paw32xx_input_mode get_input_mode_for_current_layer(const struct device *dev);

/**
 * @brief Motion timer handler
 *
 * @param timer Timer that expired
 */
void paw32xx_motion_timer_handler(struct k_timer *timer);

/**
 * @brief Motion work handler - processes sensor data
 *
 * @param work Work item to process
 */
void paw32xx_motion_work_handler(struct k_work *work);

/**
 * @brief GPIO interrupt handler for motion detection
 *
 * @param gpio_dev GPIO device
 * @param cb Callback structure
 * @param pins Pin mask
 */
void paw32xx_motion_handler(const struct device *gpio_dev, struct gpio_callback *cb,
                           uint32_t pins);

#endif /* PAW3222_INPUT_H_ */