/*
 * Copyright 2024 Google LLC
 * Modifications Copyright 2025 sekigon-gonnoc
 * Modifications Copyright 2025 nuovotaka
 * Original source code:
 * https://github.com/zephyrproject-rtos/zephyr/blob/19c6240b6865bcb28e1d786d4dcadfb3a02067a0/include/zephyr/input/input_paw32xx.h
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INPUT_PAW32XX_H_
#define ZEPHYR_INCLUDE_INPUT_PAW32XX_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

/* These functions are declared in paw3222_power.h */

/**
 * @brief Input mode switching methods
 * 
 * Defines how the PAW3222 driver switches between different input modes.
 */
enum paw32xx_mode_switch_method {
  PAW32XX_SWITCH_LAYER,  /**< Original layer-based switching using ZMK layers */
  PAW32XX_SWITCH_TOGGLE, /**< Toggle key based switching using behavior API */
};

/**
 * @brief Current input mode state
 * 
 * Represents the current operational mode of the PAW3222 sensor.
 * Each mode affects how motion data is interpreted and reported.
 */
enum paw32xx_current_mode {
  PAW32XX_MODE_MOVE,                    /**< Standard cursor movement mode */
  PAW32XX_MODE_SCROLL,                  /**< Vertical scrolling mode */
  PAW32XX_MODE_SCROLL_HORIZONTAL,       /**< Horizontal scrolling mode */
  PAW32XX_MODE_SNIPE,                   /**< High-precision cursor movement mode */
  PAW32XX_MODE_SCROLL_SNIPE,            /**< High-precision vertical scrolling mode */
  PAW32XX_MODE_SCROLL_HORIZONTAL_SNIPE, /**< High-precision horizontal scrolling mode */
};

/**
 * @brief PAW3222 device configuration structure
 *
 * Contains all configuration parameters for the PAW3222 sensor driver.
 * This structure is populated from device tree properties during initialization.
 * 
 * @note This struct is typically used internally by the driver but may be
 *       referenced for advanced configuration or debugging purposes.
 */
struct paw32xx_config {
  struct spi_dt_spec spi;                      /**< SPI device specification from device tree */
  struct gpio_dt_spec irq_gpio;                /**< Motion interrupt GPIO specification */
  struct gpio_dt_spec power_gpio;              /**< Power control GPIO specification (optional) */
  
  /* Layer-based mode switching configuration */
  size_t scroll_layers_len;                    /**< Number of scroll layers defined */
  int32_t *scroll_layers;                      /**< Array of layer IDs for vertical scroll mode */
  size_t snipe_layers_len;                     /**< Number of snipe layers defined */
  int32_t *snipe_layers;                       /**< Array of layer IDs for snipe mode */
  size_t scroll_horizontal_layers_len;         /**< Number of horizontal scroll layers defined */
  int32_t *scroll_horizontal_layers;           /**< Array of layer IDs for horizontal scroll mode */
  size_t scroll_snipe_layers_len;              /**< Number of scroll snipe layers defined */
  int32_t *scroll_snipe_layers;                /**< Array of layer IDs for high-precision vertical scroll */
  size_t scroll_horizontal_snipe_layers_len;   /**< Number of horizontal scroll snipe layers defined */
  int32_t *scroll_horizontal_snipe_layers;     /**< Array of layer IDs for high-precision horizontal scroll */
  
  /* Sensor configuration */
  int16_t res_cpi;                             /**< Default CPI resolution (608-4826) */
  int16_t snipe_cpi;                           /**< CPI resolution for snipe mode */
  uint8_t snipe_divisor;                       /**< Additional precision divisor for snipe mode (default: 2) */
  uint8_t scroll_snipe_divisor;                /**< Additional precision divisor for scroll snipe mode */
  uint8_t scroll_snipe_tick;                   /**< Scroll tick threshold for snipe mode */
  bool force_awake;                            /**< Force sensor to stay awake (disable sleep modes) */
  uint16_t rotation;                           /**< Physical sensor rotation angle (0, 90, 180, 270 degrees) */
  uint8_t scroll_tick;                         /**< Scroll tick threshold for normal scroll modes */

  /* Mode switching configuration */
  enum paw32xx_mode_switch_method switch_method; /**< Method used for input mode switching */
};

/**
 * @brief PAW3222 runtime data structure
 *
 * Contains all runtime state and working data for the PAW3222 driver.
 * This structure is used internally by the driver to maintain sensor state
 * and handle motion processing.
 * 
 * @note This struct is for internal driver use only and should not be
 *       accessed directly by application code.
 */
struct paw32xx_data {
  const struct device *dev;                   /**< Pointer to the device instance */
  struct k_work motion_work;                  /**< Work queue item for motion processing */
  struct gpio_callback motion_cb;             /**< GPIO callback for motion interrupt */
  struct k_timer motion_timer;                /**< Timer for motion processing timeout */
  int16_t current_cpi;                        /**< Currently configured CPI value */
  int16_t scroll_accumulator;                 /**< Accumulator for smooth scrolling (reduced from int32_t) */

  /* Mode switching state */
  enum paw32xx_current_mode current_mode;     /**< Current operational mode of the sensor */
  bool mode_toggle_state;                     /**< Toggle state for behavior-based mode switching */
};

#endif /* ZEPHYR_INCLUDE_INPUT_PAW32XX_H_ */
