/*
 * Copyright 2024 Google LLC
 * Modifications Copyright 2025 sekigon-gonnoc
 * Modifications Copyright 2025 nuovotaka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PAW3222_INPUT_H_
#define PAW3222_INPUT_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "paw3222_regs.h"

/**
 * @brief Get the input mode for the current active layer or behavior state
 *
 * Determines the current input mode based on either the active ZMK layer
 * (for layer-based switching) or the current behavior state (for toggle-based switching).
 * The mode affects how motion data from the sensor is interpreted and reported.
 *
 * @param dev PAW3222 device pointer (must not be NULL)
 * 
 * @return Current input mode enum value
 * @retval PAW32XX_MOVE Default cursor movement mode
 * @retval PAW32XX_SCROLL Vertical scroll mode
 * @retval PAW32XX_SCROLL_HORIZONTAL Horizontal scroll mode
 * @retval PAW32XX_SNIPE High-precision cursor mode
 * @retval PAW32XX_SCROLL_SNIPE High-precision vertical scroll mode
 * @retval PAW32XX_SCROLL_HORIZONTAL_SNIPE High-precision horizontal scroll mode
 * 
 * @note This function is called during motion processing to determine how
 *       to interpret sensor data. The behavior depends on the switch_method
 *       configured in the device tree.
 */
enum paw32xx_input_mode
get_input_mode_for_current_layer(const struct device *dev);

#ifdef CONFIG_PAW3222_BEHAVIOR
/**
 * @brief Set the PAW3222 device reference for behavior-based mode switching
 *
 * Stores a global reference to the PAW3222 device instance for use by the
 * behavior driver. This allows the behavior system to control the sensor's
 * input mode without direct device access.
 *
 * @param dev PAW3222 device pointer (must not be NULL)
 * 
 * @note This function is called automatically during device initialization
 *       when CONFIG_PAW3222_BEHAVIOR is enabled. It should not be called
 *       directly by application code.
 * 
 * @warning Only one PAW3222 device instance is supported when using behaviors.
 */
void paw32xx_set_device_reference(const struct device *dev);
#endif

/**
 * @brief Motion timer expiration handler
 *
 * Called when the motion processing timer expires. This handler submits
 * the motion work item to continue processing sensor data after a delay.
 * The timer is used to implement a polling mechanism for continuous
 * motion detection.
 *
 * @param timer Pointer to the expired timer (must not be NULL)
 * 
 * @note This function is called from interrupt context and should perform
 *       minimal work. The actual motion processing is deferred to the
 *       work queue handler.
 */
void paw32xx_motion_timer_handler(struct k_timer *timer);

/**
 * @brief Motion work queue handler - processes sensor data
 *
 * This is the main motion processing function that reads motion data from
 * the PAW3222 sensor and generates appropriate input events. The function:
 * - Reads motion status and X/Y delta values from the sensor
 * - Determines the current input mode (move, scroll, snipe, etc.)
 * - Applies coordinate transformations based on sensor rotation
 * - Handles CPI switching for different modes
 * - Generates input events (cursor movement, scroll wheel, etc.)
 * - Manages scroll accumulation for smooth scrolling
 *
 * @param work Pointer to the work item being processed (must not be NULL)
 * 
 * @note This function runs in work queue context and can perform blocking
 *       operations like SPI transactions. It's triggered by GPIO interrupts
 *       or timer expiration.
 * 
 * @warning This function temporarily disables motion interrupts during
 *          processing to prevent race conditions.
 */
void paw32xx_motion_work_handler(struct k_work *work);

/**
 * @brief GPIO interrupt handler for motion detection
 *
 * Called when the PAW3222 motion pin (IRQ) transitions to active state,
 * indicating that new motion data is available. This handler:
 * - Disables further motion interrupts to prevent race conditions
 * - Stops any running motion timer
 * - Submits motion work to the work queue for processing
 *
 * @param gpio_dev GPIO device that triggered the interrupt (unused)
 * @param cb Pointer to the GPIO callback structure containing driver data
 * @param pins Bitmask of pins that triggered the interrupt (unused)
 * 
 * @note This function runs in interrupt context and must complete quickly.
 *       All actual motion processing is deferred to the work queue.
 * 
 * @note The motion interrupt is re-enabled after motion processing completes
 *       in the work handler or when no motion is detected.
 */
void paw32xx_motion_handler(const struct device *gpio_dev,
                            struct gpio_callback *cb, uint32_t pins);

#endif /* PAW3222_INPUT_H_ */