// paw3222.c

/*
 * Copyright 2024 Google LLC
 * Modifications Copyright 2025 sekigon-gonnoc
 *
 * Modifications Copyright 2025 nuovotaka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>

#include <zmk/keymap.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

#include "../include/paw3222.h"
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>

LOG_MODULE_REGISTER(paw32xx, CONFIG_ZMK_LOG_LEVEL);

// 複数センサー対応のためのグローバル変数
static const struct device *paw32xx_devices[CONFIG_PAW32XX_MAX_DEVICES] = {NULL};
static size_t paw32xx_device_count = 0;
static int8_t current_device_index = -1; // -1: すべてのデバイス、0以上: 特定のデバイス

// センサー選択モード
static bool device_select_mode = false;

// 加速度カーブ機能のグローバル制御変数


// センサー選択モード
static bool device_select_mode = false;

#define DT_DRV_COMPAT pixart_paw3222

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define PAW32XX_PRODUCT_ID1 0x00
#define PAW32XX_PRODUCT_ID2 0x01
#define PAW32XX_MOTION 0x02
#define PAW32XX_DELTA_X 0x03
#define PAW32XX_DELTA_Y 0x04
#define PAW32XX_OPERATION_MODE 0x05
#define PAW32XX_CONFIGURATION 0x06
#define PAW32XX_WRITE_PROTECT 0x09
#define PAW32XX_SLEEP1 0x0a
#define PAW32XX_SLEEP2 0x0b
#define PAW32XX_SLEEP3 0x0c
#define PAW32XX_CPI_X 0x0d
#define PAW32XX_CPI_Y 0x0e

#define PRODUCT_ID_PAW32XX 0x30
#define SPI_WRITE BIT(7)

#define MOTION_STATUS_MOTION BIT(7)
#define OPERATION_MODE_SLP_ENH BIT(4)
#define OPERATION_MODE_SLP2_ENH BIT(3)
#define OPERATION_MODE_SLP_MASK (OPERATION_MODE_SLP_ENH | OPERATION_MODE_SLP2_ENH)
#define CONFIGURATION_PD_ENH BIT(3)
#define CONFIGURATION_RESET BIT(7)
#define WRITE_PROTECT_ENABLE 0x00
#define WRITE_PROTECT_DISABLE 0x5a

#define PAW32XX_DATA_SIZE_BITS 8

#define RESET_DELAY_MS 2

#define RES_STEP 38
#define RES_MIN (16 * RES_STEP)
#define RES_MAX (127 * RES_STEP)

enum paw32xx_input_mode {
    PAW32XX_MOVE,
    PAW32XX_SCROLL,                // Vertical scroll
    PAW32XX_SCROLL_HORIZONTAL,     // Horizontal scroll
    PAW32XX_SNIPE,                 // High-precision cursor movement (snipe)
    PAW32XX_SCROLL_SNIPE,          // High-precision vertical scroll
    PAW32XX_SCROLL_SNIPE_HORIZONTAL // High-precision horizontal scroll
};


static const int32_t default_accel_thresholds[] = {2, 5, 10};
static const int32_t default_accel_factors[] = {1000, 1000, 1500, 2000};

// センサー選択モードのキー処理用
static uint8_t number_keys[] = {
    0x1E, // 1キー
    0x1F, // 2キー
    0x20, // 3キー
    0x21  // 4キー
};

static inline int32_t sign_extend(uint32_t value, uint8_t index) {
    __ASSERT_NO_MSG(index <= 31);
    uint8_t shift = 31 - index;
    return (int32_t)(value << shift) >> shift;
}

static int paw32xx_read_reg(const struct device *dev, uint8_t addr, uint8_t *value) {
    const struct paw32xx_config *cfg = dev->config;
    int ret;

    const struct spi_buf tx_buf = {
        .buf = &addr,
        .len = sizeof(addr),
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1,
    };

    struct spi_buf rx_buf[] = {
        {
            .buf = NULL,
            .len = sizeof(addr),
        },
        {
            .buf = value,
            .len = 1,
        },
    };
    const struct spi_buf_set rx = {
        .buffers = rx_buf,
        .count = ARRAY_SIZE(rx_buf),
    };

    ret = spi_transceive_dt(&cfg->spi, &tx, &rx);

    return ret;
}

static int paw32xx_write_reg(const struct device *dev, uint8_t addr, uint8_t value) {
    const struct paw32xx_config *cfg = dev->config;

    uint8_t write_buf[] = {addr | SPI_WRITE, value};
    const struct spi_buf tx_buf = {
        .buf = write_buf,
        .len = sizeof(write_buf),
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1,
    };

    return spi_write_dt(&cfg->spi, &tx);
}

static int paw32xx_update_reg(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t value) {
    uint8_t val;
    int ret;

    ret = paw32xx_read_reg(dev, addr, &val);
    if (ret < 0) {
        return ret;
    }

    val = (val & ~mask) | (value & mask);

    ret = paw32xx_write_reg(dev, addr, val);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int paw32xx_read_xy(const struct device *dev, int16_t *x, int16_t *y) {
    const struct paw32xx_config *cfg = dev->config;
    int ret;

    uint8_t tx_data[] = {
        PAW32XX_DELTA_X,
        0xff,
        PAW32XX_DELTA_Y,
        0xff,
    };
    uint8_t rx_data[sizeof(tx_data)];

    const struct spi_buf tx_buf = {
        .buf = tx_data,
        .len = sizeof(tx_data),
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1,
    };

    struct spi_buf rx_buf = {
        .buf = rx_data,
        .len = sizeof(rx_data),
    };
    const struct spi_buf_set rx = {
        .buffers = &rx_buf,
        .count = 1,
    };

    ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
    if (ret < 0) {
        return ret;
    }

    *x = rx_data[1];
    *y = rx_data[3];

    *x = sign_extend(*x, PAW32XX_DATA_SIZE_BITS - 1);
    *y = sign_extend(*y, PAW32XX_DATA_SIZE_BITS - 1);

    return 0;
}

static enum paw32xx_input_mode get_input_mode_for_current_layer(const struct device *dev) {
    const struct paw32xx_config *cfg = dev->config;
    uint8_t curr_layer = zmk_keymap_highest_layer_active();

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


int paw32xx_set_resolution(const struct device *dev, uint16_t res_cpi) {
    uint8_t val;
    int ret;

    if (!IN_RANGE(res_cpi, RES_MIN, RES_MAX)) {
        LOG_ERR("res_cpi out of range: %d", res_cpi);
        return -EINVAL;
    }

    val = res_cpi / RES_STEP;

    ret = paw32xx_write_reg(dev, PAW32XX_WRITE_PROTECT, WRITE_PROTECT_DISABLE);
    if (ret < 0) {
        return ret;
    }

    ret = paw32xx_write_reg(dev, PAW32XX_CPI_X, val);
    if (ret < 0) {
        return ret;
    }

    ret = paw32xx_write_reg(dev, PAW32XX_CPI_Y, val);
    if (ret < 0) {
        return ret;
    }

    ret = paw32xx_write_reg(dev, PAW32XX_WRITE_PROTECT, WRITE_PROTECT_ENABLE);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static void apply_acceleration_curve(const struct paw32xx_config *cfg,
                                     int64_t *prev_time,
                                     int16_t *tx, int16_t *ty)
{
    int64_t now = k_uptime_get();
    int64_t dt = now - *prev_time;
    if (dt == 0) dt = 1;

    int32_t tx32 = *tx;
    int32_t ty32 = *ty;

    float dist = sqrtf((float)(tx32 * tx32 + ty32 * ty32));
    float speed = dist / (float)dt;

    // デフォルト倍率（最初のfactor）
    int factor = (cfg->accel_factors_len > 0) ? cfg->accel_factors[0] : 1000;

    // しきい値を超えるごとにfactorを切り替え
    for (size_t i = 0; i < cfg->accel_thresholds_len && i + 1 < cfg->accel_factors_len; i++) {
        if (speed > cfg->accel_thresholds[i]) {
            factor = cfg->accel_factors[i + 1];
        }
    }

    int32_t tx_scaled = ((int32_t)tx32 * factor + 500) / 1000;
    int32_t ty_scaled = ((int32_t)ty32 * factor + 500) / 1000;

    if (tx_scaled > INT16_MAX) tx_scaled = INT16_MAX;
    if (tx_scaled < INT16_MIN) tx_scaled = INT16_MIN;
    if (ty_scaled > INT16_MAX) ty_scaled = INT16_MAX;
    if (ty_scaled < INT16_MIN) ty_scaled = INT16_MIN;

    *tx = (int16_t)tx_scaled;
    *ty = (int16_t)ty_scaled;

    *prev_time = now;
}

static void paw32xx_motion_timer_handler(struct k_timer *timer) {
    struct paw32xx_data *data = CONTAINER_OF(timer, struct paw32xx_data, motion_timer);
    k_work_submit(&data->motion_work);
}


static void paw32xx_motion_work_handler(struct k_work *work) {
    struct paw32xx_data *data = CONTAINER_OF(work, struct paw32xx_data, motion_work);
    const struct device *dev = data->dev;
    const struct paw32xx_config *cfg = dev->config;
    uint8_t val;
    int16_t x, y;
    int ret;

    ret = paw32xx_read_reg(dev, PAW32XX_MOTION, &val);
    if (ret < 0) {
        return;
    }

    if ((val & MOTION_STATUS_MOTION) == 0x00) {
        gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
        if (gpio_pin_get_dt(&cfg->irq_gpio) == 0) {
            return;
        }
    }

    ret = paw32xx_read_xy(dev, &x, &y);
    if (ret < 0) {
        return;
    }

    int16_t tx = x, ty = y;
    int rotation = cfg->rotation;
    switch (rotation) {
        case 0:
            break;
        case 90: {
            int16_t tmp = tx;
            tx = -ty;
            ty = tmp;
            break;
        }
        case 180:
            tx = -tx;
            ty = -ty;
            break;
        case 270: {
            int16_t tmp = tx;
            tx = ty;
            ty = -tmp;
            break;
        }
        default:
            break;
    }

    // Debug log
    LOG_DBG("x=%d y=%d tx=%d ty=%d", x, y, tx, ty);


    enum paw32xx_input_mode input_mode = get_input_mode_for_current_layer(dev);

    // CPI Switching
    int16_t target_cpi = cfg->res_cpi;
    if (input_mode == PAW32XX_SNIPE && cfg->snipe_cpi > 0) {
        target_cpi = cfg->snipe_cpi;
    }
    if (data->current_cpi != target_cpi) {
        paw32xx_set_resolution(dev, target_cpi);
        data->current_cpi = target_cpi;
    }

    switch (input_mode) {
        case PAW32XX_MOVE: // Normal cursor movement
        case PAW32XX_SNIPE: { // High-precision cursor movement
            if (cfg->accel_move_enable && data->accel_move_enabled) {
                apply_acceleration_curve(cfg, &data->prev_time_move, &tx, &ty);
            }
            // Send X/Y movement
            input_report_rel(data->dev, INPUT_REL_X, tx, false, K_NO_WAIT);
            input_report_rel(data->dev, INPUT_REL_Y, ty, true, K_FOREVER);
            break;
        }
        case PAW32XX_SCROLL: // Vertical scroll
            if (cfg->accel_scroll_enable && data->accel_scroll_enabled) {
                apply_acceleration_curve(cfg, &data->prev_time_scroll, &tx, &ty);
            }
            if (abs(ty) > cfg->scroll_tick) {
                input_report_rel(data->dev, INPUT_REL_WHEEL, (ty > 0 ? 1 : -1), true, K_FOREVER);
            }
            break;
        case PAW32XX_SCROLL_HORIZONTAL: // Horizontal scroll
            if (cfg->accel_scroll_enable && data->accel_scroll_enabled) {
                apply_acceleration_curve(cfg, &data->prev_time_scroll, &tx, &ty);
            }
            if (abs(ty) > cfg->scroll_tick) {
                input_report_rel(data->dev, INPUT_REL_HWHEEL, (ty > 0 ? 1 : -1), true, K_FOREVER);
            }
            break;
        case PAW32XX_SCROLL_SNIPE: // High-precision vertical scroll
            if (cfg->accel_scroll_enable && data->accel_scroll_enabled) {
                apply_acceleration_curve(cfg, &data->prev_time_scroll, &tx, &ty);
            }
            if (abs(ty) > cfg->scroll_tick) {
                // 必要に応じてスケーリング処理を追加
                input_report_rel(data->dev, INPUT_REL_WHEEL, (ty > 0 ? 1 : -1), true, K_FOREVER);
            }
            break;
        case PAW32XX_SCROLL_SNIPE_HORIZONTAL: // High-precision horizontal scroll
            if (cfg->accel_scroll_enable && data->accel_scroll_enabled) {
                apply_acceleration_curve(cfg, &data->prev_time_scroll, &tx, &ty);
            }
            if (abs(ty) > cfg->scroll_tick) {
                // 必要に応じてスケーリング処理を追加
                input_report_rel(data->dev, INPUT_REL_HWHEEL, (ty > 0 ? 1 : -1), true, K_FOREVER);
            }
            break;
        default:
            LOG_ERR("Unknown input_mode: %d", input_mode);
            break;
    }

    k_timer_start(&data->motion_timer, K_MSEC(15), K_NO_WAIT);
}

static void paw32xx_motion_handler(const struct device *gpio_dev, struct gpio_callback *cb,
                                   uint32_t pins) {
    struct paw32xx_data *data = CONTAINER_OF(cb, struct paw32xx_data, motion_cb);
    const struct device *dev = data->dev;
    const struct paw32xx_config *cfg = dev->config;

    gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_DISABLE);
    k_timer_stop(&data->motion_timer);
    k_work_submit(&data->motion_work);
}

int paw32xx_force_awake(const struct device *dev, bool enable) {
    uint8_t val = enable ? 0 : OPERATION_MODE_SLP_MASK;
    int ret;

    ret = paw32xx_write_reg(dev, PAW32XX_WRITE_PROTECT, WRITE_PROTECT_DISABLE);
    if (ret < 0) {
        return ret;
    }

    ret = paw32xx_update_reg(dev, PAW32XX_OPERATION_MODE, OPERATION_MODE_SLP_MASK, val);
    if (ret < 0) {
        return ret;
    }

    ret = paw32xx_write_reg(dev, PAW32XX_WRITE_PROTECT, WRITE_PROTECT_ENABLE);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

int paw32xx_toggle_acceleration(const struct device *dev, bool enable_move, bool enable_scroll) {
    struct paw32xx_data *data = dev->data;
    
    // デバイス固有の設定を更新
    data->accel_move_enabled = enable_move;
    data->accel_scroll_enabled = enable_scroll;
    
    LOG_INF("Device %d acceleration settings - Move: %s, Scroll: %s", 
            data->device_id,
            enable_move ? "enabled" : "disabled", 
            enable_scroll ? "enabled" : "disabled");
    
    return 0;
}

static int paw32xx_configure(const struct device *dev) {
    const struct paw32xx_config *cfg = dev->config;
    uint8_t val;
    int ret;

    ret = paw32xx_read_reg(dev, PAW32XX_PRODUCT_ID1, &val);
    if (ret < 0) {
        return ret;
    }

    if (val != PRODUCT_ID_PAW32XX) {
        LOG_ERR("Invalid product id: %02x", val);
        return -ENOTSUP;
    }

    ret = paw32xx_update_reg(dev, PAW32XX_CONFIGURATION, CONFIGURATION_RESET, CONFIGURATION_RESET);
    if (ret < 0) {
        return ret;
    }

    k_sleep(K_MSEC(RESET_DELAY_MS));

    if (cfg->res_cpi > 0) {
        paw32xx_set_resolution(dev, cfg->res_cpi);
    }

    paw32xx_force_awake(dev, cfg->force_awake);

    return 0;
}

// デバイス登録関数
static void register_paw32xx_device(const struct device *dev) {
    if (paw32xx_device_count < CONFIG_PAW32XX_MAX_DEVICES) {
        struct paw32xx_data *data = dev->data;
        const struct paw32xx_config *cfg = dev->config;
        
        // デバイスIDを設定
        data->device_id = paw32xx_device_count;
        
        // デバイスを登録
        paw32xx_devices[paw32xx_device_count] = dev;
        paw32xx_device_count++;
        
        // 初期状態では設定に基づいて加速度カーブを設定
        data->accel_move_enabled = cfg->accel_move_enable;
        data->accel_scroll_enabled = cfg->accel_scroll_enable;
        
        LOG_INF("Registered PAW32XX device %p (ID: %d, total: %d, accel_move: %s, accel_scroll: %s)", 
                dev, data->device_id, paw32xx_device_count,
                data->accel_move_enabled ? "ON" : "OFF",
                data->accel_scroll_enabled ? "ON" : "OFF");
    } else {
        LOG_ERR("Maximum number of PAW32XX devices reached (%d)", CONFIG_PAW32XX_MAX_DEVICES);
    }
}

static int paw32xx_init(const struct device *dev) {
    const struct paw32xx_config *cfg = dev->config;
    struct paw32xx_data *data = dev->data;
    int ret;

    data->current_cpi = -1;
    
    // デバイスを登録
    register_paw32xx_device(dev);

    if (!spi_is_ready_dt(&cfg->spi)) {
        LOG_ERR("%s is not ready", cfg->spi.bus->name);
        return -ENODEV;
    }

    data->dev = dev;

    k_work_init(&data->motion_work, paw32xx_motion_work_handler);
    k_timer_init(&data->motion_timer, paw32xx_motion_timer_handler, NULL);

#if DT_INST_NODE_HAS_PROP(0, power_gpios)
    if (gpio_is_ready_dt(&cfg->power_gpio)) {
        ret = gpio_pin_configure_dt(&cfg->power_gpio, GPIO_OUTPUT_INACTIVE);
        if (ret != 0) {
            LOG_ERR("Power pin configuration failed: %d", ret);
            return ret;
        }
        k_sleep(K_MSEC(500));
        ret = gpio_pin_set_dt(&cfg->power_gpio, 1);
        if (ret != 0) {
            LOG_ERR("Power pin set failed: %d", ret);
            return ret;
        }
        k_sleep(K_MSEC(10));
    }
#endif

    if (!gpio_is_ready_dt(&cfg->irq_gpio)) {
        LOG_ERR("%s is not ready", cfg->irq_gpio.port->name);
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&cfg->irq_gpio, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Motion pin configuration failed: %d", ret);
        return ret;
    }

    gpio_init_callback(&data->motion_cb, paw32xx_motion_handler, BIT(cfg->irq_gpio.pin));

    ret = gpio_add_callback_dt(&cfg->irq_gpio, &data->motion_cb);
    if (ret < 0) {
        LOG_ERR("Could not set motion callback: %d", ret);
        return ret;
    }

    ret = paw32xx_configure(dev);
    if (ret != 0) {
        LOG_ERR("Device configuration failed: %d", ret);
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        LOG_ERR("Motion interrupt configuration failed: %d", ret);
        return ret;
    }

    ret = pm_device_runtime_enable(dev);
    if (ret < 0) {
        LOG_ERR("Failed to enable runtime power management: %d", ret);
        return ret;
    }

    return 0;
}

// ZMKキーイベントリスナー
static int paw32xx_keycode_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // キーイベント処理関数を呼び出す
    if (paw32xx_process_key_event(ev)) {
        return ZMK_EV_EVENT_HANDLED;
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(paw32xx_keycode_listener, paw32xx_keycode_listener);
ZMK_SUBSCRIPTION(paw32xx_keycode_listener, zmk_keycode_state_changed);

// Public API functions for acceleration control
int paw32xx_set_accel_move_enabled(bool enabled) {
    // すべてのデバイスの設定を更新
    for (size_t i = 0; i < paw32xx_device_count; i++) {
        if (paw32xx_devices[i] != NULL) {
            struct paw32xx_data *data = paw32xx_devices[i]->data;
            data->accel_move_enabled = enabled;
        }
    }
    LOG_INF("PAW3222 Move acceleration: %s", enabled ? "ON" : "OFF");
    return 0;
}

int paw32xx_set_accel_scroll_enabled(bool enabled) {
    // すべてのデバイスの設定を更新
    for (size_t i = 0; i < paw32xx_device_count; i++) {
        if (paw32xx_devices[i] != NULL) {
            struct paw32xx_data *data = paw32xx_devices[i]->data;
            data->accel_scroll_enabled = enabled;
        }
    }
    LOG_INF("PAW3222 Scroll acceleration: %s", enabled ? "ON" : "OFF");
    return 0;
}

bool paw32xx_get_accel_move_enabled(void) {
    // 最初のデバイスの設定を返す
    if (paw32xx_device_count > 0 && paw32xx_devices[0] != NULL) {
        struct paw32xx_data *data = paw32xx_devices[0]->data;
        return data->accel_move_enabled;
    }
    return false;
}

bool paw32xx_get_accel_scroll_enabled(void) {
    // 最初のデバイスの設定を返す
    if (paw32xx_device_count > 0 && paw32xx_devices[0] != NULL) {
        struct paw32xx_data *data = paw32xx_devices[0]->data;
        return data->accel_scroll_enabled;
    }
    return false;
}
    return global_accel_scroll_enabled;
}

size_t paw32xx_get_device_count(void) {
    return paw32xx_device_count;
}

const struct device *paw32xx_get_device(size_t index) {
    if (index >= paw32xx_device_count) {
        return NULL;
    }
    return paw32xx_devices[index];
}

#ifdef CONFIG_PM_DEVICE
static int paw32xx_pm_action(const struct device *dev, enum pm_device_action action) {
    const struct paw32xx_config *cfg = dev->config;
    int ret;
    uint8_t val;

    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
        val = CONFIGURATION_PD_ENH;
        ret = paw32xx_update_reg(dev, PAW32XX_CONFIGURATION, CONFIGURATION_PD_ENH, val);
        if (ret < 0) {
            return ret;
        }

#if DT_INST_NODE_HAS_PROP(0, power_gpios)
        if (gpio_is_ready_dt(&cfg->power_gpio)) {
            ret = gpio_pin_set_dt(&cfg->power_gpio, 0);
            if (ret < 0) {
                LOG_ERR("Failed to disable power: %d", ret);
                return ret;
            }
        }
#endif
        break;

    case PM_DEVICE_ACTION_RESUME:
#if DT_INST_NODE_HAS_PROP(0, power_gpios)
        if (gpio_is_ready_dt(&cfg->power_gpio)) {
            ret = gpio_pin_set_dt(&cfg->power_gpio, 1);
            if (ret < 0) {
                LOG_ERR("Failed to enable power: %d", ret);
                return ret;
            }
            k_sleep(K_MSEC(10));
        }
#endif

        val = 0;
        ret = paw32xx_update_reg(dev, PAW32XX_CONFIGURATION, CONFIGURATION_PD_ENH, val);
        if (ret < 0) {
            return ret;
        }
        break;

    default:
        return -ENOTSUP;
    }

    return 0;
}
#endif

#define PAW32XX_SPI_MODE \
    (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_TRANSFER_MSB)

#define PAW32XX_INIT(n) \
    COND_CODE_1(DT_INST_NODE_HAS_PROP(n, scroll_layers), \
        (static int32_t scroll_layers##n[] = DT_INST_PROP(n, scroll_layers);), ()) \
    COND_CODE_1(DT_INST_NODE_HAS_PROP(n, snipe_layers), \
        (static int32_t snipe_layers##n[] = DT_INST_PROP(n, snipe_layers);), ()) \
    COND_CODE_1(DT_INST_NODE_HAS_PROP(n, scroll_horizontal_layers), \
        (static int32_t scroll_horizontal_layers##n[] = DT_INST_PROP(n, scroll_horizontal_layers);), ()) \
    COND_CODE_1(DT_INST_NODE_HAS_PROP(n, accel_thresholds), \
        (static int32_t accel_thresholds##n[] = DT_INST_PROP(n, accel_thresholds);), ()) \
    COND_CODE_1(DT_INST_NODE_HAS_PROP(n, accel_factors), \
        (static int32_t accel_factors##n[] = DT_INST_PROP(n, accel_factors);), ()) \
    static const struct paw32xx_config paw32xx_cfg_##n = { \
        .spi = SPI_DT_SPEC_INST_GET(n, PAW32XX_SPI_MODE, 0), \
        .irq_gpio = GPIO_DT_SPEC_INST_GET(n, irq_gpios), \
        .power_gpio = GPIO_DT_SPEC_INST_GET_OR(n, power_gpios, {0}), \
        .accel_move_enable = DT_INST_PROP_OR(n, accel_move_enable, CONFIG_PAW32XX_ACCEL_MOVE_ENABLE), \
        .accel_scroll_enable = DT_INST_PROP_OR(n, accel_scroll_enable, CONFIG_PAW32XX_ACCEL_SCROLL_ENABLE), \
        .accel_thresholds = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, accel_thresholds), \
            (accel_thresholds##n), (default_accel_thresholds)), \
        .accel_thresholds_len = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, accel_thresholds), \
            (DT_INST_PROP_LEN(n, accel_thresholds)), (ARRAY_SIZE(default_accel_thresholds))), \
        .accel_factors = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, accel_factors), \
            (accel_factors##n), (default_accel_factors)), \
        .accel_factors_len = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, accel_factors), \
            (DT_INST_PROP_LEN(n, accel_factors)), (ARRAY_SIZE(default_accel_factors))), \
        .scroll_layers = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, scroll_layers), \
            (scroll_layers##n), (NULL)), \
        .scroll_layers_len = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, scroll_layers), \
            (DT_INST_PROP_LEN(n, scroll_layers)), (0)), \
        .snipe_layers = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, snipe_layers), \
            (snipe_layers##n), (NULL)), \
        .snipe_layers_len = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, snipe_layers), \
            (DT_INST_PROP_LEN(n, snipe_layers)), (0)), \
        .scroll_horizontal_layers = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, scroll_horizontal_layers), \
            (scroll_horizontal_layers##n), (NULL)), \
        .scroll_horizontal_layers_len = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, scroll_horizontal_layers), \
            (DT_INST_PROP_LEN(n, scroll_horizontal_layers)), (0)), \
        .res_cpi = DT_INST_PROP_OR(n, res_cpi, CONFIG_PAW3222_RES_CPI), \
        .snipe_cpi = DT_INST_PROP_OR(n, snipe_cpi, CONFIG_PAW3222_SNIPE_CPI), \
        .force_awake = DT_INST_PROP(n, force_awake), \
        .scroll_enabled = DT_INST_NODE_HAS_PROP(n, scroll_layers),  \
        .snipe_enabled = DT_INST_NODE_HAS_PROP(n, snipe_layers), \
        .rotation = DT_INST_PROP_OR(n, rotation, CONFIG_PAW32XX_SENSOR_ROTATION),     \
        .scroll_tick = DT_INST_PROP_OR(n, scroll_tick, CONFIG_PAW32XX_SCROLL_TICK)   \
    }; \
    static struct paw32xx_data paw32xx_data_##n; \
    PM_DEVICE_DT_INST_DEFINE(n, paw32xx_pm_action); \
    DEVICE_DT_INST_DEFINE(n, paw32xx_init, PM_DEVICE_DT_INST_GET(n), &paw32xx_data_##n, \
                          &paw32xx_cfg_##n, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(PAW32XX_INIT)

#endif // DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)// キーイ
ベント処理関数
bool paw32xx_process_key_event(const struct zmk_keycode_state_changed *event) {
    // キーが押されたときのみ処理（リリース時は無視）
    if (!event->state) {
        return false;
    }
    
    // センサー選択モード切り替えキー
    if (event->keycode == PAW32XX_TOGGLE_ACCEL_DEVICE_SELECT_KEYCODE) {
        device_select_mode = !device_select_mode;
        
        if (device_select_mode) {
            LOG_INF("Sensor select mode: ON - Press 1-%d to select sensor, 0 for all", paw32xx_device_count);
            current_device_index = -1; // デフォルトはすべてのデバイス
        } else {
            if (current_device_index < 0) {
                LOG_INF("Sensor select mode: OFF - Currently affecting ALL sensors");
            } else {
                LOG_INF("Sensor select mode: OFF - Currently affecting sensor %d", current_device_index + 1);
            }
        }
        return true;
    }
    
    // センサー選択モードの場合、数字キーでセンサーを選択
    if (device_select_mode) {
        // 0キー: すべてのセンサーを選択
        if (event->keycode == 0x27) { // 0キー
            current_device_index = -1;
            LOG_INF("Selected ALL sensors");
            return true;
        }
        
        // 1-4キー: 特定のセンサーを選択
        for (int i = 0; i < MIN(paw32xx_device_count, 4); i++) {
            if (event->keycode == number_keys[i]) {
                current_device_index = i;
                LOG_INF("Selected sensor %d", i + 1);
                return true;
            }
        }
    }
    
    // 加速度カーブ切り替えキー
    if (event->keycode == PAW32XX_TOGGLE_ACCEL_KEYCODE) {
        LOG_INF("Acceleration toggle key pressed");
        
        if (current_device_index < 0) {
            // すべてのセンサーの設定を切り替え
            for (size_t i = 0; i < paw32xx_device_count; i++) {
                if (paw32xx_devices[i] != NULL) {
                    struct paw32xx_data *data = paw32xx_devices[i]->data;
                    data->accel_move_enabled = !data->accel_move_enabled;
                    data->accel_scroll_enabled = !data->accel_scroll_enabled;
                    
                    LOG_INF("Sensor %d acceleration - Move: %s, Scroll: %s", 
                            i + 1,
                            data->accel_move_enabled ? "ON" : "OFF",
                            data->accel_scroll_enabled ? "ON" : "OFF");
                }
            }
        } else if (current_device_index < paw32xx_device_count) {
            // 特定のセンサーの設定を切り替え
            struct paw32xx_data *data = paw32xx_devices[current_device_index]->data;
            data->accel_move_enabled = !data->accel_move_enabled;
            data->accel_scroll_enabled = !data->accel_scroll_enabled;
            
            LOG_INF("Sensor %d acceleration - Move: %s, Scroll: %s", 
                    current_device_index + 1,
                    data->accel_move_enabled ? "ON" : "OFF",
                    data->accel_scroll_enabled ? "ON" : "OFF");
        }
        
        return true;
    }
    
    return false;
}