/**
 * @file main.c
 * @brief Standalone bootloader entry point
 * SPDX-License-Identifier: GPL-2.0-or-later
 **/

#include "boot_config.h"
#include "stm32h5xx_hal.h"
#include "bootloader/second_stage/boot.h"
#include "core/mailbox.h"
#include "drivers/flash/internal/stm32h5xx_flash_driver.h"
#include "modules/memory/memory.h"
#include "boot_hooks.h"

static void bootConfigureMemory(BootSettings *settings);

static BootContext bootContext;
static BootSettings bootSettings;

int main(void)
{
    // Blink LED immediately using raw registers (proves we're alive)
    // Enable GPIOB clock
    *(volatile uint32_t *)0x44020C8CU |= (1U << 1);
    for (volatile int i = 0; i < 10; i++);
    // PB13 as output
    uint32_t moder = *(volatile uint32_t *)0x42020400U;
    moder &= ~(3U << 26);
    moder |= (1U << 26);
    *(volatile uint32_t *)0x42020400U = moder;
    // Brief blink to show bootloader is alive
    for (int i = 0; i < 4; i++) {
        *(volatile uint32_t *)0x42020414U ^= (1U << 13);
        for (volatile uint32_t d = 0; d < 200000; d++);
    }

    HAL_Init();
    HAL_ICACHE_Enable();

    // Configure CycloneBOOT memory layout
    bootGetDefaultSettings(&bootSettings);
    bootConfigureMemory(&bootSettings);
    bootInit(&bootContext, &bootSettings);

    // Check if app slot has a valid-looking image (SP should be in RAM range)
    uint32_t appSp = *(volatile uint32_t *)(FLASH_BANK1_ADDR + BOOT_OFFSET);
    if (appSp >= 0x20000000 && appSp <= 0x200A0000) {
        // Valid app present — run CycloneBOOT FSM (validates, jumps, or falls back)
        while (1) {
            bootFsm(&bootContext);
        }
    } else {
        // No valid app — blink LED forever
        while (1) {
            *(volatile uint32_t *)0x42020414U ^= (1U << 13);
            for (volatile uint32_t d = 0; d < 300000; d++);
        }
    }

    return 0;
}

static void bootConfigureMemory(BootSettings *settings)
{
    Memory *primaryMem = &settings->memories[0];
    primaryMem->memoryType = MEMORY_TYPE_FLASH;
    primaryMem->memoryRole = MEMORY_ROLE_PRIMARY;
    primaryMem->driver     = &stm32h5xxFlashDriver;
    primaryMem->nbSlots    = 2;

    primaryMem->slots[0].type      = SLOT_TYPE_DIRECT;
    primaryMem->slots[0].cType     = SLOT_CONTENT_APP | SLOT_CONTENT_BACKUP;
    primaryMem->slots[0].memParent = primaryMem;
    primaryMem->slots[0].addr      = FLASH_BANK1_ADDR;
    primaryMem->slots[0].size      = FLASH_BANK1_SIZE;

    primaryMem->slots[1].type      = SLOT_TYPE_DIRECT;
    primaryMem->slots[1].cType     = SLOT_CONTENT_APP | SLOT_CONTENT_BACKUP;
    primaryMem->slots[1].memParent = primaryMem;
    primaryMem->slots[1].addr      = FLASH_BANK2_ADDR;
    primaryMem->slots[1].size      = FLASH_BANK2_SIZE;
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

//-----------------------------------------------------------------------------
// CycloneBOOT v2.6.2 weak hook implementations
//-----------------------------------------------------------------------------

__attribute__((weak)) void bootInitHook(void)
{
    // LED already initialized via raw registers above
}

__attribute__((weak)) void bootIdleStateHook(void)
{
    *(volatile uint32_t *)0x42020414U ^= (1U << 13);
    HAL_Delay(50);
}

__attribute__((weak)) void bootNoValidUpdatesHook(void)
{
    // Fast blink forever
    while (1) {
        *(volatile uint32_t *)0x42020414U ^= (1U << 13);
        HAL_Delay(100);
    }
}

__attribute__((weak)) void bootJumpingToApplicationHook(void)
{
    *(volatile uint32_t *)0x42020414U &= ~(1U << 13);
}

__attribute__((weak)) void bootFallbackPerformedHook(void)
{
    for (int i = 0; i < 8; i++) {
        *(volatile uint32_t *)0x42020414U ^= (1U << 13);
        HAL_Delay(50);
    }
}

__attribute__((weak)) void bootHandleFallbackError(void)
{
    bootNoValidUpdatesHook();
}

__attribute__((weak)) void bootHandleGenericError(void)
{
    bootNoValidUpdatesHook();
}
