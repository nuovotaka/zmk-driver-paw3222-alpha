// behavior_accel_toggle.c

#include <zephyr/device.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zephyr/logging/log.h>
#include "../include/paw3222.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct accel_toggle_config {
    const char *dev_label;
};

static int behavior_accel_toggle_pressed(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    const struct accel_toggle_config *cfg = binding->param1;
    const struct device *paw32xx_dev = device_get_binding(cfg->dev_label);
    if (paw32xx_dev && device_is_ready(paw32xx_dev)) {
        paw32xx_toggle_accel_move_enable(paw32xx_dev);
        LOG_INF("paw32xx accel toggle called for %s", cfg->dev_label);
    } else {
        LOG_ERR("paw32xx device %s not found or not ready", cfg->dev_label);
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_accel_toggle_released(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

const struct behavior_driver behavior_accel_toggle = {
    .binding_pressed = behavior_accel_toggle_pressed,
    .binding_released = behavior_accel_toggle_released,
};
