/**
 * @file main.c
 * @brief Standalone bootloader entry point with OTA update support
 *
 * Boot flow:
 *   1. Blink LED (alive indicator)
 *   2. HAL init
 *   3. Check mailbox for UPDATE_REQUESTED → handle OTA update from Bank 2
 *   4. Validate app vector table at 0x08010000
 *   5. Jump to app (or blink error if no valid app)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 **/

#include "boot_config.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx.h"
#include "bootloader/second_stage/boot.h"
#include "core/mailbox.h"
#include "drivers/flash/internal/stm32h5xx_flash_driver.h"
#include "modules/memory/memory.h"
#include "boot_hooks.h"
#include <string.h>

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

#define APP_ADDR            (FLASH_BANK1_ADDR + BOOT_OFFSET)  // 0x08010000
#define BANK2_ADDR          FLASH_BANK2_ADDR                   // 0x08100000
#define IMG_HEADER_SIZE     64U
#define FLASH_QUAD_WORD     16U
#define SECTOR_SIZE         0x2000U  // 8KB

// App region: sector 8 (0x08010000) through sector 126 (0x080FC000)
// Sector 127 (0x080FE000) is config — don't erase
#define APP_START_SECTOR    8U
#define APP_NUM_SECTORS     119U  // sectors 8-126

// Mailbox constants (matching CycloneBOOT BootMailBox layout)
#define MBX_SIGNATURE       0x1B241671U
#define MBX_FLAG_UPDATE_REQ (1U << 0)
#define MBX_FLAG_UPDATE_OK  (1U << 1)
#define MBX_FLAG_CONFIRMED  (1U << 2)

// Mailbox struct (132 bytes at 0x20000000, NOLOAD)
typedef struct __attribute__((packed)) {
    uint32_t version;
    uint32_t signature;
    uint32_t pskSize;
    uint8_t  psk[32];
    uint32_t flags;
    uint32_t bootCounter;
    uint8_t  reserved[80];
} BootMailbox;

// Image header (64 bytes, matching CycloneBOOT v2.6.2)
typedef struct __attribute__((packed)) {
    uint32_t headVers;
    uint32_t imgIndex;
    uint8_t  imgType;
    uint32_t dataPadding;
    uint32_t dataSize;
    uint32_t binarySize;
    uint8_t  versMajor;
    uint8_t  versMinor;
    uint16_t versRevision;
    uint32_t versBuild;
    uint64_t imgTime;
    uint8_t  reserved[23];
    uint32_t headCrc;
} OtaImageHeader;

//-----------------------------------------------------------------------------
// LED helpers (raw register, no HAL dependency)
//-----------------------------------------------------------------------------

#define LED_TOGGLE() (*(volatile uint32_t *)0x42020414U ^= (1U << 13))
#define LED_ON()     (*(volatile uint32_t *)0x42020414U |= (1U << 13))
#define LED_OFF()    (*(volatile uint32_t *)0x42020414U &= ~(1U << 13))

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

static void initLed(void);
static void jumpToApp(uint32_t addr);
static int  handleOtaUpdate(void);
static uint32_t crc32Calc(const uint8_t *data, uint32_t len);

// CycloneBOOT stubs (keep linker happy — we don't use bootFsm)
static BootContext bootContext;
static BootSettings bootSettings;
static void bootConfigureMemory(BootSettings *settings);

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    initLed();

    // Brief blink to show bootloader is alive
    for (int i = 0; i < 4; i++) {
        LED_TOGGLE();
        for (volatile uint32_t d = 0; d < 200000; d++);
    }

    HAL_Init();
    HAL_ICACHE_Enable();

    // Read shared mailbox at start of RAM (NOLOAD, survives warm reset)
    volatile BootMailbox *mbx = (volatile BootMailbox *)0x20000000U;

    // Check for OTA update request
    if (mbx->signature == MBX_SIGNATURE &&
        (mbx->flags & MBX_FLAG_UPDATE_REQ)) {

        // Slow blink while updating
        LED_ON();

        int result = handleOtaUpdate();

        if (result == 0) {
            // Success — update mailbox
            mbx->flags &= ~MBX_FLAG_UPDATE_REQ;
            mbx->flags |= MBX_FLAG_UPDATE_OK;
            mbx->flags &= ~MBX_FLAG_CONFIRMED;
            mbx->bootCounter++;
        } else {
            // Failed — clear request, keep existing app
            mbx->flags &= ~MBX_FLAG_UPDATE_REQ;
        }

        LED_OFF();
        // Fall through to app jump
    }

    // Check if app slot has a valid vector table
    uint32_t appSp = *(volatile uint32_t *)(APP_ADDR);
    uint32_t appPc = *(volatile uint32_t *)(APP_ADDR + 4);

    if (appSp >= 0x20000000 && appSp <= 0x200A0000 &&
        appPc >= APP_ADDR && appPc < (APP_ADDR + 0x100000)) {
        jumpToApp(APP_ADDR);
    }

    // No valid app — blink LED forever
    while (1) {
        LED_TOGGLE();
        for (volatile uint32_t d = 0; d < 300000; d++);
    }

    return 0;
}

//-----------------------------------------------------------------------------
// OTA Update Handler
//-----------------------------------------------------------------------------

/**
 * @brief Handle OTA update: validate Bank 2 image, copy payload to Bank 1
 * @return 0 on success, -1 on failure
 */
static int handleOtaUpdate(void)
{
    // Read image header from Bank 2
    const OtaImageHeader *hdr = (const OtaImageHeader *)BANK2_ADDR;

    // Validate header CRC (first 60 bytes)
    uint32_t headerCrc = crc32Calc((const uint8_t *)hdr, 60);
    if (headerCrc != hdr->headCrc) {
        return -1;  // Header corrupt
    }

    // Basic sanity checks
    uint32_t payloadSize = hdr->dataSize;
    if (payloadSize == 0 || payloadSize > (APP_NUM_SECTORS * SECTOR_SIZE)) {
        return -1;  // Invalid size
    }
    if (hdr->imgType != 0) {  // IMAGE_TYPE_APP = 0
        return -1;
    }

    // Read payload CRC check data (4 bytes after payload)
    const uint8_t *payloadSrc = (const uint8_t *)(BANK2_ADDR + IMG_HEADER_SIZE);
    uint32_t storedCrc = *(const uint32_t *)(payloadSrc + payloadSize);

    // Verify payload CRC in Bank 2 before erasing Bank 1
    uint32_t payloadCrc = crc32Calc(payloadSrc, payloadSize);
    if (payloadCrc != storedCrc) {
        return -1;  // Payload corrupt
    }

    // Erase Bank 1 app region (sectors 8-126, preserving bootloader + config)
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef eraseInit;
    uint32_t sectorError = 0;
    eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    eraseInit.Banks     = FLASH_BANK_1;
    eraseInit.Sector    = APP_START_SECTOR;
    eraseInit.NbSectors = APP_NUM_SECTORS;

    if (HAL_FLASHEx_Erase(&eraseInit, &sectorError) != HAL_OK) {
        HAL_FLASH_Lock();
        return -1;
    }

    // Copy payload from Bank 2 to Bank 1 app region
    uint32_t dstAddr = APP_ADDR;  // 0x08010000
    uint32_t remaining = payloadSize;
    uint32_t srcOffset = 0;

    while (remaining > 0) {
        uint8_t quadWord[FLASH_QUAD_WORD];
        uint32_t chunkSize = (remaining >= FLASH_QUAD_WORD) ? FLASH_QUAD_WORD : remaining;

        // Copy chunk (pad with 0xFF if last partial quad-word)
        memcpy(quadWord, payloadSrc + srcOffset, chunkSize);
        if (chunkSize < FLASH_QUAD_WORD) {
            memset(quadWord + chunkSize, 0xFF, FLASH_QUAD_WORD - chunkSize);
        }

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD,
                              dstAddr,
                              (uint32_t)(uintptr_t)quadWord) != HAL_OK) {
            HAL_FLASH_Lock();
            return -1;
        }

        // Blink LED during copy to show progress
        if ((srcOffset & 0xFFFF) == 0) {
            LED_TOGGLE();
        }

        dstAddr   += FLASH_QUAD_WORD;
        srcOffset += chunkSize;
        remaining -= chunkSize;
    }

    HAL_FLASH_Lock();

    // Verify: CRC32 of copied data at Bank 1
    uint32_t verifyCrc = crc32Calc((const uint8_t *)APP_ADDR, payloadSize);
    if (verifyCrc != storedCrc) {
        return -1;  // Copy verification failed
    }

    return 0;  // Success
}

//-----------------------------------------------------------------------------
// CRC32 (same algorithm as CycloneBOOT and image_pack.py)
//-----------------------------------------------------------------------------

/**
 * @brief CRC32 calculation (init 0xFFFFFFFF, poly 0xEDB88320)
 * @return Raw accumulator (no final XOR) — matches CycloneBOOT stored format
 */
static uint32_t crc32Calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 1U) {
                crc = (crc >> 1) ^ 0xEDB88320U;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;  // No final XOR — stored as raw accumulator
}

//-----------------------------------------------------------------------------
// Jump to application
//-----------------------------------------------------------------------------

static void jumpToApp(uint32_t addr)
{
    uint32_t appSp = *(volatile uint32_t *)(addr);
    uint32_t appPc = *(volatile uint32_t *)(addr + 4);

    // Disable SysTick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    // Disable all interrupts and clear pending
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    SCB->SHCSR = 0;
    SCB->VTOR = addr;

    __DSB();
    __ISB();

    __set_MSP(appSp);
    ((void (*)(void))(appPc))();
    while (1);
}

//-----------------------------------------------------------------------------
// LED init (raw register)
//-----------------------------------------------------------------------------

static void initLed(void)
{
    *(volatile uint32_t *)0x44020C8CU |= (1U << 1);  // GPIOB clock
    for (volatile int i = 0; i < 10; i++);
    uint32_t moder = *(volatile uint32_t *)0x42020400U;
    moder &= ~(3U << 26);
    moder |= (1U << 26);  // PB13 output
    *(volatile uint32_t *)0x42020400U = moder;
}

//-----------------------------------------------------------------------------
// CycloneBOOT stubs (required by linker, not used in direct-flash boot path)
//-----------------------------------------------------------------------------

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
// CycloneBOOT v2.6.2 weak hooks (required by linker)
//-----------------------------------------------------------------------------

__attribute__((weak)) void bootInitHook(void) {}
__attribute__((weak)) void bootIdleStateHook(void) {}
__attribute__((weak)) void bootNoValidUpdatesHook(void) {}
__attribute__((weak)) void bootJumpingToApplicationHook(void) {}
__attribute__((weak)) void bootFallbackPerformedHook(void) {}
__attribute__((weak)) void bootHandleFallbackError(void) {}
__attribute__((weak)) void bootHandleGenericError(void) {}
