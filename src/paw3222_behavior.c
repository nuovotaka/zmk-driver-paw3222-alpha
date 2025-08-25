/*
 * Copyright 2025 nuovotaka
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_PAW3222_BEHAVIOR
#include <drivers/behavior.h>
#endif

#include "paw3222.h"
#include "paw3222_input.h"

LOG_MODULE_REGISTER(paw32xx_behavior, CONFIG_ZMK_LOG_LEVEL);

#ifdef CONFIG_PAW3222_BEHAVIOR

#define DT_DRV_COMPAT paw32xx_mode

// Global pointer to the PAW3222 device (set during init)
static const struct device *paw3222_dev = NULL;

// Function to set the PAW3222 device reference
void paw32xx_set_device_reference(const struct device *dev)
{
    paw3222_dev = dev;
}

// Helper function to change mode
static int paw32xx_change_mode(enum paw32xx_current_mode new_mode)
{
    if (!paw3222_dev) {
        LOG_ERR("PAW3222 device not initialized");
        return -ENODEV;
    }

    struct paw32xx_data *data = paw3222_dev->data;
    data->current_mode = new_mode;

    const char* mode_names[] = {
        "MOVE", "SCROLL", "SCROLL_HORIZONTAL",
        "SNIPE", "SCROLL_SNIPE", "SCROLL_HORIZONTAL_SNIPE"
    };

    if ((int)new_mode >= 0 && new_mode < ARRAY_SIZE(mode_names)) {
        LOG_INF("Switched to %s mode", mode_names[new_mode]);
    }

    return 0;
}

// Move Scroll Toggle between move and scroll modes
static int paw32xx_move_scroll_toggle_mode(void)
{
    if (!paw3222_dev) {
        LOG_ERR("PAW3222 device not initialized");
        return -ENODEV;
    }

    struct paw32xx_data *data = paw3222_dev->data;

    switch (data->current_mode) {
        case PAW32XX_MODE_MOVE:
        case PAW32XX_MODE_SNIPE:
            return paw32xx_change_mode(PAW32XX_MODE_SCROLL);
        case PAW32XX_MODE_SCROLL:
        case PAW32XX_MODE_SCROLL_HORIZONTAL:
        case PAW32XX_MODE_SCROLL_SNIPE:
        case PAW32XX_MODE_SCROLL_HORIZONTAL_SNIPE:
            return paw32xx_change_mode(PAW32XX_MODE_MOVE);
        default:
            LOG_ERR("Unsupported mode");
            return -ENODEV;
    }
}

// Normal Snipe Toggle between move and scroll modes
static int paw32xx_normal_snipe_toggle_mode(void)
{
    if (!paw3222_dev) {
        LOG_ERR("PAW3222 device not initialized");
        return -ENODEV;
    }

    struct paw32xx_data *data = paw3222_dev->data;

    switch (data->current_mode) {
        case PAW32XX_MODE_MOVE:
            return paw32xx_change_mode(PAW32XX_MODE_SNIPE);
        case PAW32XX_MODE_SNIPE:
            return paw32xx_change_mode(PAW32XX_MODE_MOVE);
        case PAW32XX_MODE_SCROLL:
            return paw32xx_change_mode(PAW32XX_MODE_SCROLL_SNIPE);
        case PAW32XX_MODE_SCROLL_SNIPE:
            return paw32xx_change_mode(PAW32XX_MODE_SCROLL);
        case PAW32XX_MODE_SCROLL_HORIZONTAL:
            return paw32xx_change_mode(PAW32XX_MODE_SCROLL_HORIZONTAL_SNIPE);
        case PAW32XX_MODE_SCROLL_HORIZONTAL_SNIPE:
            return paw32xx_change_mode(PAW32XX_MODE_SCROLL_HORIZONTAL);
        default:
            LOG_ERR("Unsupported mode");
            return -ENODEV;
    }
}

// Vertical Horizontal Toggle between scroll modes
static int paw32xx_vertical_horizontal_toggle_mode(void)
{
    if (!paw3222_dev) {
        LOG_ERR("PAW3222 device not initialized");
        return -ENODEV;
    }

    struct paw32xx_data *data = paw3222_dev->data;

    if (data->current_mode == PAW32XX_MODE_MOVE || data->current_mode == PAW32XX_MODE_SNIPE) {
        LOG_INF("PAW3222 not SCROLL MODE");
        return -ENODEV;
    }

    switch (data->current_mode) {
        case PAW32XX_MODE_SCROLL:
            return paw32xx_change_mode(PAW32XX_MODE_SCROLL_HORIZONTAL);
        case PAW32XX_MODE_SCROLL_SNIPE:
            return paw32xx_change_mode(PAW32XX_MODE_SCROLL_HORIZONTAL_SNIPE);
        case PAW32XX_MODE_SCROLL_HORIZONTAL:
            return paw32xx_change_mode(PAW32XX_MODE_SCROLL);
        case PAW32XX_MODE_SCROLL_HORIZONTAL_SNIPE:
            return paw32xx_change_mode(PAW32XX_MODE_SCROLL_SNIPE);
        default:
            LOG_ERR("Unsupported mode");
            return -ENODEV;
    }
}

// Behavior implementation for PAW3222 mode switching
static int on_paw32xx_mode_binding_pressed(
    struct zmk_behavior_binding *binding,
    struct zmk_behavior_binding_event binding_event)
{
    uint32_t param1 = binding->param1;

    LOG_DBG("PAW32xx mode binding pressed: param1=%d", param1);

    switch (param1) {
        case 0: // Move <-> Scroll Toggle mode
            LOG_DBG("Move <-> Scroll Toggle mode");
            return paw32xx_move_scroll_toggle_mode();
        case 1: // Normal <-> Snipe Toggle mode
            LOG_DBG("Normal <-> Snipe Toggle mode");
            return paw32xx_normal_snipe_toggle_mode();
        case 2: // Vertical <-> Horizontal mode
            LOG_DBG("Vertical <-> Horizontal mode");
            return paw32xx_vertical_horizontal_toggle_mode();
        default:
            LOG_ERR("Unknown PAW3222 mode parameter: %d", param1);
            return -EINVAL;
    }
}

static int on_paw32xx_mode_binding_released(
    struct zmk_behavior_binding *binding,
    struct zmk_behavior_binding_event binding_event)
{
    uint32_t param1 = binding->param1;

    LOG_DBG("PAW32xx mode binding released: param1=%d", param1);

    switch (param1) {
        case 0: // Toggle modes - no action on release
        case 1:
        case 2:
            return 0;
        default:
            return 0;
    }
}

static const struct behavior_driver_api behavior_paw32xx_mode_driver_api = {
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
    .binding_pressed = on_paw32xx_mode_binding_pressed,
    .binding_released = on_paw32xx_mode_binding_released,
    .sensor_binding_accept_data = NULL,
    .sensor_binding_process = NULL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = NULL,
    .parameter_metadata = NULL,
#endif
};

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int behavior_paw32xx_mode_init(const struct device *dev)
{
    LOG_DBG("PAW3222 behavior initialized");
    return 0;
}

#define PAW32XX_MODE_INST(n)                                                \
  BEHAVIOR_DT_INST_DEFINE(n, behavior_paw32xx_mode_init, NULL, NULL, NULL,  \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                          &behavior_paw32xx_mode_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PAW32XX_MODE_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY */

#endif /* CONFIG_PAW3222_BEHAVIOR */