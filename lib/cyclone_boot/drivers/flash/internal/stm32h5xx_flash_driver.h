/**
 * @file stm32h5xx_flash_driver.h
 * @brief CycloneBOOT STM32H5xx Flash Driver
 *
 * @section License
 *
 * Copyright (C) 2021-2025 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneBOOT Open
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
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
 * @version 2.5.4-revb
 **/

#ifndef _STM32H5XX_FLASH_DRIVER_H
#define _STM32H5XX_FLASH_DRIVER_H

// Dependencies
// Pull in global boot configuration so FLASH_DB_MODE and related options
// are consistently applied to this driver.
#include "boot_config.h"
#include "core/flash.h"
#include "error.h"
#include "stm32h5xx_flash_driver.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_flash_ex.h"

// STM32H5xx name
#define STM32H5xx_FLASH_NAME "STM32H5xx Internal Flash"
// STM32H5xx start address
#define STM32H5xx_FLASH_ADDR 0x08000000
// STM32H5xx flash size
#define STM32H5xx_FLASH_SIZE 0x200000
// STM32H5xx write size
#define STM32H5xx_FLASH_WRITE_SIZE 16 // N-bytes word
// STM32H5xx read size
#define STM32H5xx_FLASH_READ_SIZE                                                                  \
        16 // 16 bytes (quad-word) also : 8 bytes (double-word), 4 bytes (word), 2
// bytes and 1 byte

// Device flash sector size
#define STM32H5xx_FLASH_SECTOR_SIZE FLASH_SECTOR_SIZE

// Device flash sector number
#define STM32H5xx_FLASH_SECTOR_NUMBER                                                              \
        FLASH_SECTOR_NB *(STM32H5xx_FLASH_SIZE / FLASH_SECTOR_NB / FLASH_SECTOR_SIZE)

// Device flash bank IDs
#define STM32H5xx_FLASH_BANK1_ID FLASH_BANK_1
#define STM32H5xx_FLASH_BANK2_ID FLASH_BANK_2

// Device flash bank info
#if defined(FLASH_DB_MODE)
#define STM32H5xx_FLASH_BANK_SIZE STM32H5xx_FLASH_SIZE >> 1
#define STM32H5xx_FLASH_BANK1_ADDR 0x08000000
#define STM32H5xx_FLASH_BANK2_ADDR 0x08100000
#else
#define STM32H5xx_FLASH_BANK_SIZE STM32H5xx_FLASH_SIZE
#define STM32H5xx_FLASH_BANK1_ADDR 0x08000000
#define STM32H5xx_FLASH_BANK2_ADDR 0x08000000
#endif

// STM32H5xx Internal Memory Flash driver
extern const FlashDriver stm32h5xxFlashDriver;

#endif /* _STM32H5XX_FLASH_DRIVER_H */
