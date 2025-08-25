/*
 * Copyright 2024 Google LLC
 * Modifications Copyright 2025 sekigon-gonnoc
 * Modifications Copyright 2025 nuovotaka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PAW3222_SPI_H_
#define PAW3222_SPI_H_

#include <stdint.h>
#include <zephyr/device.h>

/**
 * @brief Read a register from the PAW3222 sensor
 *
 * @param dev PAW3222 device
 * @param addr Register address (0x00-0x0E)
 * @param value Pointer to store the read value (must not be NULL)
 * @return 0 on success, negative error code on failure
 * @retval -ENODEV if device is not ready
 * @retval -EIO if SPI communication fails
 */
int paw32xx_read_reg(const struct device *dev, uint8_t addr, uint8_t *value);

/**
 * @brief Write a register to the PAW3222 sensor
 *
 * @param dev PAW3222 device
 * @param addr Register address (0x00-0x0E)
 * @param value Value to write (0x00-0xFF)
 * @return 0 on success, negative error code on failure
 * @retval -ENODEV if device is not ready
 * @retval -EIO if SPI communication fails
 */
int paw32xx_write_reg(const struct device *dev, uint8_t addr, uint8_t value);

/**
 * @brief Update specific bits in a register
 *
 * @param dev PAW3222 device
 * @param addr Register address
 * @param mask Bit mask for the bits to update
 * @param value New value for the masked bits
 * @return 0 on success, negative error code on failure
 */
int paw32xx_update_reg(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t value);

/**
 * @brief Read X and Y delta values from the sensor
 *
 * @param dev PAW3222 device
 * @param x Pointer to store X delta (must not be NULL, range: -128 to 127)
 * @param y Pointer to store Y delta (must not be NULL, range: -128 to 127)
 * @return 0 on success, negative error code on failure
 * @retval -ENODEV if device is not ready
 * @retval -EIO if SPI communication fails
 */
int paw32xx_read_xy(const struct device *dev, int16_t *x, int16_t *y);

#endif /* PAW3222_SPI_H_ */