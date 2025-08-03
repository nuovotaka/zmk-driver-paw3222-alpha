/*
 * Copyright 2024 Google LLC
 * Modifications Copyright 2025 sekigon-gonnoc
 * Modifications Copyright 2025 nuovotaka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>

#include "paw3222.h"
#include "paw3222_input.h"
#include "paw3222_power.h"

LOG_MODULE_REGISTER(paw32xx, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT pixart_paw3222

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int paw32xx_init(const struct device *dev) {
    const struct paw32xx_config *cfg = dev->config;
    struct paw32xx_data *data = dev->data;
    int ret;

    data->current_cpi = 0;  // 初期値を0に設定して確実にCPI変更を検出
    data->scroll_accumulator = 0;  // スクロールアキュムレータを初期化

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

#define PAW32XX_SPI_MODE \
    (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_TRANSFER_MSB)

#define PAW32XX_INIT(n) \
    COND_CODE_1(DT_INST_NODE_HAS_PROP(n, scroll_layers), \
        (static int32_t scroll_layers##n[] = DT_INST_PROP(n, scroll_layers);), \
        (/* Do nothing */)) \
    COND_CODE_1(DT_INST_NODE_HAS_PROP(n, snipe_layers), \
        (static int32_t snipe_layers##n[] = DT_INST_PROP(n, snipe_layers);), \
        (/* Do nothing */)) \
    COND_CODE_1(DT_INST_NODE_HAS_PROP(n, scroll_horizontal_layers), \
        (static int32_t scroll_horizontal_layers##n[] = DT_INST_PROP(n, scroll_horizontal_layers);), \
        (/* Do nothing */)) \
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
        .scroll_horizontal_layers = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, scroll_horizontal_layers), \
            (scroll_horizontal_layers##n), (NULL)), \
        .scroll_horizontal_layers_len = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, scroll_horizontal_layers), \
            (DT_INST_PROP_LEN(n, scroll_horizontal_layers)), (0)), \
        .res_cpi = DT_INST_PROP_OR(n, res_cpi, CONFIG_PAW3222_RES_CPI), \
        .snipe_cpi = DT_INST_PROP_OR(n, snipe_cpi, CONFIG_PAW3222_SNIPE_CPI), \
        .snipe_divisor = DT_INST_PROP_OR(n, snipe_divisor, CONFIG_PAW3222_SNIPE_DIVISOR), \
        .force_awake = DT_INST_PROP(n, force_awake), \
        .rotation = DT_INST_PROP_OR(n, rotation, CONFIG_PAW3222_SENSOR_ROTATION),     \
        .scroll_tick = DT_INST_PROP_OR(n, scroll_tick, CONFIG_PAW3222_SCROLL_TICK)   \
    }; \
    static struct paw32xx_data paw32xx_data_##n; \
    PM_DEVICE_DT_INST_DEFINE(n, paw32xx_pm_action); \
    DEVICE_DT_INST_DEFINE(n, paw32xx_init, PM_DEVICE_DT_INST_GET(n), &paw32xx_data_##n, \
                          &paw32xx_cfg_##n, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(PAW32XX_INIT)

#endif // DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
