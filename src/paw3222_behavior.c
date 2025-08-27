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

/**
 * @brief Set the global PAW3222 device reference for behavior system
 *
 * Stores a global reference to the PAW3222 device instance for use by the
 * behavior driver system. This allows behavior key bindings to control the
 * sensor's input mode without requiring direct device access.
 *
 * @param dev PAW3222 device pointer to store globally
 * 
 * @note This function is called automatically during device initialization
 *       when CONFIG_PAW3222_BEHAVIOR is enabled.
 * 
 * @warning Only supports a single PAW3222 device instance when using behaviors.
 *          Multiple devices would overwrite the global reference.
 */
void paw32xx_set_device_reference(const struct device *dev)
{
    paw3222_dev = dev;
}

/**
 * @brief Change the PAW3222 input mode and log the change
 *
 * Updates the current input mode of the PAW3222 sensor and logs the change
 * for debugging purposes. This is a helper function used by the various
 * toggle mode functions.
 *
 * @param new_mode The new input mode to set
 * 
 * @return 0 on success, negative error code on failure
 * @retval 0 Mode changed successfully
 * @retval -ENODEV PAW3222 device not initialized or not available
 * 
 * @note This function updates the mode immediately and the change takes
 *       effect on the next motion event.
 */
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

/**
 * @brief Toggle between cursor movement and scroll modes
 *
 * Switches between cursor movement modes (MOVE/SNIPE) and scroll modes
 * (SCROLL/SCROLL_SNIPE/SCROLL_HORIZONTAL/SCROLL_HORIZONTAL_SNIPE).
 * 
 * Mode transitions:
 * - From MOVE or SNIPE: Switch to SCROLL
 * - From any SCROLL mode: Switch to MOVE
 *
 * @return 0 on success, negative error code on failure
 * @retval 0 Mode toggled successfully
 * @retval -ENODEV PAW3222 device not initialized
 * 
 * @note This implements parameter 0 of the paw_mode behavior
 */
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

/**
 * @brief Toggle between normal and high-precision (snipe) modes
 *
 * Toggles the high-precision (snipe) mode for the current operation type.
 * This affects sensitivity but maintains the same operation (move vs scroll).
 * 
 * Mode transitions:
 * - MOVE ↔ SNIPE (cursor movement)
 * - SCROLL ↔ SCROLL_SNIPE (vertical scrolling)
 * - SCROLL_HORIZONTAL ↔ SCROLL_HORIZONTAL_SNIPE (horizontal scrolling)
 *
 * @return 0 on success, negative error code on failure
 * @retval 0 Mode toggled successfully
 * @retval -ENODEV PAW3222 device not initialized or unsupported mode
 * 
 * @note This implements parameter 1 of the paw_mode behavior
 */
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

/**
 * @brief Toggle between vertical and horizontal scroll directions
 *
 * Switches scroll direction between vertical and horizontal while maintaining
 * the same precision level (normal vs snipe). Only works when already in a
 * scroll mode - has no effect in cursor movement modes.
 * 
 * Mode transitions:
 * - SCROLL ↔ SCROLL_HORIZONTAL
 * - SCROLL_SNIPE ↔ SCROLL_HORIZONTAL_SNIPE
 * - MOVE/SNIPE: No effect (logs info message)
 *
 * @return 0 on success, negative error code on failure
 * @retval 0 Mode toggled successfully
 * @retval -ENODEV PAW3222 device not initialized, not in scroll mode, or unsupported mode
 * 
 * @note This implements parameter 2 of the paw_mode behavior
 */
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

/**
 * @brief Handle PAW3222 mode behavior key press events
 *
 * Called when a paw_mode behavior key is pressed. Dispatches to the appropriate
 * mode toggle function based on the behavior parameter.
 *
 * Supported parameters:
 * - 0: Move/Scroll toggle
 * - 1: Normal/Snipe toggle  
 * - 2: Vertical/Horizontal toggle
 *
 * @param binding Pointer to the behavior binding containing parameters
 * @param binding_event Event information (unused)
 * 
 * @return 0 on success, negative error code on failure
 * @retval 0 Mode change completed successfully
 * @retval -EINVAL Unknown parameter value
 * @retval -ENODEV PAW3222 device not available or mode change failed
 */
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

/**
 * @brief Handle PAW3222 mode behavior key release events
 *
 * Called when a paw_mode behavior key is released. For toggle-based behaviors,
 * no action is typically needed on key release, so this function serves as
 * a placeholder for the behavior API.
 *
 * @param binding Pointer to the behavior binding containing parameters
 * @param binding_event Event information (unused)
 * 
 * @return Always returns 0 (no action needed for toggle behaviors)
 */
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

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

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

/**
 * @brief Initialize the PAW3222 mode behavior driver
 *
 * Initializes the behavior driver for PAW3222 mode switching. This function
 * is called during system initialization to set up the behavior system.
 *
 * @param dev Behavior device instance (unused)
 * 
 * @return Always returns 0 (initialization always succeeds)
 * 
 * @note The actual PAW3222 device reference is set separately during
 *       PAW3222 device initialization via paw32xx_set_device_reference().
 */
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