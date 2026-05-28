/**
 * @file boot_config.h
 * @brief CycloneBOOT configuration file
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2021-2025 Oryx Embedded SARL. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.5.4
 **/

#ifndef _BOOT_CONFIG_H
#define _BOOT_CONFIG_H

//-----------------------------------------------------------------------------
// GPL License Acceptance (required for open source version)
//-----------------------------------------------------------------------------
#define GPL_LICENSE_TERMS_ACCEPTED

//-----------------------------------------------------------------------------
// Target MCU Selection - STM32H5xx
//-----------------------------------------------------------------------------
// Note: Driver headers are included where needed in source files, not here
// to avoid circular dependencies. Use:
//   #include "drivers/mcu_core/stm32h5xx_mcu_driver.h"
//   #include "drivers/flash/internal/stm32h5xx_flash_driver.h"

//-----------------------------------------------------------------------------
// Flash Memory Configuration - STM32H5xx (2MB dual-bank)
//-----------------------------------------------------------------------------
// Flash start address
#define FLASH_START_ADDR           0x08000000
// Total flash size (2MB)
#define FLASH_TOTAL_SIZE           0x200000
// Enable dual-bank mode
#define FLASH_DB_MODE              1

// Bank 1 configuration
#define FLASH_BANK1_ADDR           0x08000000
#define FLASH_BANK1_SIZE           0x100000    // 1MB

// Bank 2 configuration
#define FLASH_BANK2_ADDR           0x08100000
#define FLASH_BANK2_SIZE           0x100000    // 1MB

//-----------------------------------------------------------------------------
// Memory Layout Configuration
//-----------------------------------------------------------------------------
// Number of memory devices (1 = internal flash only)
#define NB_MEMORIES                1

// Maximum number of slots per memory
#define NB_MAX_MEMORY_SLOTS        2

// Disable external memory support (enable if using external SPI/QSPI flash)
#define EXTERNAL_MEMORY_SUPPORT    DISABLED

// Disable filesystem layer support
#define MEMORIES_FS_SUPPORT        DISABLED

//-----------------------------------------------------------------------------
// Bootloader Configuration
//-----------------------------------------------------------------------------
// Bootloader offset from flash start (where bootloader resides)
// Adjust based on your bootloader size
#define BOOT_OFFSET                0x10000     // 64KB for bootloader

// Enable fallback support for automatic revert on failed boot
#define BOOT_FALLBACK_SUPPORT      ENABLED

// Enable automatic fallback mode
#define BOOT_FALLBACK_AUTO_MODE    ENABLED

// Disable manual fallback mode
#define BOOT_FALLBACK_MANUAL_MODE  DISABLED

// Enable boot flag support (tracks boot success/failure)
#define BOOT_FLAG_SUPPORT          ENABLED

// Enable boot counter support (counts attempts before fallback)
#define BOOT_COUNTER_SUPPORT       ENABLED

// Maximum boot attempts before fallback (if counter enabled)
#define BOOT_COUNTER_MAX_ATTEMPTS  3

// Disable external memory encryption
#define BOOT_EXT_MEM_ENCRYPTION_SUPPORT DISABLED

// Disable custom MCU deinitialization
#define BOOT_CUSTOM_DEINIT_SUPPORT DISABLED

//-----------------------------------------------------------------------------
// Update Mode Configuration
//-----------------------------------------------------------------------------
// Disable single bank mode
#define UPDATE_SINGLE_BANK_SUPPORT DISABLED

// Enable dual bank mode (recommended for STM32H5xx)
#define UPDATE_DUAL_BANK_SUPPORT   ENABLED

// Enable update fallback support (revert on failed update)
#define UPDATE_FALLBACK_SUPPORT    ENABLED

// Disable anti-rollback protection (enable for production)
#define UPDATE_ANTI_ROLLBACK_SUPPORT DISABLED

//-----------------------------------------------------------------------------
// Security / Verification Configuration
//-----------------------------------------------------------------------------
// Enable integrity verification (CRC32/Hash)
#define VERIFY_INTEGRITY_SUPPORT   ENABLED

// Disable HMAC/MAC authentication (enable for secure updates)
#define VERIFY_AUTHENTICATION_SUPPORT DISABLED

// Disable digital signature verification (enable for secure updates)
#define VERIFY_SIGNATURE_SUPPORT   DISABLED

// Disable RSA signature support
#define VERIFY_RSA_SUPPORT         DISABLED

// Disable ECDSA signature support
#define VERIFY_ECDSA_SUPPORT       DISABLED

//-----------------------------------------------------------------------------
// Image Encryption Configuration
//-----------------------------------------------------------------------------
// Disable cipher engine
#define CIPHER_SUPPORT             DISABLED

// Disable input image encryption
#define IMAGE_INPUT_ENCRYPTED      DISABLED

// Disable output image encryption
#define IMAGE_OUTPUT_ENCRYPTED     DISABLED

//-----------------------------------------------------------------------------
// Image Header Configuration
//-----------------------------------------------------------------------------
#define IMAGE_HEADER_VERSION_MAJOR 1
#define IMAGE_HEADER_VERSION_MINOR 2
#define IMAGE_HEADER_VERSION_PATCH 0

// Maximum size for integrity/authentication check data
#define IMAGE_MAX_CHECK_DATA_SIZE  512

//-----------------------------------------------------------------------------
// Mailbox Configuration (for bootloader-application communication)
//-----------------------------------------------------------------------------
#define BOOT_MBX_SIGNATURE         0x1B241671
#define BOOT_MBX_NVM_MAGIC         0xBEEF
#define BOOT_MBX_PSK_MAX_SIZE      32

// NVM Flags for update status
#define BOOT_MBX_NVM_FLAG_UPDATE_REQUESTED   0xF00D
#define BOOT_MBX_NVM_FLAG_UPDATE_PERFORMED   0xC0DE
#define BOOT_MBX_NVM_FLAG_UPDATE_CONFIRMED   0xFEED
#define BOOT_MBX_NVM_FLAG_FALLBACK_REQUESTED 0xB105
#define BOOT_MBX_NVM_FLAG_FALLBACK_PERFORMED 0xDEAD

#endif // _BOOT_CONFIG_H
