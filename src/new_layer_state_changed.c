struct layer_state_changed *new_layer_state_changed(void) {
    struct layer_state_changed *ev = k_malloc(sizeof(struct layer_state_changed));
    ev->header.last_listener_index = 0;
    return ev;
}
