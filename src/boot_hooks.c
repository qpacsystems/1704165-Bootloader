/**
 * @file boot_hooks.c
 * @brief Bootloader hook implementations
 *
 * Weak default implementations for bootloader status indication.
 * Override these in board-specific code if needed.
 *
 * @section License
 * SPDX-License-Identifier: GPL-2.0-or-later
 **/

#include "boot_hooks.h"
#include "stm32h5xx_hal.h"

// Status LED: PB0 (green LED on 1704165 board)
#define BOOT_LED_PORT   GPIOB
#define BOOT_LED_PIN    GPIO_PIN_0

__attribute__((weak)) void bootHookInitLed(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = BOOT_LED_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(BOOT_LED_PORT, &gpio);
    HAL_GPIO_WritePin(BOOT_LED_PORT, BOOT_LED_PIN, GPIO_PIN_SET);
}

__attribute__((weak)) void bootHookToggleLed(void)
{
    HAL_GPIO_TogglePin(BOOT_LED_PORT, BOOT_LED_PIN);
}

__attribute__((weak)) void bootHookFatalError(void)
{
    bootHookInitLed();

    while (1) {
        HAL_GPIO_TogglePin(BOOT_LED_PORT, BOOT_LED_PIN);
        // ~100ms blink at 250 MHz: crude delay
        for (volatile uint32_t i = 0; i < 2500000; i++);
    }
}
