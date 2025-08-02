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
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>

#include "paw3222.h"
#include "paw3222_regs.h"
#include "paw3222_spi.h"
#include "paw3222_power.h"

LOG_MODULE_DECLARE(paw32xx);

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

int paw32xx_configure(const struct device *dev) {
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

#ifdef CONFIG_PM_DEVICE
int paw32xx_pm_action(const struct device *dev, enum pm_device_action action) {
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