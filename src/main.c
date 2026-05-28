/**
 * @file main.c
 * @brief Standalone bootloader entry point
 *
 * Bare-metal (no RTOS). Initializes minimal hardware, configures
 * CycloneBOOT dual-bank memory layout, and runs the boot FSM.
 *
 * @section License
 * SPDX-License-Identifier: GPL-2.0-or-later
 **/

#include "boot_config.h"
#include "stm32h5xx_hal.h"
#include "bootloader/second_stage/boot.h"
#include "core/mailbox.h"
#include "drivers/flash/internal/stm32h5xx_flash_driver.h"
#include "modules/memory/memory.h"
#include "boot_hooks.h"

// Forward declarations
static void SystemClock_Config(void);
static void bootConfigureMemory(void);

// CycloneBOOT boot context (defined in boot.h)
extern BootContext bootContext;

int main(void)
{
    // Minimal HAL init (SysTick, flash latency)
    HAL_Init();

    // Configure system clock (HSE -> PLL -> 250 MHz)
    SystemClock_Config();

    // Enable ICACHE for faster flash reads
    HAL_ICACHE_Enable();

    // Configure CycloneBOOT memory layout
    bootConfigureMemory();

    // Initialize the boot FSM
    bootInit(&bootContext);

    // Run boot state machine in a loop
    // bootFsm() will eventually jump to the application or
    // perform fallback if the image is invalid
    while (1) {
        bootFsm(&bootContext);
    }

    // Never reached
    return 0;
}

/**
 * @brief Configure dual-bank memory layout for CycloneBOOT
 */
static void bootConfigureMemory(void)
{
    BootSettings *settings = &bootContext.settings;

    // Verification: CRC32 integrity check
    settings->imageOutCrypto.verifySettings.verifyMethod = VERIFY_METHOD_INTEGRITY;
    settings->imageOutCrypto.verifySettings.integrityAlgo = CRC32_HASH_ALGO;

    // Primary memory: internal flash
    Memory *primaryMem = &settings->memories[0];
    primaryMem->memoryType = MEMORY_TYPE_FLASH;
    primaryMem->memoryRole = MEMORY_ROLE_PRIMARY;
    primaryMem->driver     = &stm32h5xxFlashDriver;
    primaryMem->nbSlots    = 2;

    // Slot 0: Application (Bank 1, after bootloader)
    primaryMem->slots[0].type      = SLOT_TYPE_DIRECT;
    primaryMem->slots[0].cType     = SLOT_CONTENT_APP | SLOT_CONTENT_BACKUP;
    primaryMem->slots[0].memParent = primaryMem;
    primaryMem->slots[0].addr      = FLASH_BANK1_ADDR;
    primaryMem->slots[0].size      = FLASH_BANK1_SIZE;

    // Slot 1: Update staging (Bank 2)
    primaryMem->slots[1].type      = SLOT_TYPE_DIRECT;
    primaryMem->slots[1].cType     = SLOT_CONTENT_APP | SLOT_CONTENT_BACKUP;
    primaryMem->slots[1].memParent = primaryMem;
    primaryMem->slots[1].addr      = FLASH_BANK2_ADDR;
    primaryMem->slots[1].size      = FLASH_BANK2_SIZE;
}

/**
 * @brief System clock configuration
 *
 * HSE (25 MHz) -> PLL1 -> SYSCLK = 250 MHz
 * Minimal config: no USB, no Ethernet, no SAI clocks.
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Configure power supply
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

    // Configure HSE oscillator
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLL1_SOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 5;   // 25MHz / 5 = 5MHz
    RCC_OscInitStruct.PLL.PLLN       = 100;  // 5MHz * 100 = 500MHz VCO
    RCC_OscInitStruct.PLL.PLLP       = 2;   // 500MHz / 2 = 250MHz SYSCLK
    RCC_OscInitStruct.PLL.PLLQ       = 2;
    RCC_OscInitStruct.PLL.PLLR       = 2;
    RCC_OscInitStruct.PLL.PLLFRACN   = 0;
    RCC_OscInitStruct.PLL.PLLRGE     = RCC_PLL1_VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL  = RCC_PLL1_VCORANGE_WIDE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        // Clock config failed — hang
        while (1);
    }

    // Configure bus clocks
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2  |
                                       RCC_CLOCKTYPE_PCLK3;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

    // Flash latency for 250 MHz: 5 wait states
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        while (1);
    }
}

// Required by HAL for tick source (bare-metal, no RTOS)
void SysTick_Handler(void)
{
    HAL_IncTick();
}
