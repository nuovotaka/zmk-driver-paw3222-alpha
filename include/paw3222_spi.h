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
 * @brief Read a register from the PAW3222 sensor via SPI
 *
 * Performs a single register read operation from the PAW3222 sensor using
 * the SPI interface. The function sends the register address and receives
 * the register value in a single SPI transaction.
 *
 * @param dev PAW3222 device pointer (must not be NULL)
 * @param addr Register address to read (valid range: 0x00-0x0E)
 * @param value Pointer to store the read value (must not be NULL)
 * 
 * @return 0 on success, negative error code on failure
 * @retval 0 Register read successfully
 * @retval -ENODEV SPI device is not ready or not available
 * @retval -EIO SPI communication failure or transaction error
 * 
 * @note This is a low-level function used by other driver components.
 *       Application code should use higher-level APIs instead.
 * 
 * @warning The caller must ensure the register address is valid for the
 *          PAW3222 sensor. Invalid addresses may cause undefined behavior.
 */
int paw32xx_read_reg(const struct device *dev, uint8_t addr, uint8_t *value);

/**
 * @brief Write a register to the PAW3222 sensor via SPI
 *
 * Performs a single register write operation to the PAW3222 sensor using
 * the SPI interface. The function sends both the register address (with
 * write bit set) and the value to write in a single SPI transaction.
 *
 * @param dev PAW3222 device pointer (must not be NULL)
 * @param addr Register address to write (valid range: 0x00-0x0E)
 * @param value Value to write to the register (0x00-0xFF)
 * 
 * @return 0 on success, negative error code on failure
 * @retval 0 Register written successfully
 * @retval -ENODEV SPI device is not ready or not available
 * @retval -EIO SPI communication failure or transaction error
 * 
 * @note This is a low-level function used by other driver components.
 *       Some registers may require write protection to be disabled first.
 * 
 * @warning Writing to certain registers may affect sensor operation.
 *          Ensure proper register values and sequences are used.
 */
int paw32xx_write_reg(const struct device *dev, uint8_t addr, uint8_t value);

/**
 * @brief Update specific bits in a PAW3222 register
 *
 * Performs a read-modify-write operation on a PAW3222 register to update
 * only specific bits while preserving others. This is useful for changing
 * configuration flags without affecting other settings in the same register.
 *
 * The operation sequence is:
 * 1. Read the current register value
 * 2. Clear bits specified by mask
 * 3. Set new bits from value (masked)
 * 4. Write the modified value back
 *
 * @param dev PAW3222 device pointer (must not be NULL)
 * @param addr Register address to update (valid range: 0x00-0x0E)
 * @param mask Bit mask specifying which bits to modify (1 = modify, 0 = preserve)
 * @param value New value for the masked bits (only masked bits are used)
 * 
 * @return 0 on success, negative error code on failure
 * @retval 0 Register updated successfully
 * @retval -ENODEV SPI device is not ready or not available
 * @retval -EIO SPI communication failure during read or write operation
 * 
 * @note This function is atomic from the driver perspective but involves
 *       multiple SPI transactions. External changes to the register between
 *       read and write operations are not protected against.
 * 
 * @example
 * // Set bit 3, clear bit 1, preserve other bits
 * paw32xx_update_reg(dev, REG_ADDR, 0x0A, 0x08);  // mask=1010, value=1000
 */
int paw32xx_update_reg(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t value);

/**
 * @brief Read X and Y motion delta values from the PAW3222 sensor
 *
 * Reads the accumulated motion data from the sensor's delta registers.
 * This function performs an optimized SPI transaction to read both X and Y
 * delta values in a single operation, then applies sign extension to convert
 * the 8-bit sensor values to signed 16-bit integers.
 *
 * The motion data represents the accumulated movement since the last read:
 * - Positive X values indicate rightward movement
 * - Negative X values indicate leftward movement  
 * - Positive Y values indicate downward movement
 * - Negative Y values indicate upward movement
 *
 * @param dev PAW3222 device pointer (must not be NULL)
 * @param x Pointer to store X delta value (must not be NULL)
 *          Range: -128 to +127 (8-bit signed, extended to 16-bit)
 * @param y Pointer to store Y delta value (must not be NULL)
 *          Range: -128 to +127 (8-bit signed, extended to 16-bit)
 * 
 * @return 0 on success, negative error code on failure
 * @retval 0 Motion data read successfully
 * @retval -ENODEV SPI device is not ready or not available
 * @retval -EIO SPI communication failure or transaction error
 * 
 * @note This function should be called when motion is detected (IRQ active)
 *       to retrieve the accumulated motion data. Reading clears the delta
 *       registers in the sensor.
 * 
 * @note The values are automatically sign-extended from 8-bit to 16-bit
 *       to maintain proper signed arithmetic in motion calculations.
 */
int paw32xx_read_xy(const struct device *dev, int16_t *x, int16_t *y);

#endif /* PAW3222_SPI_H_ */