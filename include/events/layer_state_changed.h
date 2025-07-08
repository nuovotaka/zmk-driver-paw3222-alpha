#pragma once

#include "../events/event_manager.h"
#include <zephyr/kernel.h>

struct layer_state_changed {
    struct zmk_event_header header;
    uint8_t layer;
    bool state;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(layer_state_changed);

static inline struct layer_state_changed *create_layer_state_changed(uint8_t layer, bool state) {
    struct layer_state_changed *ev = new_layer_state_changed();
    ev->layer = layer;
    ev->state = state;
    ev->timestamp = k_uptime_get();
    return ev;
}
