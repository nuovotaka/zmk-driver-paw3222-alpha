/*
 * Copyright 2024 Google LLC
 * Modifications Copyright 2025 sekigon-gonnoc
 * Modifications Copyright 2025 nuovotaka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PAW3222_POWER_H_
#define PAW3222_POWER_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>

/**
 * @brief Configure the PAW3222 sensor
 *
 * @param dev PAW3222 device
 * @return 0 on success, negative error code on failure
 */
int paw32xx_configure(const struct device *dev);

/**
 * @brief Set resolution on a paw32xx device
 *
 * @param dev paw32xx device.
 * @param res_cpi CPI resolution, e.g. 200 to 3200.
 * @return 0 on success, negative error code on failure
 */
int paw32xx_set_resolution(const struct device *dev, uint16_t res_cpi);

/**
 * @brief Set force awake mode on a paw32xx device
 *
 * @param dev paw32xx device.
 * @param enable whether to enable or disable force awake mode.
 * @return 0 on success, negative error code on failure
 */
int paw32xx_force_awake(const struct device *dev, bool enable);

#ifdef CONFIG_PM_DEVICE
/**
 * @brief Power management action handler
 *
 * @param dev PAW3222 device
 * @param action Power management action
 * @return 0 on success, negative error code on failure
 */
int paw32xx_pm_action(const struct device *dev, enum pm_device_action action);
#endif

#endif /* PAW3222_POWER_H_ */