/**
 * @file boot_hooks.h
 * @brief Bootloader hook declarations (LED blink, error handling)
 *
 * @section License
 * SPDX-License-Identifier: GPL-2.0-or-later
 **/

#ifndef BOOT_HOOKS_H
#define BOOT_HOOKS_H

#include <stdint.h>

/**
 * @brief Initialize bootloader status LED GPIO
 */
void bootHookInitLed(void);

/**
 * @brief Toggle status LED (called during boot FSM iterations)
 */
void bootHookToggleLed(void);

/**
 * @brief Signal fatal bootloader error (fast LED blink, hang)
 */
void bootHookFatalError(void);

#endif // BOOT_HOOKS_H
