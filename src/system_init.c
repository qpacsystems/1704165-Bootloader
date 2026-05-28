/**
 * @file system_init.c
 * @brief Minimal SystemInit for bootloader (called before main)
 *
 * @section License
 * SPDX-License-Identifier: GPL-2.0-or-later
 **/

#include "stm32h5xx.h"

/**
 * @brief System initialization function
 *
 * Called from Reset_Handler before main(). Sets up FPU and
 * VTOR for bootloader. Minimal — no clock configuration here
 * (done in main via HAL).
 */
void SystemInit(void)
{
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    // Enable FPU (CP10 and CP11 full access)
    SCB->CPACR |= ((3UL << 20U) | (3UL << 22U));
#endif

    // Set VTOR to bootloader flash base
    SCB->VTOR = 0x08000000U;
}

// Required by HAL
uint32_t SystemCoreClock = 64000000U; // Default HSI, updated after PLL config
