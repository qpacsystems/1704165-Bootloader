/**
 * @file sam_ed_7x_flash_driver.h
 * @brief SAM(E|D)7x CycloneBOOT flash driver
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

#ifndef _SAM_ED_7X_DRIVER_H
#define _SAM_ED_7X_DRIVER_H

#include "component/efc.h"
#include "core/flash.h"
#include "plib_efc.h"
#include "same70q21b.h"

// #define EFC ((Efc *)(uint32_t *)EFC_BASE_ADDRESS)

#define EFC_ACCESS_MODE_128 0
#if (!(SAMV71 || SAMV70 || SAMS70 || SAME70))
#define EFC_ACCESS_MODE_64 EEFC_FMR_FAM
#endif

#define MEMORY_LOWER_BOUND 0x00400000u
#define MEMORY_UPPER_BOUND MEMORY_LOWER_BOUND + 0x200000u

const extern FlashDriver same7x_driver;

// SAM_ED_5x Flash name
#define SAM_ED_7x_FLASH_NAME "SAM_ED_7x Internal Flash"
// SAM_ED_5x Flash start address
#define SAM_ED_7x_FLASH_ADDR IFLASH_ADDR
// SAM_ED_5x Flash write size
// #define SAM_ED_7x_FLASH_WRITE_SIZE 4*4 //Quad-word -> 16 bytes
#define SAM_ED_7x_FLASH_WRITE_SIZE 512 // Quad-word -> 16 bytes
// SAM_ED_5x Flash read size
#define SAM_ED_7x_FLASH_READ_SIZE 0x4 // 4-bytes word

// Device flash size
#define SAM_ED_7x_FLASH_SIZE IFLASH_SIZE

// Device flash block (page) size (512 Bytes)
#define SAM_ED_7x_FLASH_BLOCK_SIZE IFLASH_PAGE_SIZE
// Device flash sector size
#define SAM_ED_7x_FLASH_SECTOR_SIZE (IFLASH_PAGE_SIZE * 256) // 128 Kbytes
// Device flash sector number
#define SAM_ED_7x_FLASH_SECTOR_NUMBER (SAM_ED_7x_FLASH_SIZE / SAM_ED_7x_FLASH_SECTOR_SIZE)

#endif //_SAM_ED_7X_DRIVER_H
