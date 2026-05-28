/**
 * @file boot_config.h
 * @brief CycloneBOOT configuration for standalone bootloader
 *
 * @section License
 * SPDX-License-Identifier: GPL-2.0-or-later
 **/

#ifndef _BOOT_CONFIG_H
#define _BOOT_CONFIG_H

// MCU / HAL selection
#define STM32H5XX_HAL_DRIVER
#define STM32H563xx

// Flash memory configuration (STM32H563VI - 2MB dual bank)
#define FLASH_START_ADDR                0x08000000
#define FLASH_TOTAL_SIZE                0x200000
#define FLASH_DB_MODE                   1

// Bank 1 configuration
#define FLASH_BANK1_ADDR                0x08000000
#define FLASH_BANK1_SIZE                0x100000

// Bank 2 configuration
#define FLASH_BANK2_ADDR                0x08100000
#define FLASH_BANK2_SIZE                0x100000

// Memory layout
#define NB_MEMORIES                     1
#define NB_MAX_MEMORY_SLOTS             2

// Bootloader configuration
#define BOOT_OFFSET                     0x10000  // 64KB for bootloader
#define BOOT_FALLBACK_SUPPORT           ENABLED
#define BOOT_FALLBACK_AUTO_MODE         ENABLED
#define BOOT_FALLBACK_MANUAL_MODE       DISABLED
#define BOOT_FLAG_SUPPORT               ENABLED
#define BOOT_COUNTER_SUPPORT            ENABLED
#define BOOT_COUNTER_MAX_ATTEMPTS       3

// Update mode configuration
#define UPDATE_SINGLE_BANK_SUPPORT      DISABLED
#define UPDATE_DUAL_BANK_SUPPORT        ENABLED
#define UPDATE_FALLBACK_SUPPORT         ENABLED
#define UPDATE_ANTI_ROLLBACK_SUPPORT    DISABLED

// Security / Verification configuration
#define VERIFY_INTEGRITY_SUPPORT        ENABLED
#define VERIFY_AUTHENTICATION_SUPPORT   DISABLED
#define VERIFY_SIGNATURE_SUPPORT        DISABLED
#define VERIFY_RSA_SUPPORT              DISABLED
#define VERIFY_ECDSA_SUPPORT            DISABLED

// Image encryption configuration
#define CIPHER_SUPPORT                  DISABLED
#define IMAGE_INPUT_ENCRYPTED           DISABLED
#define IMAGE_OUTPUT_ENCRYPTED          DISABLED

#endif // _BOOT_CONFIG_H
