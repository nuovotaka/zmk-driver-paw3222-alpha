/*
 * Copyright 2024 Google LLC
 * Modifications Copyright 2025 sekigon-gonnoc
 * Modifications Copyright 2025 nuovotaka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "paw3222.h"
#include "paw3222_regs.h"
#include "paw3222_spi.h"

LOG_MODULE_DECLARE(paw32xx);

static inline int32_t sign_extend(uint32_t value, uint8_t index) {
    __ASSERT_NO_MSG(index <= 31);
    uint8_t shift = 31 - index;
    return (int32_t)(value << shift) >> shift;
}

int paw32xx_read_reg(const struct device *dev, uint8_t addr, uint8_t *value) {
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

int paw32xx_write_reg(const struct device *dev, uint8_t addr, uint8_t value) {
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

int paw32xx_update_reg(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t value) {
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

int paw32xx_read_xy(const struct device *dev, int16_t *x, int16_t *y) {
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