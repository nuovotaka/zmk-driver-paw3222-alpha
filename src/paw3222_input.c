/*
 * Copyright 2024 Google LLC
 * Modifications Copyright 2025 sekigon-gonnoc
 * Modifications Copyright 2025 nuovotaka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zmk/keymap.h>

// Utility macros
#ifndef CLAMP
#define CLAMP(val, low, high)                                                  \
  (((val) < (low)) ? (low) : (((val) > (high)) ? (high) : (val)))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#include "paw3222.h"
#include "paw3222_input.h"
#include "paw3222_power.h"
#include "paw3222_regs.h"
#include "paw3222_spi.h"

LOG_MODULE_DECLARE(paw32xx);

/**
 * @brief Calculate absolute value of int16_t (memory optimized)
 *
 * Computes the absolute value of a 16-bit signed integer without using
 * the standard library abs() function. This inline function is optimized
 * for memory usage and performance in the motion processing path.
 *
 * @param value Input signed 16-bit integer
 * 
 * @return Absolute value of the input
 * 
 * @note This function handles the INT16_MIN case correctly by returning
 *       the positive equivalent without overflow issues.
 */
static inline int16_t abs_int16(int16_t value) {
  return (value < 0) ? -value : value;
}

enum paw32xx_input_mode
get_input_mode_for_current_layer(const struct device *dev) {
  const struct paw32xx_config *cfg = dev->config;
  struct paw32xx_data *data = dev->data;

  // Check if using behavior-based switching instead of layer-based
  if (cfg->switch_method != PAW32XX_SWITCH_LAYER) {
    // Convert current mode state to input mode enum
    switch (data->current_mode) {
    case PAW32XX_MODE_SCROLL:
      return PAW32XX_SCROLL;
    case PAW32XX_MODE_SCROLL_HORIZONTAL:
      return PAW32XX_SCROLL_HORIZONTAL;
    case PAW32XX_MODE_SNIPE:
      return PAW32XX_SNIPE;
    case PAW32XX_MODE_SCROLL_SNIPE:
      return PAW32XX_SCROLL_SNIPE;
    case PAW32XX_MODE_SCROLL_HORIZONTAL_SNIPE:
      return PAW32XX_SCROLL_HORIZONTAL_SNIPE;
    default:
      return PAW32XX_MOVE;
    }
  }

  // Original layer-based switching logic
  uint8_t curr_layer = zmk_keymap_highest_layer_active();

  // High-precision horizontal scroll (snipe)
  if (cfg->scroll_horizontal_snipe_layers &&
      cfg->scroll_horizontal_snipe_layers_len > 0) {
    for (size_t i = 0; i < cfg->scroll_horizontal_snipe_layers_len; i++) {
      if (curr_layer == cfg->scroll_horizontal_snipe_layers[i]) {
        return PAW32XX_SCROLL_HORIZONTAL_SNIPE;
      }
    }
  }
  // High-precision vertical scroll (snipe)
  if (cfg->scroll_snipe_layers && cfg->scroll_snipe_layers_len > 0) {
    for (size_t i = 0; i < cfg->scroll_snipe_layers_len; i++) {
      if (curr_layer == cfg->scroll_snipe_layers[i]) {
        return PAW32XX_SCROLL_SNIPE;
      }
    }
  }
  // Horizontal scroll
  if (cfg->scroll_horizontal_layers && cfg->scroll_horizontal_layers_len > 0) {
    for (size_t i = 0; i < cfg->scroll_horizontal_layers_len; i++) {
      if (curr_layer == cfg->scroll_horizontal_layers[i]) {
        return PAW32XX_SCROLL_HORIZONTAL;
      }
    }
  }
  // Vertical scroll
  if (cfg->scroll_layers && cfg->scroll_layers_len > 0) {
    for (size_t i = 0; i < cfg->scroll_layers_len; i++) {
      if (curr_layer == cfg->scroll_layers[i]) {
        return PAW32XX_SCROLL;
      }
    }
  }
  // High-precision cursor movement (snipe)
  if (cfg->snipe_layers && cfg->snipe_layers_len > 0) {
    for (size_t i = 0; i < cfg->snipe_layers_len; i++) {
      if (curr_layer == cfg->snipe_layers[i]) {
        return PAW32XX_SNIPE;
      }
    }
  }
  return PAW32XX_MOVE;
}

/**
 * @brief Calculate scroll Y coordinate based on sensor rotation
 *
 * Transforms the raw sensor coordinates to ensure that Y-axis movement
 * always triggers scrolling regardless of the physical sensor orientation.
 * This allows the sensor to be mounted at different angles while maintaining
 * consistent scroll behavior.
 *
 * @param x Raw X coordinate from sensor
 * @param y Raw Y coordinate from sensor  
 * @param rotation Physical sensor rotation in degrees (0, 90, 180, 270)
 * 
 * @return Transformed Y coordinate for scroll calculations
 * 
 * @note For cursor movement, use ZMK input-processors like zip_xy_transform
 *       instead of this function. This is specifically for scroll modes.
 * 
 * @note Handles INT16_MIN overflow case to prevent undefined behavior
 *       when negating the minimum signed integer value.
 */
static int16_t calculate_scroll_y(int16_t x, int16_t y, uint16_t rotation) {
  switch (rotation) {
  case 0:
    return y;
  case 90:
    return x;
  case 180:
    return (y == INT16_MIN) ? INT16_MAX : -y;
  case 270:
    return (x == INT16_MIN) ? INT16_MAX : -x;
  default:
    return y;
  }
}

void paw32xx_motion_timer_handler(struct k_timer *timer) {
  struct paw32xx_data *data =
      CONTAINER_OF(timer, struct paw32xx_data, motion_timer);
  k_work_submit(&data->motion_work);
}

void paw32xx_motion_work_handler(struct k_work *work) {
  struct paw32xx_data *data =
      CONTAINER_OF(work, struct paw32xx_data, motion_work);
  const struct device *dev = data->dev;
  const struct paw32xx_config *cfg = dev->config;
  uint8_t val;
  int16_t x, y;
  int ret;
  bool irq_disabled = true;

  ret = paw32xx_read_reg(dev, PAW32XX_MOTION, &val);
  if (ret < 0) {
    LOG_ERR("Motion register read failed: %d", ret);
    goto cleanup;
  }

  if ((val & MOTION_STATUS_MOTION) == 0x00) {
    gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
    irq_disabled = false;
    if (gpio_pin_get_dt(&cfg->irq_gpio) == 0) {
      return;
    }
  }

  ret = paw32xx_read_xy(dev, &x, &y);
  if (ret < 0) {
    LOG_ERR("XY data read failed: %d", ret);
    goto cleanup;
  }

  // For scroll modes, we need to transform coordinates based on rotation
  // to ensure y-axis movement always triggers scroll regardless of sensor
  // orientation
  int16_t scroll_y = calculate_scroll_y(x, y, cfg->rotation);

  // Debug log
  LOG_DBG("x=%d y=%d scroll_y=%d rotation=%d", x, y, scroll_y, cfg->rotation);

  enum paw32xx_input_mode input_mode = get_input_mode_for_current_layer(dev);

  // CPI Switching
  int16_t target_cpi = cfg->res_cpi;
  if (input_mode == PAW32XX_SNIPE) {
    // Use snipe_cpi if configured, otherwise use default from Kconfig
    target_cpi =
        (cfg->snipe_cpi > 0) ? cfg->snipe_cpi : CONFIG_PAW3222_SNIPE_CPI;
  }
  if (data->current_cpi != target_cpi) {
    ret = paw32xx_set_resolution(dev, target_cpi);
    if (ret == 0) {
      data->current_cpi = target_cpi;
    } else {
      LOG_WRN("Failed to set CPI to %d: %d", target_cpi, ret);
    }
  }

  switch (input_mode) {
  case PAW32XX_MOVE: { // Normal cursor movement
    // Send raw X/Y movement - let input-processors handle rotation
    input_report_rel(data->dev, INPUT_REL_X, x, false, K_NO_WAIT);
    input_report_rel(data->dev, INPUT_REL_Y, y, true, K_FOREVER);
    break;
  }
  case PAW32XX_SNIPE: { // High-precision cursor movement
    // Apply additional precision scaling for snipe mode
    // Reduce movement by configurable divisor for ultra-precision
    uint8_t divisor = MAX(1, cfg->snipe_divisor); // Prevent division by zero
    int16_t snipe_x = x / divisor;
    int16_t snipe_y = y / divisor;

    input_report_rel(data->dev, INPUT_REL_X, snipe_x, false, K_NO_WAIT);
    input_report_rel(data->dev, INPUT_REL_Y, snipe_y, true, K_FOREVER);
    break;
  }
  case PAW32XX_SCROLL: // Vertical scroll
    // Accumulate scroll movement for smoother scrolling (with overflow
    // protection)
    {
      int32_t temp = (int32_t)data->scroll_accumulator + scroll_y;
      if (temp > INT16_MAX) {
        LOG_DBG("Scroll accumulator overflow: %d, clamped to %d", temp, INT16_MAX);
        data->scroll_accumulator = INT16_MAX;
      } else if (temp < INT16_MIN) {
        LOG_DBG("Scroll accumulator underflow: %d, clamped to %d", temp, INT16_MIN);
        data->scroll_accumulator = INT16_MIN;
      } else {
        data->scroll_accumulator = (int16_t)temp;
      }

      // Send scroll event when accumulator exceeds threshold
      if (abs_int16(data->scroll_accumulator) >= cfg->scroll_tick) {
        int16_t scroll_direction = (data->scroll_accumulator > 0) ? 1 : -1;
        input_report_rel(data->dev, INPUT_REL_WHEEL, scroll_direction, true,
                         K_FOREVER);
        data->scroll_accumulator -= scroll_direction * cfg->scroll_tick;
      }
    }
    break;
  case PAW32XX_SCROLL_HORIZONTAL: // Horizontal scroll
    // Accumulate scroll movement for smoother scrolling (with overflow
    // protection)
    {
      int32_t temp = (int32_t)data->scroll_accumulator + scroll_y;
      if (temp > INT16_MAX) {
        LOG_DBG("Scroll accumulator overflow: %d, clamped to %d", temp, INT16_MAX);
        data->scroll_accumulator = INT16_MAX;
      } else if (temp < INT16_MIN) {
        LOG_DBG("Scroll accumulator underflow: %d, clamped to %d", temp, INT16_MIN);
        data->scroll_accumulator = INT16_MIN;
      } else {
        data->scroll_accumulator = (int16_t)temp;
      }

      // Send scroll event when accumulator exceeds threshold
      if (abs_int16(data->scroll_accumulator) >= cfg->scroll_tick) {
        int16_t scroll_direction = (data->scroll_accumulator > 0) ? 1 : -1;
        input_report_rel(data->dev, INPUT_REL_HWHEEL, scroll_direction, true,
                         K_FOREVER);
        data->scroll_accumulator -= scroll_direction * cfg->scroll_tick;
      }
    }
    break;
  case PAW32XX_SCROLL_SNIPE: // High-precision vertical scroll
    // Apply precision scaling and use higher threshold for ultra-precision
    // scrolling
    {
      uint8_t divisor = MAX(1, cfg->scroll_snipe_divisor);
      int16_t snipe_scroll_y = scroll_y / divisor;
      int32_t temp = (int32_t)data->scroll_accumulator + snipe_scroll_y;
      if (temp > INT16_MAX) {
        LOG_DBG("Scroll accumulator overflow: %d, clamped to %d", temp, INT16_MAX);
        data->scroll_accumulator = INT16_MAX;
      } else if (temp < INT16_MIN) {
        LOG_DBG("Scroll accumulator underflow: %d, clamped to %d", temp, INT16_MIN);
        data->scroll_accumulator = INT16_MIN;
      } else {
        data->scroll_accumulator = (int16_t)temp;
      }

      // Use higher threshold for snipe mode
      uint8_t threshold = cfg->scroll_snipe_tick;
      if (abs_int16(data->scroll_accumulator) >= threshold) {
        int16_t scroll_direction = (data->scroll_accumulator > 0) ? 1 : -1;
        input_report_rel(data->dev, INPUT_REL_WHEEL, scroll_direction, true,
                         K_FOREVER);
        data->scroll_accumulator -= scroll_direction * threshold;
      }
    }
    break;
  case PAW32XX_SCROLL_HORIZONTAL_SNIPE: // High-precision horizontal scroll
    // Apply precision scaling and use higher threshold for ultra-precision
    // scrolling
    {
      uint8_t divisor = MAX(1, cfg->scroll_snipe_divisor);
      int16_t snipe_scroll_y = scroll_y / divisor;
      int32_t temp = (int32_t)data->scroll_accumulator + snipe_scroll_y;
      if (temp > INT16_MAX) {
        LOG_DBG("Scroll accumulator overflow: %d, clamped to %d", temp, INT16_MAX);
        data->scroll_accumulator = INT16_MAX;
      } else if (temp < INT16_MIN) {
        LOG_DBG("Scroll accumulator underflow: %d, clamped to %d", temp, INT16_MIN);
        data->scroll_accumulator = INT16_MIN;
      } else {
        data->scroll_accumulator = (int16_t)temp;
      }

      // Use higher threshold for snipe mode
      uint8_t threshold = cfg->scroll_snipe_tick;
      if (abs_int16(data->scroll_accumulator) >= threshold) {
        int16_t scroll_direction = (data->scroll_accumulator > 0) ? 1 : -1;
        input_report_rel(data->dev, INPUT_REL_HWHEEL, scroll_direction, true,
                         K_FOREVER);
        data->scroll_accumulator -= scroll_direction * threshold;
      }
    }
    break;

  default:
    LOG_ERR("Unknown input_mode: %d", input_mode);
    break;
  }

  k_timer_start(&data->motion_timer, K_MSEC(15), K_NO_WAIT);
  return;

cleanup:
  if (irq_disabled) {
    gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
  }
}

void paw32xx_motion_handler(const struct device *gpio_dev,
                            struct gpio_callback *cb, uint32_t pins) {
  ARG_UNUSED(gpio_dev);
  ARG_UNUSED(pins);
  struct paw32xx_data *data = CONTAINER_OF(cb, struct paw32xx_data, motion_cb);
  const struct device *dev = data->dev;
  const struct paw32xx_config *cfg = dev->config;

  gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_DISABLE);
  k_timer_stop(&data->motion_timer);
  k_work_submit(&data->motion_work);
}