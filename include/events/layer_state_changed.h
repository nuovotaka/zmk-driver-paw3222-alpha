#pragma once

#include <zephyr.h>
#include <zmk/event_manager.h>

/**
 * @brief Event type for when a layer is activated or deactivated.
 */
struct layer_state_changed {
    struct zmk_event_header header;
    uint8_t layer;      // 変更があったレイヤー番号
    bool state;         // true = 有効化 / false = 無効化
    int64_t timestamp;  // k_uptime_get() によるミリ秒単位のタイムスタンプ
};

// イベントの宣言（リスナー登録用）
ZMK_EVENT_DECLARE(layer_state_changed);

/**
 * @brief layer_state_changed イベント構造体を作成するヘルパー関数
 *
 * @param layer レイヤー番号
 * @param state 有効化(true)または無効化(false)
 * @return 構築済みの layer_state_changed イベントポインタ
 */
static inline struct layer_state_changed *create_layer_state_changed(uint8_t layer, bool state) {
    struct layer_state_changed *ev = new_layer_state_changed();
    ev->layer = layer;
    ev->state = state;
    ev->timestamp = k_uptime_get();
    return ev;
}

