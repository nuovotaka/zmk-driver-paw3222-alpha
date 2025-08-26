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
 * @brief Configure and initialize the PAW3222 sensor
 *
 * Performs initial configuration of the PAW3222 sensor including:
 * - Verifying the product ID to ensure proper sensor communication
 * - Performing a software reset of the sensor
 * - Setting the initial CPI resolution if configured
 * - Configuring force awake mode if enabled
 * - Validating configuration parameters
 *
 * @param dev PAW3222 device pointer (must not be NULL)
 * 
 * @return 0 on success, negative error code on failure
 * @retval 0 Configuration completed successfully
 * @retval -EIO SPI communication failure
 * @retval -ENOTSUP Invalid product ID (sensor not detected or wrong type)
 * @retval -EINVAL Invalid configuration parameters
 * 
 * @note This function is called during device initialization and should not
 *       be called directly by application code.
 * 
 * @warning The sensor must be powered and SPI communication must be working
 *          before calling this function.
 */
int paw32xx_configure(const struct device *dev);

/**
 * @brief Set CPI resolution on a PAW3222 device
 *
 * Changes the sensor's CPI (Counts Per Inch) resolution, which affects
 * the sensitivity of cursor movement. Higher CPI values result in more
 * sensitive movement. The function:
 * - Validates the CPI value is within hardware limits
 * - Disables write protection on the sensor
 * - Updates both X and Y CPI registers
 * - Re-enables write protection
 *
 * @param dev PAW3222 device pointer (must not be NULL)
 * @param res_cpi CPI resolution value (range: 608-4826)
 *                Values below 608 will be rejected as invalid
 *                Hardware supports 16*38 to 127*38 CPI in steps of 38
 * 
 * @return 0 on success, negative error code on failure
 * @retval 0 CPI set successfully
 * @retval -EINVAL CPI value is out of valid range (< 608 or > 4826)
 * @retval -EIO SPI communication failure during register access
 * 
 * @note This function can be called at runtime to dynamically adjust
 *       sensor sensitivity. The driver automatically switches CPI for
 *       different input modes (normal vs snipe).
 * 
 * @warning Changing CPI affects all subsequent motion readings until
 *          changed again or device reset.
 */
int paw32xx_set_resolution(const struct device *dev, uint16_t res_cpi);

/**
 * @brief Set force awake mode on a PAW3222 device
 *
 * Controls the sensor's automatic sleep functionality. When force awake
 * mode is enabled, the sensor will not enter sleep modes automatically,
 * ensuring immediate response to motion but consuming more power. When
 * disabled, the sensor can enter sleep modes to conserve power during
 * periods of inactivity.
 *
 * @param dev PAW3222 device pointer (must not be NULL)
 * @param enable true to enable force awake mode (disable sleep),
 *               false to allow automatic sleep modes
 * 
 * @return 0 on success, negative error code on failure
 * @retval 0 Force awake mode set successfully
 * @retval -EIO SPI communication failure during register access
 * 
 * @note This setting affects power consumption vs response time trade-off:
 *       - Force awake ON: Lower latency, higher power consumption
 *       - Force awake OFF: Higher latency, lower power consumption
 * 
 * @note This function can be called at runtime to dynamically adjust
 *       power management behavior based on usage patterns.
 */
int paw32xx_force_awake(const struct device *dev, bool enable);

#ifdef CONFIG_PM_DEVICE
/**
 * @brief Power management action handler
 *
 * Handles system power management requests for the PAW3222 device.
 * This function is called by the Zephyr power management subsystem
 * to suspend or resume the device during system power state changes.
 *
 * Supported actions:
 * - PM_DEVICE_ACTION_SUSPEND: Put sensor into power-down mode and
 *   optionally disable power GPIO if configured
 * - PM_DEVICE_ACTION_RESUME: Wake sensor from power-down mode and
 *   optionally enable power GPIO if configured
 *
 * @param dev PAW3222 device pointer (must not be NULL)
 * @param action Power management action to perform
 * 
 * @return 0 on success, negative error code on failure
 * @retval 0 Power management action completed successfully
 * @retval -ENOTSUP Unsupported power management action
 * @retval -EIO SPI communication failure during power state change
 * 
 * @note This function is called automatically by the power management
 *       subsystem and should not be called directly by application code.
 * 
 * @note If power-gpios is configured in device tree, this function will
 *       also control the external power supply to the sensor.
 */
int paw32xx_pm_action(const struct device *dev, enum pm_device_action action);
#endif

#endif /* PAW3222_POWER_H_ */