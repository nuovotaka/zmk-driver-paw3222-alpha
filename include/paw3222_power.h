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
 * @param dev paw32xx device (must not be NULL)
 * @param res_cpi CPI resolution (range: 608-4826, values below 608 are clamped)
 * @return 0 on success, negative error code on failure
 * @retval -EINVAL if res_cpi is out of valid range
 * @retval -EIO if SPI communication fails
 */
int paw32xx_set_resolution(const struct device *dev, uint16_t res_cpi);

/**
 * @brief Set force awake mode on a paw32xx device
 *
 * Force awake mode prevents the sensor from entering sleep mode automatically.
 * This can improve response time but increases power consumption.
 *
 * @param dev paw32xx device (must not be NULL)
 * @param enable true to enable force awake mode, false to allow sleep
 * @return 0 on success, negative error code on failure
 * @retval -EIO if SPI communication fails
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