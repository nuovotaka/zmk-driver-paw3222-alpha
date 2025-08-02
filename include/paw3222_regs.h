/*
 * Copyright 2024 Google LLC
 * Modifications Copyright 2025 sekigon-gonnoc
 * Modifications Copyright 2025 nuovotaka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PAW3222_REGS_H_
#define ZEPHYR_INCLUDE_PAW3222_REGS_H_

#include <zephyr/sys/util.h>

/* PAW3222 Register Definitions */
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

/* Register Values */
#define PRODUCT_ID_PAW32XX 0x30
#define SPI_WRITE BIT(7)

/* Motion Register Bits */
#define MOTION_STATUS_MOTION BIT(7)

/* Operation Mode Register Bits */
#define OPERATION_MODE_SLP_ENH BIT(4)
#define OPERATION_MODE_SLP2_ENH BIT(3)
#define OPERATION_MODE_SLP_MASK (OPERATION_MODE_SLP_ENH | OPERATION_MODE_SLP2_ENH)

/* Configuration Register Bits */
#define CONFIGURATION_PD_ENH BIT(3)
#define CONFIGURATION_RESET BIT(7)

/* Write Protection Values */
#define WRITE_PROTECT_ENABLE 0x00
#define WRITE_PROTECT_DISABLE 0x5a

/* Data Size and Timing */
#define PAW32XX_DATA_SIZE_BITS 8
#define RESET_DELAY_MS 2

/* Resolution Settings */
#define RES_STEP 38
#define RES_MIN (16 * RES_STEP)
#define RES_MAX (127 * RES_STEP)

/* Input Modes */
enum paw32xx_input_mode {
    PAW32XX_MOVE,
    PAW32XX_SCROLL,                // Vertical scroll
    PAW32XX_SCROLL_HORIZONTAL,     // Horizontal scroll
    PAW32XX_SNIPE,                 // High-precision cursor movement (snipe)
};

#endif /* ZEPHYR_INCLUDE_PAW3222_REGS_H_ */