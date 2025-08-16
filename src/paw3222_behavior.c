/*
 * Copyright 2025 nuovotaka
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_PAW3222_BEHAVIOR

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>

#include "paw3222.h"
#include "paw3222_input.h"

LOG_MODULE_REGISTER(paw32xx_behavior, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT paw32xx_mode

// Forward declarations
static int on_paw32xx_mode_binding_pressed(
    struct zmk_behavior_binding *binding,
    struct zmk_behavior_binding_event binding_event);
static int on_paw32xx_mode_binding_released(
    struct zmk_behavior_binding *binding,
    struct zmk_behavior_binding_event binding_event);

// Global pointer to the PAW3222 device (set during init)
static const struct device *paw3222_dev = NULL;

// Function to set the PAW3222 device reference
void paw32xx_set_device_reference(const struct device *dev) {
  paw3222_dev = dev;
}

// Function to cycle through input modes
static int paw32xx_cycle_mode(void) {
  if (!paw3222_dev) {
    LOG_ERR("PAW3222 device not initialized");
    return -ENODEV;
  }

  struct paw32xx_data *data = paw3222_dev->data;

  // Cycle through available modes
  switch (data->current_mode) {
  case PAW32XX_MODE_MOVE:
    data->current_mode = PAW32XX_MODE_SCROLL;
    LOG_INF("Switched to SCROLL mode");
    break;
  case PAW32XX_MODE_SCROLL:
    data->current_mode = PAW32XX_MODE_SCROLL_HORIZONTAL;
    LOG_INF("Switched to SCROLL_HORIZONTAL mode");
    break;
  case PAW32XX_MODE_SCROLL_HORIZONTAL:
    data->current_mode = PAW32XX_MODE_SNIPE;
    LOG_INF("Switched to SNIPE mode");
    break;
  case PAW32XX_MODE_SNIPE:
    data->current_mode = PAW32XX_MODE_SCROLL_SNIPE;
    LOG_INF("Switched to SCROLL_SNIPE mode");
    break;
  case PAW32XX_MODE_SCROLL_SNIPE:
    data->current_mode = PAW32XX_MODE_SCROLL_HORIZONTAL_SNIPE;
    LOG_INF("Switched to SCROLL_HORIZONTAL_SNIPE mode");
    break;
  default:
    data->current_mode = PAW32XX_MODE_MOVE;
    LOG_INF("Switched to MOVE mode");
    break;
  }

  return 0;
}

// Toggle between move and scroll modes
static int paw32xx_toggle_mode(void) {
  if (!paw3222_dev) {
    LOG_ERR("PAW3222 device not initialized");
    return -ENODEV;
  }

  struct paw32xx_data *data = paw3222_dev->data;

  if (data->current_mode == PAW32XX_MODE_MOVE) {
    data->current_mode = PAW32XX_MODE_SCROLL;
    LOG_INF("Switched to SCROLL mode");
  } else {
    data->current_mode = PAW32XX_MODE_MOVE;
    LOG_INF("Switched to MOVE mode");
  }

  return 0;
}

// Behavior implementation for PAW3222 mode switching

static int on_paw32xx_mode_binding_pressed(
    struct zmk_behavior_binding *binding,
    struct zmk_behavior_binding_event binding_event) {
  uint32_t param = binding->param1;

  switch (param) {
  case 0: // Toggle mode
    return paw32xx_toggle_mode();
  case 1: // Cycle mode
    return paw32xx_cycle_mode();
  default:
    LOG_ERR("Unknown PAW3222 mode parameter: %d", param);
    return -EINVAL;
  }
}

static int on_paw32xx_mode_binding_released(
    struct zmk_behavior_binding *binding,
    struct zmk_behavior_binding_event binding_event) {
  // No action on release for toggle/cycle modes
  return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_paw32xx_mode_driver_api = {
    .binding_pressed = on_paw32xx_mode_binding_pressed,
    .binding_released = on_paw32xx_mode_binding_released,
};

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int behavior_paw32xx_mode_init(const struct device *dev) { return 0; }

#define PAW32XX_MODE_INST(n)                                                   \
  BEHAVIOR_DT_INST_DEFINE(n, behavior_paw32xx_mode_init, NULL, NULL, NULL,     \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,    \
                          &behavior_paw32xx_mode_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PAW32XX_MODE_INST)

#endif