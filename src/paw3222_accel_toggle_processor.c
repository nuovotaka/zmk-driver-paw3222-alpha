// behavior_accel_toggle.c

#include <zephyr/device.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zephyr/logging/log.h>
#include "../include/paw3222.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// paw3222.c で定義されているデバイスリストをexternで参照
extern const struct device *paw32xx_devs[];
extern int paw32xx_dev_count;

static int behavior_accel_toggle_pressed(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    int dev_idx = binding->param1; // キーマップで指定したインデックス
    if (dev_idx < 0 || dev_idx >= paw32xx_dev_count) {
        LOG_ERR("paw32xx device index %d out of range", dev_idx);
        return ZMK_BEHAVIOR_OPAQUE;
    }
    const struct device *paw32xx_dev = paw32xx_devs[dev_idx];
    if (paw32xx_dev && device_is_ready(paw32xx_dev)) {
        paw32xx_toggle_accel_move_enable(paw32xx_dev);
        LOG_INF("paw32xx accel toggle called for dev_idx=%d", dev_idx);
    } else {
        LOG_ERR("paw32xx device at index %d not found or not ready", dev_idx);
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

ZMK_BEHAVIOR(behavior_accel_toggle);
