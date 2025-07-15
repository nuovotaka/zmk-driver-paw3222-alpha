// paw3222.c

/*
 * Copyright 2024 Google LLC
 * Modifications Copyright 2025 nuovotaka
 * Modifications Copyright 2025 sekigon-gonnoc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>

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

#include "../include/paw3222.h"

LOG_MODULE_REGISTER(paw32xx, CONFIG_ZMK_LOG_LEVEL);

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
#define PAW32XX_DELTA_XY_HI 0x12
#define PAW32XX_MOUSE_OPTION 0x19

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
#define MOUSE_OPTION_MOVX_INV_BIT 3
#define MOUSE_OPTION_MOVY_INV_BIT 4

#define PAW32XX_DATA_SIZE_BITS 8

#define RESET_DELAY_MS 2

#define RES_STEP 38
#define RES_MIN (16 * RES_STEP)
#define RES_MAX (127 * RES_STEP)

#define SCROLL_TICK 10
#define SCROLL_LOCK_MS 250

enum paw32xx_input_mode {
    PAW32XX_MOVE,
    PAW32XX_SCROLL,
    PAW32XX_SNIPE,
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

    if (cfg->scroll_enabled && cfg->scroll_layers && cfg->scroll_layers_len > 0) {
        for (size_t i = 0; i < cfg->scroll_layers_len; i++) {
            if (curr_layer == cfg->scroll_layers[i]) {
                return PAW32XX_SCROLL;
            }
        }
    }
    if (cfg->snipe_enabled && cfg->snipe_layers && cfg->snipe_layers_len > 0) {
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

    enum paw32xx_input_mode input_mode = get_input_mode_for_current_layer(dev);

    // CPI切り替え
    int16_t target_cpi = cfg->res_cpi;
    if (input_mode == PAW32XX_SNIPE && cfg->snipe_cpi > 0) {
        target_cpi = cfg->snipe_cpi;
    }
    if (data->current_cpi != target_cpi) {
        paw32xx_set_resolution(dev, target_cpi);
        data->current_cpi = target_cpi;
    }

    switch (input_mode) {
        case PAW32XX_MOVE:
            input_report_rel(data->dev, INPUT_REL_X, x, false, K_FOREVER);
            input_report_rel(data->dev, INPUT_REL_Y, y, true, K_FOREVER);
            data->scroll_delta_x = 0;
            data->scroll_delta_y = 0;
            break;
        case PAW32XX_SCROLL:
            int64_t now = k_uptime_get();

            if (data->scroll_lock != SCROLL_UNLOCKED && now > data->scroll_lock_expire_time) {
                data->scroll_lock = SCROLL_UNLOCKED;
            }

            if (data->scroll_lock == SCROLL_LOCKED_X) {
                data->scroll_delta_x += x;
                if (abs(data->scroll_delta_x) > SCROLL_TICK) {
                    input_report_rel(data->dev, INPUT_REL_HWHEEL,
                        data->scroll_delta_x > 0 ? 1 : -1, true, K_FOREVER);
                    data->scroll_delta_x = 0;
                }
                data->scroll_delta_y = 0;
            } else if (data->scroll_lock == SCROLL_LOCKED_Y) {
                data->scroll_delta_y += y;
                if (abs(data->scroll_delta_y) > SCROLL_TICK) {
                    input_report_rel(data->dev, INPUT_REL_WHEEL,
                        data->scroll_delta_y > 0 ? 1 : -1, true, K_FOREVER);
                    data->scroll_delta_y = 0;
                }
                data->scroll_delta_x = 0;
            } else {
                if (abs(x) > abs(y) && abs(x) > SCROLL_TICK) {
                    data->scroll_lock = SCROLL_LOCKED_X;
                    data->scroll_lock_expire_time = now + SCROLL_LOCK_MS;
                    data->scroll_delta_x += x;
                    if (abs(data->scroll_delta_x) > SCROLL_TICK) {
                        input_report_rel(data->dev, INPUT_REL_HWHEEL,
                            data->scroll_delta_x > 0 ? 1 : -1, true, K_FOREVER);
                        data->scroll_delta_x = 0;
                    }
                    data->scroll_delta_y = 0;
                } else if (abs(y) > SCROLL_TICK) {
                    data->scroll_lock = SCROLL_LOCKED_Y;
                    data->scroll_lock_expire_time = now + SCROLL_LOCK_MS;
                    data->scroll_delta_y += y;
                    if (abs(data->scroll_delta_y) > SCROLL_TICK) {
                        input_report_rel(data->dev, INPUT_REL_WHEEL,
                            data->scroll_delta_y > 0 ? 1 : -1, true, K_FOREVER);
                        data->scroll_delta_y = 0;
                    }
                    data->scroll_delta_x = 0;
                }
            }

            input_report_rel(data->dev, INPUT_REL_X, x, false, K_FOREVER);
            input_report_rel(data->dev, INPUT_REL_Y, y, true, K_FOREVER);
            break;
        case PAW32XX_SNIPE:
            input_report_rel(data->dev, INPUT_REL_X, x, false, K_FOREVER);
            input_report_rel(data->dev, INPUT_REL_Y, y, true, K_FOREVER);
            data->scroll_delta_x = 0;
            data->scroll_delta_y = 0;
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

static int paw32xx_init(const struct device *dev) {
    const struct paw32xx_config *cfg = dev->config;
    struct paw32xx_data *data = dev->data;
    int ret;

    data->scroll_delta_x = 0;
    data->scroll_delta_y = 0;
    data->current_cpi = -1;

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
        (static int32_t scroll_layers##n[] = DT_INST_PROP(n, scroll_layers);), \
        (/* 何もしない */)) \
    COND_CODE_1(DT_INST_NODE_HAS_PROP(n, snipe_layers), \
        (static int32_t snipe_layers##n[] = DT_INST_PROP(n, snipe_layers);), \
        (/* 何もしない */)) \
    static const struct paw32xx_config paw32xx_cfg_##n = { \
        .spi = SPI_DT_SPEC_INST_GET(n, PAW32XX_SPI_MODE, 0), \
        .irq_gpio = GPIO_DT_SPEC_INST_GET(n, irq_gpios), \
        .power_gpio = GPIO_DT_SPEC_INST_GET_OR(n, power_gpios, {0}), \
        .scroll_layers = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, scroll_layers), \
            (scroll_layers##n), (NULL)), \
        .scroll_layers_len = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, scroll_layers), \
            (DT_INST_PROP_LEN(n, scroll_layers)), (0)), \
        .snipe_layers = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, snipe_layers), \
            (snipe_layers##n), (NULL)), \
        .snipe_layers_len = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, snipe_layers), \
            (DT_INST_PROP_LEN(n, snipe_layers)), (0)), \
        .res_cpi = DT_INST_PROP_OR(n, res_cpi, CONFIG_PAW3222_RES_CPI), \
        .snipe_cpi = DT_INST_PROP_OR(n, snipe_cpi, CONFIG_PAW3222_SNIPE_CPI), \
        .force_awake = DT_INST_PROP(n, force_awake), \
        .scroll_enabled = DT_INST_NODE_HAS_PROP(n, scroll_layers),  \
        .snipe_enabled = DT_INST_NODE_HAS_PROP(n, snipe_layers) \
    }; \
    static struct paw32xx_data paw32xx_data_##n; \
    PM_DEVICE_DT_INST_DEFINE(n, paw32xx_pm_action); \
    DEVICE_DT_INST_DEFINE(n, paw32xx_init, PM_DEVICE_DT_INST_GET(n), &paw32xx_data_##n, \
                          &paw32xx_cfg_##n, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(PAW32XX_INIT)

#endif // DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
