/**
 * @file sam_ed_5x_flash_driver.h
 * @brief SAM(E|D)5x CycloneBOOT flash driver
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

#ifndef _SAM_ED_5x_FLASH_DRIVER_H
#define _SAM_ED_5x_FLASH_DRIVER_H

// Dependencies
#include "core/flash.h"
#include "error.h"
#include "sam.h"

#include <stdint.h>
#include <stdlib.h>

// SAM_ED_5x Flash name
#define SAM_ED_5x_FLASH_NAME "SAM_ED_5x Internal Flash"
// SAM_ED_5x Flash start addr
#define SAM_ED_5x_FLASH_ADDR FLASH_ADDR
// SAM_ED_5x Flash write size
#define SAM_ED_5x_FLASH_WRITE_SIZE (4 * 4) // quad-word -> 16bytes
// SAM_ED_5x Flash read size
#define SAM_ED_5x_FLASH_READ_SIZE 0x04 // 4-bytes word

// Device flash size
#define SAM_ED_5x_FLASH_SIZE NVMCTRL_FLASH_SIZE

// Device flash block (sector) size
#define SAM_ED_5x_FLASH_BLOCK_SIZE (FLASH_PAGE_SIZE * 16)

// Device flash regions size (protection granularity)
#if (SAM_ED_5x_FLASH_SIZE == 0x100000)
#define SAM_ED_5x_FLASH_REGION_SIZE (SAM_ED_5x_FLASH_BLOCK_SIZE * 4) // 32KB region (4 blocks)
#elif (SAE54_FLASH_SIZE == 0x80000)
#define SAM_ED_5x_FLASH_REGION_SIZE (SAM_ED_5x_FLASH_BLOCK_SIZE * 2) // 16KB region (2 blocks)
#else
#define SAM_ED_5x_FLASH_REGION_SIZE (SAM_ED_5x_FLASH_BLOCK_SIZE) // 8KB region  (1 block)
#endif

// Device flash sector size
#define SAM_ED_5x_FLASH_SECTOR_SIZE SAM_ED_5x_FLASH_BLOCK_SIZE

// Device flash sector number
#define SAM_ED_5x_FLASH_SECTOR_NUMBER (SAM_ED_5x_FLASH_SIZE / SAM_ED_5x_FLASH_BLOCK_SIZE)

// Device flash bank IDs
#define SAM_ED_5x_FLASH_BANK1_ID 1
#define SAM_ED_5x_FLASH_BANK2_ID 2

// Device flash bank info
#if defined(FLASH_DB_MODE)
#define SAM_ED_5x_FLASH_BANK_SIZE (SAM_ED_5x_FLASH_SIZE / 2)
#define SAM_ED_5x_FLASH_BANK1_ADDR FLASH_ADDR
#define SAM_ED_5x_FLASH_BANK2_ADDR (FLASH_ADDR + SAM_ED_5x_FLASH_BANK_SIZE)
#endif

// SAM_ED_5x Internal Memory Flash driver
extern const FlashDriver sam_ed_5x_FlashDriver;

#endif //!_SAM_ED_5x_FLASH_DRIVER_H
