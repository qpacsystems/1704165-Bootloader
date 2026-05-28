/**
 * @file stm32f7xx_flash_driver.h
 * @brief CycloneBOOT STM32F7xx Flash Driver
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

#ifndef _STM32F7xx_FLASH_DRIVER_H
#define _STM32F7xx_FLASH_DRIVER_H

// Dependencies
#include "core/flash.h"
#include "error.h"

#include <stdint.h>
#include <stdlib.h>

// STM32F7xx name
#define STM32F7xx_NAME "STM32F7xx Internal Flash"
// STM32F7xx start addr
#define STM32F7xx_ADDR 0x08000000
// STM32F7xx write size
#define STM32F7xx_WRITE_SIZE 0x04 // 4-bytes word
// STM32F7xx read size
#define STM32F7xx_READ_SIZE 0x04 // 4-bytes word

#ifndef FLASH_DB_MODE
#ifdef FLASH_1MB

// STM32F7xx size
#define STM32F7xx_SIZE 0x100000
// STM32F7xx sectors number
#define STM32F7xx_SECTORS_NUMBER 8

// Flash memory single bank organization
// 1 MBytes STM32F7xx Sector start address list (from STM32F7 datasheet)
#define STM32F7xx_SECTOR_0_ADDR                                                                    \
        0x08000000 // Base address of Sector 0,  End address at 0x00200000, 32 Kbytes
// | 32 Kbytes
#define STM32F7xx_SECTOR_1_ADDR                                                                    \
        0x08008000 // Base address of Sector 1,  End address at 0x00208000, 32 Kbytes
// | 64 Kbytes
#define STM32F7xx_SECTOR_2_ADDR                                                                    \
        0x08010000 // Base address of Sector 2,  End address at 0x00210000, 32 Kbytes
// | 96 Kbytes
#define STM32F7xx_SECTOR_3_ADDR                                                                    \
        0x08018000 // Base address of Sector 3,  End address at 0x00218000, 32 Kbytes
// | 128 Kbytes
#define STM32F7xx_SECTOR_4_ADDR                                                                    \
        0x08020000 // Base address of Sector 4,  End address at 0x00220000, 128 Kbytes
// | 256 Kbytes
#define STM32F7xx_SECTOR_5_ADDR                                                                    \
        0x08040000 // Base address of Sector 5,  End address at 0x00240000, 256 Kbytes
// | 512 Kbytes
#define STM32F7xx_SECTOR_6_ADDR                                                                    \
        0x08080000 // Base address of Sector 6,  End address at 0x00280000, 256 Kbytes
// | 768 Kbytes
#define STM32F7xx_SECTOR_7_ADDR                                                                    \
        0x080C0000 // Base address of Sector 7,  End address at 0x002C0000, 256 Kbytes
// | 1024 Kbytes
// 1 MBytes STM32F7xx Sectors size list (from STM32F7 datasheet)
#define STM32F7xx_SECTOR_0_SIZE 0x8000
#define STM32F7xx_SECTOR_1_SIZE 0x8000
#define STM32F7xx_SECTOR_2_SIZE 0x8000
#define STM32F7xx_SECTOR_3_SIZE 0x8000
#define STM32F7xx_SECTOR_4_SIZE 0x20000
#define STM32F7xx_SECTOR_5_SIZE 0x40000
#define STM32F7xx_SECTOR_6_SIZE 0x40000
#define STM32F7xx_SECTOR_7_SIZE 0x40000

#else

// STM32F7xx size
#define STM32F7xx_SIZE 0x200000
// STM32F7xx sectors number
#define STM32F7xx_SECTORS_NUMBER 12

// 2 MBytes STM32F7xx Sector start address list (from STM32F7 datasheet)
#define STM32F7xx_SECTOR_0_ADDR                                                                    \
        0x08000000 // Base address of Sector 0,  End address at 0x08007FFF, 32 Kbytes
// | 32 Kbytes
#define STM32F7xx_SECTOR_1_ADDR                                                                    \
        0x08008000 // Base address of Sector 1,  End address at 0x0800FFFF, 32 Kbytes
// | 64 Kbytes
#define STM32F7xx_SECTOR_2_ADDR                                                                    \
        0x08010000 // Base address of Sector 2,  End address at 0x08017FFF, 32 Kbytes
// | 96 Kbytes
#define STM32F7xx_SECTOR_3_ADDR                                                                    \
        0x08018000 // Base address of Sector 3,  End address at 0x0801FFFF, 32 Kbytes
// | 128 Kbytes
#define STM32F7xx_SECTOR_4_ADDR                                                                    \
        0x08020000 // Base address of Sector 4,  End address at 0x0803FFFF, 128 Kbytes
// | 256 Kbytes
#define STM32F7xx_SECTOR_5_ADDR                                                                    \
        0x08040000 // Base address of Sector 5,  End address at 0x0807FFFF, 256 Kbytes
// | 512 Kbytes
#define STM32F7xx_SECTOR_6_ADDR                                                                    \
        0x08080000 // Base address of Sector 6,  End address at 0x080BFFFF, 256 Kbytes
// | 768 Kbytes
#define STM32F7xx_SECTOR_7_ADDR                                                                    \
        0x080C0000 // Base address of Sector 7,  End address at 0x080FFFFF, 256 Kbytes
// | 1024 Kbytes
#define STM32F7xx_SECTOR_8_ADDR                                                                    \
        0x08100000 // Base address of Sector 8,  End address at 0x0813FFFF, 256 Kbytes
// | 1280 Kbytes
#define STM32F7xx_SECTOR_9_ADDR                                                                    \
        0x08140000 // Base address of Sector 9,  End address at 0x0817FFFF, 256 Kbytes
// | 1536 Kbytes
#define STM32F7xx_SECTOR_10_ADDR                                                                   \
        0x08180000 // Base address of Sector 10, End address at 0x081BFFFF, 256 Kbytes
// | 1792 Kbytes
#define STM32F7xx_SECTOR_11_ADDR                                                                   \
        0x081C0000 // Base address of Sector 11, End address at 0x081FFFFF, 256 Kbytes
// | 2048 Kbytes
// 2 MBytes STM32F7xx Sectors size list (from STM32F7 datasheet)
#define STM32F7xx_SECTOR_0_SIZE 0x8000
#define STM32F7xx_SECTOR_1_SIZE 0x8000
#define STM32F7xx_SECTOR_2_SIZE 0x8000
#define STM32F7xx_SECTOR_3_SIZE 0x8000
#define STM32F7xx_SECTOR_4_SIZE 0x20000
#define STM32F7xx_SECTOR_5_SIZE 0x40000
#define STM32F7xx_SECTOR_6_SIZE 0x40000
#define STM32F7xx_SECTOR_7_SIZE 0x40000
#define STM32F7xx_SECTOR_8_SIZE 0x40000
#define STM32F7xx_SECTOR_9_SIZE 0x40000
#define STM32F7xx_SECTOR_10_SIZE 0x40000
#define STM32F7xx_SECTOR_11_SIZE 0x40000

#endif

#else

// STM32F7xx Banks ID list
#define STM32F7xx_BANK_1_ID 0
#define STM32F7xx_BANK_2_ID 1

#ifdef FLASH_1MB

// STM32F7xx size
#define STM32F7xx_SIZE 0x100000
// STM32F7xx sectors number
#define STM32F7xx_SECTORS_NUMBER 16
// STM32F7xx Bank 1 Size
#define STM32F7xx_BANK_1_SIZE 0x80000
// STM32F7xx Bank 2 Size
#define STM32F7xx_BANK_2_SIZE 0x80000

// Flash memory dual bank organization
// 1 MBytes STM32F7xx Sector start address list (from STM32F7 datasheet)
#define STM32F7xx_SECTOR_0_ADDR                                                                    \
        0x08000000 // Base address of Sector 0,  End address at 0x08003FFF, 16 Kbytes
// | 16 Kbytes
#define STM32F7xx_SECTOR_1_ADDR                                                                    \
        0x08004000 // Base address of Sector 1,  End address at 0x08007FFF, 16 Kbytes
// | 32 Kbytes
#define STM32F7xx_SECTOR_2_ADDR                                                                    \
        0x08008000 // Base address of Sector 2,  End address at 0x0800BFFF, 16 Kbytes
// | 48 Kbytes
#define STM32F7xx_SECTOR_3_ADDR                                                                    \
        0x0800C000 // Base address of Sector 3,  End address at 0x0800FFFF, 16 Kbytes
// | 64 Kbytes
#define STM32F7xx_SECTOR_4_ADDR                                                                    \
        0x08010000 // Base address of Sector 4,  End address at 0x0801FFFF, 64 Kbytes
// | 128 Kbytes
#define STM32F7xx_SECTOR_5_ADDR                                                                    \
        0x08020000 // Base address of Sector 5,  End address at 0x0803FFFF, 128 Kbytes
// | 256 Kbytes
#define STM32F7xx_SECTOR_6_ADDR                                                                    \
        0x08040000 // Base address of Sector 6,  End address at 0x0805FFFF, 128 Kbytes
// | 384 Kbytes
#define STM32F7xx_SECTOR_7_ADDR                                                                    \
        0x08060000 // Base address of Sector 7,  End address at 0x0807FFFF, 128 Kbytes
// | 512 Kbytes
#define STM32F7xx_SECTOR_12_ADDR                                                                   \
        0x08080000 // Base address of Sector 12, End address at 0x08083FFF, 16 Kbytes
// | 528 Kbytes
#define STM32F7xx_SECTOR_13_ADDR                                                                   \
        0x08084000 // Base address of Sector 13, End address at 0x08087FFF, 16 Kbytes
// | 544 Kbytes
#define STM32F7xx_SECTOR_14_ADDR                                                                   \
        0x08088000 // Base address of Sector 14, End address at 0x0808BFFF, 16 Kbytes
// | 560 Kbytes
#define STM32F7xx_SECTOR_15_ADDR                                                                   \
        0x0808C000 // Base address of Sector 15, End address at 0x0808FFFF, 16 Kbytes
// | 576 Kbytes
#define STM32F7xx_SECTOR_16_ADDR                                                                   \
        0x08090000 // Base address of Sector 16, End address at 0x0809FFFF, 64 Kbytes
// | 640 Kbytes
#define STM32F7xx_SECTOR_17_ADDR                                                                   \
        0x080A0000 // Base address of Sector 17, End address at 0x080BFFFF, 128 Kbytes
// | 768 Kbytes
#define STM32F7xx_SECTOR_18_ADDR                                                                   \
        0x080C0000 // Base address of Sector 18, End address at 0x080EFFFF, 128 Kbytes
// | 896 Kbytes
#define STM32F7xx_SECTOR_19_ADDR                                                                   \
        0x080E0000 // Base address of Sector 19, End address at 0x080FFFFF, 128 Kbytes
// | 1024 Kbytes
// 1 MBytes STM32F7xx Sectors size list (from STM32F7 datasheet)
#define STM32F7xx_SECTOR_0_SIZE 0x4000
#define STM32F7xx_SECTOR_1_SIZE 0x4000
#define STM32F7xx_SECTOR_2_SIZE 0x4000
#define STM32F7xx_SECTOR_3_SIZE 0x4000
#define STM32F7xx_SECTOR_4_SIZE 0x10000
#define STM32F7xx_SECTOR_5_SIZE 0x20000
#define STM32F7xx_SECTOR_6_SIZE 0x20000
#define STM32F7xx_SECTOR_7_SIZE 0x20000
#define STM32F7xx_SECTOR_12_SIZE 0x4000
#define STM32F7xx_SECTOR_13_SIZE 0x4000
#define STM32F7xx_SECTOR_14_SIZE 0x4000
#define STM32F7xx_SECTOR_15_SIZE 0x4000
#define STM32F7xx_SECTOR_16_SIZE 0x10000
#define STM32F7xx_SECTOR_17_SIZE 0x20000
#define STM32F7xx_SECTOR_18_SIZE 0x20000
#define STM32F7xx_SECTOR_19_SIZE 0x20000

#else

// STM32F7xx size
#define STM32F7xx_SIZE 0x200000
// STM32F7xx sectors number
#define STM32F7xx_SECTORS_NUMBER 24
// STM32F7xx Bank 1 Size
#define STM32F7xx_BANK_1_SIZE 0x100000
// STM32F7xx Bank 2 Size
#define STM32F7xx_BANK_2_SIZE 0x100000

// 2 MBytes STM32F7xx Sector start address list (from STM32F7 datasheet)
#define STM32F7xx_SECTOR_0_ADDR                                                                    \
        0x08000000 // Base address of Sector 0,  End address at 0x08003FFF, 16 Kbytes
// | 16 Kbytes
#define STM32F7xx_SECTOR_1_ADDR                                                                    \
        0x08004000 // Base address of Sector 1,  End address at 0x08007FFF, 16 Kbytes
// | 32 Kbytes
#define STM32F7xx_SECTOR_2_ADDR                                                                    \
        0x08008000 // Base address of Sector 2,  End address at 0x0800BFFF, 16 Kbytes
// | 48 Kbytes
#define STM32F7xx_SECTOR_3_ADDR                                                                    \
        0x0800C000 // Base address of Sector 3,  End address at 0x0800FFFF, 16 Kbytes
// | 64 Kbytes
#define STM32F7xx_SECTOR_4_ADDR                                                                    \
        0x08010000 // Base address of Sector 4,  End address at 0x0801FFFF, 64 Kbytes
// | 128 Kbytes
#define STM32F7xx_SECTOR_5_ADDR                                                                    \
        0x08020000 // Base address of Sector 5,  End address at 0x0803FFFF, 128 Kbytes
// | 256 Kbytes
#define STM32F7xx_SECTOR_6_ADDR                                                                    \
        0x08040000 // Base address of Sector 6,  End address at 0x0805FFFF, 128 Kbytes
// | 384 Kbytes
#define STM32F7xx_SECTOR_7_ADDR                                                                    \
        0x08060000 // Base address of Sector 7,  End address at 0x0807FFFF, 128 Kbytes
// | 512 Kbytes
#define STM32F7xx_SECTOR_8_ADDR                                                                    \
        0x08080000 // Base address of Sector 8,  End address at 0x0809FFFF, 128 Kbytes
// | 640 Kbytes
#define STM32F7xx_SECTOR_9_ADDR                                                                    \
        0x080A0000 // Base address of Sector 9,  End address at 0x080BFFFF, 128 Kbytes
// | 768 Kbytes
#define STM32F7xx_SECTOR_10_ADDR                                                                   \
        0x080C0000 // Base address of Sector 10, End address at 0x080EFFFF, 128 Kbytes
// | 986 Kbytes
#define STM32F7xx_SECTOR_11_ADDR                                                                   \
        0x080E0000 // Base address of Sector 11, End address at 0x080FFFFF, 128 Kbytes
// | 1024 Kbytes
#define STM32F7xx_SECTOR_12_ADDR                                                                   \
        0x08100000 // Base address of Sector 12, End address at 0x08103FFF, 16 Kbytes
// | 1040 Kbytes
#define STM32F7xx_SECTOR_13_ADDR                                                                   \
        0x08104000 // Base address of Sector 13, End address at 0x08107FFF, 16 Kbytes
// | 1056 Kbytes
#define STM32F7xx_SECTOR_14_ADDR                                                                   \
        0x08108000 // Base address of Sector 14, End address at 0x0810BFFF, 16 Kbytes
// | 1072 Kbytes
#define STM32F7xx_SECTOR_15_ADDR                                                                   \
        0x0810C000 // Base address of Sector 15, End address at 0x0810FFFF, 16 Kbytes
// | 1088 Kbytes
#define STM32F7xx_SECTOR_16_ADDR                                                                   \
        0x08110000 // Base address of Sector 16, End address at 0x0811FFFF, 64 Kbytes
// | 1152 Kbytes
#define STM32F7xx_SECTOR_17_ADDR                                                                   \
        0x08120000 // Base address of Sector 17, End address at 0x0813FFFF, 128 Kbytes
// | 1280 Kbytes
#define STM32F7xx_SECTOR_18_ADDR                                                                   \
        0x08140000 // Base address of Sector 18, End address at 0x0815FFFF, 128 Kbytes
// | 1408 Kbytes
#define STM32F7xx_SECTOR_19_ADDR                                                                   \
        0x08160000 // Base address of Sector 19, End address at 0x0817FFFF, 128 Kbytes
// | 1536 Kbytes
#define STM32F7xx_SECTOR_20_ADDR                                                                   \
        0x08180000 // Base address of Sector 20, End address at 0x0819FFFF, 128 Kbytes
// | 1664 Kbytes
#define STM32F7xx_SECTOR_21_ADDR                                                                   \
        0x081A0000 // Base address of Sector 21, End address at 0x081BFFFF, 128 Kbytes
// | 1792 Kbytes
#define STM32F7xx_SECTOR_22_ADDR                                                                   \
        0x081C0000 // Base address of Sector 22, End address at 0x081EFFFF, 128 Kbytes
// | 1920 Kbytes
#define STM32F7xx_SECTOR_23_ADDR                                                                   \
        0x081E0000 // Base address of Sector 23, End address at 0x081FFFFF, 128 Kbytes
// | 2048 Kbytes
// 2 MBytes STM32F7xx Sectors size list (from STM32F7 datasheet)
#define STM32F7xx_SECTOR_0_SIZE 0x4000
#define STM32F7xx_SECTOR_1_SIZE 0x4000
#define STM32F7xx_SECTOR_2_SIZE 0x4000
#define STM32F7xx_SECTOR_3_SIZE 0x4000
#define STM32F7xx_SECTOR_4_SIZE 0x10000
#define STM32F7xx_SECTOR_5_SIZE 0x20000
#define STM32F7xx_SECTOR_6_SIZE 0x20000
#define STM32F7xx_SECTOR_7_SIZE 0x20000
#define STM32F7xx_SECTOR_8_SIZE 0x20000
#define STM32F7xx_SECTOR_9_SIZE 0x20000
#define STM32F7xx_SECTOR_10_SIZE 0x20000
#define STM32F7xx_SECTOR_11_SIZE 0x20000
#define STM32F7xx_SECTOR_12_SIZE 0x4000
#define STM32F7xx_SECTOR_13_SIZE 0x4000
#define STM32F7xx_SECTOR_14_SIZE 0x4000
#define STM32F7xx_SECTOR_15_SIZE 0x4000
#define STM32F7xx_SECTOR_16_SIZE 0x10000
#define STM32F7xx_SECTOR_17_SIZE 0x20000
#define STM32F7xx_SECTOR_18_SIZE 0x20000
#define STM32F7xx_SECTOR_19_SIZE 0x20000
#define STM32F7xx_SECTOR_20_SIZE 0x20000
#define STM32F7xx_SECTOR_21_SIZE 0x20000
#define STM32F7xx_SECTOR_22_SIZE 0x20000
#define STM32F7xx_SECTOR_23_SIZE 0x20000
#endif

// STM32F7xx Bank 1 start address
#define STM32F7xx_BANK_1_ADDR STM32F7xx_SECTOR_0_ADDR
// STM32F7xx Bank 2 start address
#define STM32F7xx_BANK_2_ADDR STM32F7xx_SECTOR_12_ADDR

#endif

// STM32F7xx Internal Memory Flash driver
extern const FlashDriver stm32f7xxFlashDriver;

#endif //!_STM32F7xx_FLASH_DRIVER_H
