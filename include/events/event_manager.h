#pragma once

#include <zephyr/kernel.h>

/**
 * @brief ZMK イベントヘッダ
 */
struct zmk_event_header {
    uint8_t last_listener_index;
};

#define ZMK_EV_EVENT_BUBBLE   0
#define ZMK_EV_EVENT_HANDLED  1
#define ZMK_EV_EVENT_CAPTURED 2

/**
 * @brief イベント型を宣言するマクロ
 *
 * struct zmk_event_type を生成し、イベントの型名に対応します。
 */
#define ZMK_EVENT_DECLARE(event_type)

/**
 * @brief イベントの実装を定義するマクロ
 *
 * リスナー登録や dispatch 処理をここで行います。
 */
#define ZMK_EVENT_IMPL(event_type)

/**
 * @brief イベントを発行するマクロ
 *
 * 内部的に zmk_event_t（void*）をラップしたイベント構造体を投げます。
 */
#define ZMK_EVENT_RAISE(event)
