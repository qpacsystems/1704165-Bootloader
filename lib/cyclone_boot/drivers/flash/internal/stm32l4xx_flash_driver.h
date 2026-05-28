/**
 * @file stm32l4xx_flash_driver.h
 * @brief CycloneBOOT STM32L4xx Flash Driver
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

#ifndef _STM32L4xx_FLASH_DRIVER_H
#define _STM32L4xx_FLASH_DRIVER_H

// Dependencies
#include "core/flash.h"
#include "error.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_flash.h"

#include <stdint.h>
#include <stdlib.h>

// STM32L4xx name
#define STM32L4xx_FLASH_NAME "STM32L4xx Internal Flash"
// STM32L4xx start addr
#define STM32L4xx_FLASH_ADDR FLASH_BASE
// STM32L4xx write size
#define STM32L4xx_FLASH_WRITE_SIZE (4 * 2) // N-bytes word
// STM32L4xx read size
#define STM32L4xx_FLASH_READ_SIZE 0x4 // 32-bytes word

// Device flash size
#if (defined(STM32L47xG) || defined(STM32L48xx) || defined(STM32L49xG) || defined(STM32L4A6xx))
#define STM32L4xx_FLASH_SIZE 0x100000 // 1MB flash
#elif (defined(STM32L47xE) || defined(STM32L49xE))
#define STM32L4xx_FLASH_SIZE 0x80000 // 512KB flash
#elif (defined(STM32L47xC))
#define STM32L4xx_FLASH_SIZE 0x40000 // 256KB flash
#else
#error You MUST define one of the above device part number!
#endif

// Device flash sector size
#define STM32L4xx_FLASH_SECTOR_SIZE 0x800 // 2KB sector

// Device flash sector number
#define STM32L4xx_FLASH_SECTOR_NUMBER (STM32L4xx_FLASH_SIZE / STM32L4xx_FLASH_SECTOR_SIZE)

// Device flash bank IDs
#define STM32L4xx_FLASH_BANK1_ID 1
#define STM32L4xx_FLASH_BANK2_ID 2

// Device flash bank info
#if defined(FLASH_DB_MODE)
#define STM32L4xx_FLASH_BANK_SIZE STM32L4xx_FLASH_SIZE / 2
#define STM32L4xx_FLASH_BANK1_ADDR FLASH_BASE
#define STM32L4xx_FLASH_BANK2_ADDR FLASH_BASE + STM32L4xx_FLASH_BANK_SIZE
#else
#define STM32L4xx_FLASH_BANK_SIZE STM32L4xx_FLASH_SIZE
#define STM32L4xx_FLASH_BANK1_ADDR FLASH_BASE
#define STM32L4xx_FLASH_BANK2_ADDR FLASH_BASE
#endif

// STM32L4xx Internal Memory Flash driver
extern const FlashDriver stm32l4xxFlashDriver;

#endif //!_STM32L4xx_FLASH_DRIVER_H
