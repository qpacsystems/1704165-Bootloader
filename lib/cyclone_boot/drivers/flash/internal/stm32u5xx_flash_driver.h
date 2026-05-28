/**
 * @file stm32u5xx_flash_driver.h
 * @brief CycloneBOOT STM32U5xx Flash Driver
 *
 * @section License
 *
 * Copyright (C) 2021-2026 Oryx Embedded SARL. All rights reserved.
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
 * @version 2.6.2
 **/

#ifndef _STM32U5XX_FLASH_DRIVER_H
#define _STM32U5XX_FLASH_DRIVER_H

// Dependencies
#include "core/flash.h"
#include "stm32u5xx_flash_driver.h"
#include "stm32u5xx_hal.h"
#include "stm32u5xx_hal_flash_ex.h"

#include <string.h>

// STM32U5xx name
#define STM32U5xx_FLASH_NAME "STM32U5xx Internal Flash"
// STM32U5xx start address
#define STM32U5xx_FLASH_ADDR 0x08000000UL
// STM32U5xx flash size
#define STM32U5xx_FLASH_SIZE 0x200000U
// STM32U5xx write size
#define STM32U5xx_FLASH_WRITE_SIZE 16 // N-bytes word
// STM32U5xx read size
#define STM32U5xx_FLASH_READ_SIZE                                                                  \
        16 // 16 bytes (quad-word) also : 8 bytes (double-word), 4 bytes (word), 2
// bytes and 1 byte

// Device flash sector size
// #define FLASH_SECTOR_SIZE 0x00002000UL
#define STM32U5xx_FLASH_SECTOR_SIZE FLASH_PAGE_SIZE

// Device flash bank IDs
#define STM32U5xx_FLASH_BANK1_ID 1
#define STM32U5xx_FLASH_BANK2_ID 2

// Device flash bank info
#define FLASH_BANK1_BASE (0x08000000UL)
#define FLASH_BANK2_BASE (0x08100000UL)

#if defined(FLASH_DB_MODE)
#define STM32U5xx_FLASH_BANK_SIZE STM32U5xx_FLASH_SIZE / 2
#define STM32U5xx_FLASH_BANK1_ADDR FLASH_BANK1_BASE
#define STM32U5xx_FLASH_BANK2_ADDR FLASH_BANK2_BASE

// Device flash sector number
#define FLASH_SECTOR_SIZE STM32U5xx_FLASH_SECTOR_SIZE // 8 Kbytes
#define STM32U5xx_FLASH_SECTOR_NUMBER 256

#else
#define STM32U5xx_FLASH_BANK_SIZE STM32U5xx_FLASH_SIZE
#define STM32U5xx_FLASH_BANK1_ADDR FLASH_BANK1_BASE
#define STM32U5xx_FLASH_BANK2_ADDR FLASH_BANK2_BASE

// Device flash sector number
#define FLASH_SECTOR_SIZE STM32U5xx_FLASH_SECTOR_SIZE * 2 // 16 Kbytes
#define STM32U5xx_FLASH_SECTOR_NUMBER 128
#endif

// STM32U5xx Internal Memory Flash driver
const extern FlashDriver stm32u5xxFlashDriver;

#endif /* _STM32U5XX_FLASH_DRIVER_H */
