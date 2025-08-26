/*
 * Copyright 2024 Google LLC
 * Modifications Copyright 2025 sekigon-gonnoc
 * Modifications Copyright 2025 nuovotaka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PAW3222_REGS_H_
#define ZEPHYR_INCLUDE_PAW3222_REGS_H_

#include <zephyr/sys/util.h>

/**
 * @defgroup PAW3222_REGISTERS PAW3222 Register Definitions
 * @brief Register addresses for PAW3222 optical sensor
 * @{
 */

/** @brief Product ID register 1 - Contains sensor identification */
#define PAW32XX_PRODUCT_ID1 0x00
/** @brief Product ID register 2 - Contains additional sensor identification */
#define PAW32XX_PRODUCT_ID2 0x01
/** @brief Motion status register - Indicates motion detection and data availability */
#define PAW32XX_MOTION 0x02
/** @brief X-axis motion delta register - Signed 8-bit X movement data */
#define PAW32XX_DELTA_X 0x03
/** @brief Y-axis motion delta register - Signed 8-bit Y movement data */
#define PAW32XX_DELTA_Y 0x04
/** @brief Operation mode register - Controls sleep and power modes */
#define PAW32XX_OPERATION_MODE 0x05
/** @brief Configuration register - General sensor configuration and reset */
#define PAW32XX_CONFIGURATION 0x06
/** @brief Write protection register - Controls write access to configuration registers */
#define PAW32XX_WRITE_PROTECT 0x09
/** @brief Sleep mode 1 configuration register */
#define PAW32XX_SLEEP1 0x0a
/** @brief Sleep mode 2 configuration register */
#define PAW32XX_SLEEP2 0x0b
/** @brief Sleep mode 3 configuration register */
#define PAW32XX_SLEEP3 0x0c
/** @brief X-axis CPI (resolution) configuration register */
#define PAW32XX_CPI_X 0x0d
/** @brief Y-axis CPI (resolution) configuration register */
#define PAW32XX_CPI_Y 0x0e

/** @} */

/**
 * @defgroup PAW3222_REGISTER_VALUES PAW3222 Register Values
 * @brief Predefined values for PAW3222 registers
 * @{
 */

/** @brief Expected product ID value for PAW3222 sensor */
#define PRODUCT_ID_PAW32XX 0x30
/** @brief SPI write bit - Set bit 7 for write operations */
#define SPI_WRITE BIT(7)

/** @} */

/**
 * @defgroup PAW3222_MOTION_BITS Motion Register Bit Definitions
 * @brief Bit field definitions for PAW32XX_MOTION register
 * @{
 */

/** @brief Motion detection bit - Set when new motion data is available */
#define MOTION_STATUS_MOTION BIT(7)

/** @} */

/**
 * @defgroup PAW3222_OPERATION_MODE_BITS Operation Mode Register Bit Definitions
 * @brief Bit field definitions for PAW32XX_OPERATION_MODE register
 * @{
 */

/** @brief Sleep enhancement mode 1 enable bit */
#define OPERATION_MODE_SLP_ENH BIT(4)
/** @brief Sleep enhancement mode 2 enable bit */
#define OPERATION_MODE_SLP2_ENH BIT(3)
/** @brief Combined mask for all sleep mode bits */
#define OPERATION_MODE_SLP_MASK                                                \
  (OPERATION_MODE_SLP_ENH | OPERATION_MODE_SLP2_ENH)

/** @} */

/**
 * @defgroup PAW3222_CONFIGURATION_BITS Configuration Register Bit Definitions
 * @brief Bit field definitions for PAW32XX_CONFIGURATION register
 * @{
 */

/** @brief Power down enhancement bit - Enables deep power down mode */
#define CONFIGURATION_PD_ENH BIT(3)
/** @brief Software reset bit - Triggers sensor reset when set */
#define CONFIGURATION_RESET BIT(7)

/** @} */

/**
 * @defgroup PAW3222_WRITE_PROTECTION Write Protection Values
 * @brief Values for controlling write protection on configuration registers
 * @{
 */

/** @brief Value to enable write protection (default state) */
#define WRITE_PROTECT_ENABLE 0x00
/** @brief Magic value to disable write protection (allows configuration changes) */
#define WRITE_PROTECT_DISABLE 0x5a

/** @} */

/**
 * @defgroup PAW3222_CONSTANTS PAW3222 Hardware Constants
 * @brief Hardware-specific constants and timing values
 * @{
 */

/** @brief Data size in bits for motion delta values */
#define PAW32XX_DATA_SIZE_BITS 8
/** @brief Required delay in milliseconds after sensor reset */
#define RESET_DELAY_MS 2

/** @} */

/**
 * @defgroup PAW3222_RESOLUTION PAW3222 Resolution Constants
 * @brief CPI resolution calculation constants
 * @{
 */

/** @brief CPI step size - Each CPI register increment represents 38 CPI */
#define RES_STEP 38
/** @brief Minimum supported CPI resolution (16 * 38 = 608 CPI) */
#define RES_MIN (16 * RES_STEP)
/** @brief Maximum supported CPI resolution (127 * 38 = 4826 CPI) */
#define RES_MAX (127 * RES_STEP)

/** @} */

/**
 * @brief PAW3222 input mode enumeration
 * 
 * Defines the different operational modes for interpreting motion data
 * from the PAW3222 sensor. Each mode affects how X/Y motion is processed
 * and what type of input events are generated.
 */
enum paw32xx_input_mode {
  PAW32XX_MOVE,                    /**< Standard cursor movement mode */
  PAW32XX_SCROLL,                  /**< Vertical scroll mode - Y motion generates scroll wheel events */
  PAW32XX_SCROLL_HORIZONTAL,       /**< Horizontal scroll mode - Y motion generates horizontal scroll events */
  PAW32XX_SNIPE,                   /**< High-precision cursor movement mode with reduced sensitivity */
  PAW32XX_SCROLL_SNIPE,            /**< High-precision vertical scroll mode with reduced sensitivity */
  PAW32XX_SCROLL_HORIZONTAL_SNIPE, /**< High-precision horizontal scroll mode with reduced sensitivity */
};

#endif /* ZEPHYR_INCLUDE_PAW3222_REGS_H_ */