/**
 * @file stm32f4xx_flash_driver.h
 * @brief STM32F4xx CycloneBOOT flash driver
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

#ifndef _STM32F4xx_FLASH_DRIVER_H
#define _STM32F4xx_FLASH_DRIVER_H

// Dependencies
#include "core/flash.h"
#include "error.h"

#include <stdint.h>
#include <stdlib.h>

// STM32F4xx name
#define STM32F4xx_NAME "STM32F4xx Internal Flash"
// STM32F4xx start addr
#define STM32F4xx_ADDR 0x08000000
// STM32F4xx write size
#define STM32F4xx_WRITE_SIZE 0x04 // 4-bytes word
// STM32F4xx read size
#define STM32F4xx_READ_SIZE 0x04 // 4-bytes word

// STM32F4xx flash size
#if defined(STM32F4xxx8)
#define STM32F4xx_SIZE 0x10000 // 64KB
#elif defined(STM32F4xxxB)
#define STM32F4xx_SIZE 0x20000 // 128KB
#elif defined(STM32F4xxxC)
#define STM32F4xx_SIZE 0x40000 // 256KB
#elif defined(STM32F4xxxD)
#define STM32F4xx_SIZE 0x60000 // 384KB
#elif defined(STM32F4xxxE)
#define STM32F4xx_SIZE 0x80000 // 512KB
#elif defined(STM32F4xxxG)
#define STM32F4xx_SIZE 0x100000 // 1024KB
#elif defined(STM32F4xxxH)
#define STM32F4xx_SIZE 0x180000 // 1536KB
#elif defined(STM32F4xxxI)
#define STM32F4xx_SIZE 0x200000 // 2048KB
#else
#error You MUST define one of the above device part number!
#endif

#if defined(STM32F410Tx) || defined(STM32F410Cx) || defined(STM32F410Rx) ||                        \
   defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F411xE) ||                        \
   defined(STM32F446xx) || defined(STM32F405xx) || defined(STM32F415xx) ||                        \
   defined(STM32F407xx) || defined(STM32F417xx) || defined(STM32F412Zx) ||                        \
   defined(STM32F412Vx) || defined(STM32F412Rx) || defined(STM32F412Cx) ||                        \
   defined(STM32F413xx) || defined(STM32F423xx)

#if defined(FLASH_DB_MODE)
#error Dual bank organization is not supported on the target STM32F4xx device used!
#endif
#endif

#if defined(STM32F410Tx) || defined(STM32F410Cx) || defined(STM32F410Rx)
// STM32F4xx sectors number
#define STM32F4xx_SECTORS_NUMBER 5

// STM32F4xx Sector start address list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_ADDR                                                                    \
        0x08000000 // Base address of Sector 0,  End address at 0x08003FFF, 16 Kbytes
// | 16 Kbytes
#define STM32F4xx_SECTOR_1_ADDR                                                                    \
        0x08004000 // Base address of Sector 1,  End address at 0x08007FFF, 16 Kbytes
// | 32 Kbytes
#define STM32F4xx_SECTOR_2_ADDR                                                                    \
        0x08008000 // Base address of Sector 2,  End address at 0x0800BFFF, 16 Kbytes
// | 48 Kbytes
#define STM32F4xx_SECTOR_3_ADDR                                                                    \
        0x0800C000 // Base address of Sector 3,  End address at 0x0800FFFF, 16 Kbytes
// | 64 Kbytes
#define STM32F4xx_SECTOR_4_ADDR                                                                    \
        0x08010000 // Base address of Sector 4,  End address at 0x0801FFFF, 64 Kbytes
// | 128 Kbytes

// STM32F4xx Sectors size list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_SIZE 0x4000
#define STM32F4xx_SECTOR_1_SIZE 0x4000
#define STM32F4xx_SECTOR_2_SIZE 0x4000
#define STM32F4xx_SECTOR_3_SIZE 0x4000
#define STM32F4xx_SECTOR_4_SIZE 0x10000

#elif defined(STM32F401xC)
// STM32F4xx sectors number
#define STM32F4xx_SECTORS_NUMBER 6

// STM32F4xx Sector start address list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_ADDR                                                                    \
        0x08000000 // Base address of Sector 0,  End address at 0x08003FFF, 16 Kbytes
// | 16 Kbytes
#define STM32F4xx_SECTOR_1_ADDR                                                                    \
        0x08004000 // Base address of Sector 1,  End address at 0x08007FFF, 16 Kbytes
// | 32 Kbytes
#define STM32F4xx_SECTOR_2_ADDR                                                                    \
        0x08008000 // Base address of Sector 2,  End address at 0x0800BFFF, 16 Kbytes
// | 48 Kbytes
#define STM32F4xx_SECTOR_3_ADDR                                                                    \
        0x0800C000 // Base address of Sector 3,  End address at 0x0800FFFF, 16 Kbytes
// | 64 Kbytes
#define STM32F4xx_SECTOR_4_ADDR                                                                    \
        0x08010000 // Base address of Sector 4,  End address at 0x0801FFFF, 64 Kbytes
// | 128 Kbytes
#define STM32F4xx_SECTOR_5_ADDR                                                                    \
        0x08020000 // Base address of Sector 5,  End address at 0x0803FFFF, 128 Kbytes
// | 256 Kbytes

// STM32F4xx Sectors size list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_SIZE 0x4000
#define STM32F4xx_SECTOR_1_SIZE 0x4000
#define STM32F4xx_SECTOR_2_SIZE 0x4000
#define STM32F4xx_SECTOR_3_SIZE 0x4000
#define STM32F4xx_SECTOR_4_SIZE 0x10000
#define STM32F4xx_SECTOR_5_SIZE 0x20000

#elif defined(STM32F401xE) || defined(STM32F411xE) || defined(STM32F446xx)
// STM32F4xx sectors number
#define STM32F4xx_SECTORS_NUMBER 8

// STM32F4xx Sector start address list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_ADDR                                                                    \
        0x08000000 // Base address of Sector 0,  End address at 0x08003FFF, 16 Kbytes
// | 16 Kbytes
#define STM32F4xx_SECTOR_1_ADDR                                                                    \
        0x08004000 // Base address of Sector 1,  End address at 0x08007FFF, 16 Kbytes
// | 32 Kbytes
#define STM32F4xx_SECTOR_2_ADDR                                                                    \
        0x08008000 // Base address of Sector 2,  End address at 0x0800BFFF, 16 Kbytes
// | 48 Kbytes
#define STM32F4xx_SECTOR_3_ADDR                                                                    \
        0x0800C000 // Base address of Sector 3,  End address at 0x0800FFFF, 16 Kbytes
// | 64 Kbytes
#define STM32F4xx_SECTOR_4_ADDR                                                                    \
        0x08010000 // Base address of Sector 4,  End address at 0x0801FFFF, 64 Kbytes
// | 128 Kbytes
#define STM32F4xx_SECTOR_5_ADDR                                                                    \
        0x08020000 // Base address of Sector 5,  End address at 0x0803FFFF, 128 Kbytes
// | 256 Kbytes
#define STM32F4xx_SECTOR_6_ADDR                                                                    \
        0x08040000 // Base address of Sector 6,  End address at 0x0805FFFF, 128 Kbytes
// | 384 Kbytes
#define STM32F4xx_SECTOR_7_ADDR                                                                    \
        0x08060000 // Base address of Sector 7,  End address at 0x0807FFFF, 128 Kbytes
// | 512 Kbytes

// STM32F4xx Sectors size list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_SIZE 0x4000
#define STM32F4xx_SECTOR_1_SIZE 0x4000
#define STM32F4xx_SECTOR_2_SIZE 0x4000
#define STM32F4xx_SECTOR_3_SIZE 0x4000
#define STM32F4xx_SECTOR_4_SIZE 0x10000
#define STM32F4xx_SECTOR_5_SIZE 0x20000
#define STM32F4xx_SECTOR_6_SIZE 0x20000
#define STM32F4xx_SECTOR_7_SIZE 0x20000

#elif defined(STM32F405xx) || defined(STM32F415xx) || defined(STM32F407xx) ||                      \
   defined(STM32F417xx) || defined(STM32F412Zx) || defined(STM32F412Vx) ||                        \
   defined(STM32F412Rx) || defined(STM32F412Cx)

// STM32F4xx sectors number
#define STM32F4xx_SECTORS_NUMBER 12

// STM32F4xx Sector start address list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_ADDR                                                                    \
        0x08000000 // Base address of Sector 0,  End address at 0x08003FFF, 16 Kbytes
// | 16 Kbytes
#define STM32F4xx_SECTOR_1_ADDR                                                                    \
        0x08004000 // Base address of Sector 1,  End address at 0x08007FFF, 16 Kbytes
// | 32 Kbytes
#define STM32F4xx_SECTOR_2_ADDR                                                                    \
        0x08008000 // Base address of Sector 2,  End address at 0x0800BFFF, 16 Kbytes
// | 48 Kbytes
#define STM32F4xx_SECTOR_3_ADDR                                                                    \
        0x0800C000 // Base address of Sector 3,  End address at 0x0800FFFF, 16 Kbytes
// | 64 Kbytes
#define STM32F4xx_SECTOR_4_ADDR                                                                    \
        0x08010000 // Base address of Sector 4,  End address at 0x0801FFFF, 64 Kbytes
// | 128 Kbytes
#define STM32F4xx_SECTOR_5_ADDR                                                                    \
        0x08020000 // Base address of Sector 5,  End address at 0x0803FFFF, 128 Kbytes
// | 256 Kbytes
#define STM32F4xx_SECTOR_6_ADDR                                                                    \
        0x08040000 // Base address of Sector 6,  End address at 0x0805FFFF, 128 Kbytes
// | 384 Kbytes
#define STM32F4xx_SECTOR_7_ADDR                                                                    \
        0x08060000 // Base address of Sector 7,  End address at 0x0807FFFF, 128 Kbytes
// | 512 Kbytes
#define STM32F4xx_SECTOR_8_ADDR                                                                    \
        0x08080000 // Base address of Sector 8,  End address at 0x0809FFFF, 128 Kbytes
// | 640 Kbytes
#define STM32F4xx_SECTOR_9_ADDR                                                                    \
        0x080A0000 // Base address of Sector 9,  End address at 0x080BFFFF, 128 Kbytes
// | 768 Kbytes
#define STM32F4xx_SECTOR_10_ADDR                                                                   \
        0x080C0000 // Base address of Sector 10, End address at 0x080EFFFF, 128 Kbytes
// | 986 Kbytes
#define STM32F4xx_SECTOR_11_ADDR                                                                   \
        0x080E0000 // Base address of Sector 11, End address at 0x080FFFFF, 128 Kbytes
// | 1024 Kbytes

// STM32F4xx Sectors size list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_SIZE 0x4000
#define STM32F4xx_SECTOR_1_SIZE 0x4000
#define STM32F4xx_SECTOR_2_SIZE 0x4000
#define STM32F4xx_SECTOR_3_SIZE 0x4000
#define STM32F4xx_SECTOR_4_SIZE 0x10000
#define STM32F4xx_SECTOR_5_SIZE 0x20000
#define STM32F4xx_SECTOR_6_SIZE 0x20000
#define STM32F4xx_SECTOR_7_SIZE 0x20000
#define STM32F4xx_SECTOR_8_SIZE 0x20000
#define STM32F4xx_SECTOR_9_SIZE 0x20000
#define STM32F4xx_SECTOR_10_SIZE 0x20000
#define STM32F4xx_SECTOR_11_SIZE 0x20000

#elif defined(STM32F413xx) || defined(STM32F423xx)

// STM32F4xx sectors number
#define STM32F4xx_SECTORS_NUMBER 16

// STM32F4xx Sector start address list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_ADDR                                                                    \
        0x08000000 // Base address of Sector 0,  End address at 0x08003FFF, 16 Kbytes
// | 16 Kbytes
#define STM32F4xx_SECTOR_1_ADDR                                                                    \
        0x08004000 // Base address of Sector 1,  End address at 0x08007FFF, 16 Kbytes
// | 32 Kbytes
#define STM32F4xx_SECTOR_2_ADDR                                                                    \
        0x08008000 // Base address of Sector 2,  End address at 0x0800BFFF, 16 Kbytes
// | 48 Kbytes
#define STM32F4xx_SECTOR_3_ADDR                                                                    \
        0x0800C000 // Base address of Sector 3,  End address at 0x0800FFFF, 16 Kbytes
// | 64 Kbytes
#define STM32F4xx_SECTOR_4_ADDR                                                                    \
        0x08010000 // Base address of Sector 4,  End address at 0x0801FFFF, 64 Kbytes
// | 128 Kbytes
#define STM32F4xx_SECTOR_5_ADDR                                                                    \
        0x08020000 // Base address of Sector 5,  End address at 0x0803FFFF, 128 Kbytes
// | 256 Kbytes
#define STM32F4xx_SECTOR_6_ADDR                                                                    \
        0x08040000 // Base address of Sector 6,  End address at 0x0805FFFF, 128 Kbytes
// | 384 Kbytes
#define STM32F4xx_SECTOR_7_ADDR                                                                    \
        0x08060000 // Base address of Sector 7,  End address at 0x0807FFFF, 128 Kbytes
// | 512 Kbytes
#define STM32F4xx_SECTOR_8_ADDR                                                                    \
        0x08080000 // Base address of Sector 8,  End address at 0x0809FFFF, 128 Kbytes
// | 640 Kbytes
#define STM32F4xx_SECTOR_9_ADDR                                                                    \
        0x080A0000 // Base address of Sector 9,  End address at 0x080BFFFF, 128 Kbytes
// | 768 Kbytes
#define STM32F4xx_SECTOR_10_ADDR                                                                   \
        0x080C0000 // Base address of Sector 10, End address at 0x080EFFFF, 128 Kbytes
// | 986 Kbytes
#define STM32F4xx_SECTOR_11_ADDR                                                                   \
        0x080E0000 // Base address of Sector 11, End address at 0x080FFFFF, 128 Kbytes
// | 1024 Kbytes
#define STM32F4xx_SECTOR_12_ADDR                                                                   \
        0x08100000 // Base address of Sector 11, End address at 0x081FFFFF, 128 Kbytes
// | 1152 Kbytes
#define STM32F4xx_SECTOR_13_ADDR                                                                   \
        0x08120000 // Base address of Sector 11, End address at 0x083FFFFF, 128 Kbytes
// | 1280 Kbytes
#define STM32F4xx_SECTOR_14_ADDR                                                                   \
        0x08140000 // Base address of Sector 11, End address at 0x085FFFFF, 128 Kbytes
// | 1408 Kbytes
#define STM32F4xx_SECTOR_15_ADDR                                                                   \
        0x08160000 // Base address of Sector 11, End address at 0x087FFFFF, 128 Kbytes
// | 1536 Kbytes

// STM32F4xx Sectors size list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_SIZE 0x4000
#define STM32F4xx_SECTOR_1_SIZE 0x4000
#define STM32F4xx_SECTOR_2_SIZE 0x4000
#define STM32F4xx_SECTOR_3_SIZE 0x4000
#define STM32F4xx_SECTOR_4_SIZE 0x10000
#define STM32F4xx_SECTOR_5_SIZE 0x20000
#define STM32F4xx_SECTOR_6_SIZE 0x20000
#define STM32F4xx_SECTOR_7_SIZE 0x20000
#define STM32F4xx_SECTOR_8_SIZE 0x20000
#define STM32F4xx_SECTOR_9_SIZE 0x20000
#define STM32F4xx_SECTOR_10_SIZE 0x20000
#define STM32F4xx_SECTOR_11_SIZE 0x20000
#define STM32F4xx_SECTOR_12_SIZE 0x20000
#define STM32F4xx_SECTOR_13_SIZE 0x20000
#define STM32F4xx_SECTOR_14_SIZE 0x20000
#define STM32F4xx_SECTOR_15_SIZE 0x20000

#elif defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) ||                      \
   defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx)

// 1MB Flash
#if (STM32F4xx_SIZE == 0x100000)

// Single bank organization
#if !defined(FLASH_DB_MODE)

// STM32F4xx sectors number
#define STM32F4xx_SECTORS_NUMBER 12

// STM32F4xx Sector start address list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_ADDR                                                                    \
        0x08000000 // Base address of Sector 0,  End address at 0x08003FFF, 16 Kbytes
// | 16 Kbytes
#define STM32F4xx_SECTOR_1_ADDR                                                                    \
        0x08004000 // Base address of Sector 1,  End address at 0x08007FFF, 16 Kbytes
// | 32 Kbytes
#define STM32F4xx_SECTOR_2_ADDR                                                                    \
        0x08008000 // Base address of Sector 2,  End address at 0x0800BFFF, 16 Kbytes
// | 48 Kbytes
#define STM32F4xx_SECTOR_3_ADDR                                                                    \
        0x0800C000 // Base address of Sector 3,  End address at 0x0800FFFF, 16 Kbytes
// | 64 Kbytes
#define STM32F4xx_SECTOR_4_ADDR                                                                    \
        0x08010000 // Base address of Sector 4,  End address at 0x0801FFFF, 64 Kbytes
// | 128 Kbytes
#define STM32F4xx_SECTOR_5_ADDR                                                                    \
        0x08020000 // Base address of Sector 5,  End address at 0x0803FFFF, 128 Kbytes
// | 256 Kbytes
#define STM32F4xx_SECTOR_6_ADDR                                                                    \
        0x08040000 // Base address of Sector 6,  End address at 0x0805FFFF, 128 Kbytes
// | 384 Kbytes
#define STM32F4xx_SECTOR_7_ADDR                                                                    \
        0x08060000 // Base address of Sector 7,  End address at 0x0807FFFF, 128 Kbytes
// | 512 Kbytes
#define STM32F4xx_SECTOR_8_ADDR                                                                    \
        0x08080000 // Base address of Sector 8,  End address at 0x0809FFFF, 128 Kbytes
// | 640 Kbytes
#define STM32F4xx_SECTOR_9_ADDR                                                                    \
        0x080A0000 // Base address of Sector 9,  End address at 0x080BFFFF, 128 Kbytes
// | 768 Kbytes
#define STM32F4xx_SECTOR_10_ADDR                                                                   \
        0x080C0000 // Base address of Sector 10, End address at 0x080EFFFF, 128 Kbytes
// | 986 Kbytes
#define STM32F4xx_SECTOR_11_ADDR                                                                   \
        0x080E0000 // Base address of Sector 11, End address at 0x080FFFFF, 128 Kbytes
// | 1024 Kbytes

// STM32F4xx Sectors size list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_SIZE 0x4000
#define STM32F4xx_SECTOR_1_SIZE 0x4000
#define STM32F4xx_SECTOR_2_SIZE 0x4000
#define STM32F4xx_SECTOR_3_SIZE 0x4000
#define STM32F4xx_SECTOR_4_SIZE 0x10000
#define STM32F4xx_SECTOR_5_SIZE 0x20000
#define STM32F4xx_SECTOR_6_SIZE 0x20000
#define STM32F4xx_SECTOR_7_SIZE 0x20000
#define STM32F4xx_SECTOR_8_SIZE 0x20000
#define STM32F4xx_SECTOR_9_SIZE 0x20000
#define STM32F4xx_SECTOR_10_SIZE 0x20000
#define STM32F4xx_SECTOR_11_SIZE 0x20000

// Dual bank organization
#else

// STM32F4xx sectors number
#define STM32F4xx_SECTORS_NUMBER 16

// STM32F4xx Sector start address list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_ADDR                                                                    \
        0x08000000 // Base address of Sector 0,  End address at 0x08003FFF, 16 Kbytes
// | 16 Kbytes
#define STM32F4xx_SECTOR_1_ADDR                                                                    \
        0x08004000 // Base address of Sector 1,  End address at 0x08007FFF, 16 Kbytes
// | 32 Kbytes
#define STM32F4xx_SECTOR_2_ADDR                                                                    \
        0x08008000 // Base address of Sector 2,  End address at 0x0800BFFF, 16 Kbytes
// | 48 Kbytes
#define STM32F4xx_SECTOR_3_ADDR                                                                    \
        0x0800C000 // Base address of Sector 3,  End address at 0x0800FFFF, 16 Kbytes
// | 64 Kbytes
#define STM32F4xx_SECTOR_4_ADDR                                                                    \
        0x08010000 // Base address of Sector 4,  End address at 0x0801FFFF, 64 Kbytes
// | 128 Kbytes
#define STM32F4xx_SECTOR_5_ADDR                                                                    \
        0x08020000 // Base address of Sector 5,  End address at 0x0803FFFF, 128 Kbytes
// | 256 Kbytes
#define STM32F4xx_SECTOR_6_ADDR                                                                    \
        0x08040000 // Base address of Sector 6,  End address at 0x0805FFFF, 128 Kbytes
// | 384 Kbytes
#define STM32F4xx_SECTOR_7_ADDR                                                                    \
        0x08060000 // Base address of Sector 7,  End address at 0x0807FFFF, 128 Kbytes
// | 512 Kbytes
#define STM32F4xx_SECTOR_12_ADDR                                                                   \
        0x08080000 // Base address of Sector 12, End address at 0x08083FFF, 16 Kbytes
// | 528 Kbytes
#define STM32F4xx_SECTOR_13_ADDR                                                                   \
        0x08084000 // Base address of Sector 13, End address at 0x08087FFF, 16 Kbytes
// | 544 Kbytes
#define STM32F4xx_SECTOR_14_ADDR                                                                   \
        0x08088000 // Base address of Sector 14, End address at 0x0808BFFF, 16 Kbytes
// | 560 Kbytes
#define STM32F4xx_SECTOR_15_ADDR                                                                   \
        0x0808C000 // Base address of Sector 15, End address at 0x0808FFFF, 16 Kbytes
// | 576 Kbytes
#define STM32F4xx_SECTOR_16_ADDR                                                                   \
        0x08090000 // Base address of Sector 16, End address at 0x0809FFFF, 64 Kbytes
// | 640 Kbytes
#define STM32F4xx_SECTOR_17_ADDR                                                                   \
        0x080A0000 // Base address of Sector 17, End address at 0x080BFFFF, 128 Kbytes
// | 768 Kbytes
#define STM32F4xx_SECTOR_18_ADDR                                                                   \
        0x080C0000 // Base address of Sector 18, End address at 0x080DFFFF, 128 Kbytes
// | 896 Kbytes
#define STM32F4xx_SECTOR_19_ADDR                                                                   \
        0x080E0000 // Base address of Sector 19, End address at 0x080FFFFF, 128 Kbytes
// | 1024 Kbytes

// STM32F4xx Sectors size list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_SIZE 0x4000
#define STM32F4xx_SECTOR_1_SIZE 0x4000
#define STM32F4xx_SECTOR_2_SIZE 0x4000
#define STM32F4xx_SECTOR_3_SIZE 0x4000
#define STM32F4xx_SECTOR_4_SIZE 0x10000
#define STM32F4xx_SECTOR_5_SIZE 0x20000
#define STM32F4xx_SECTOR_6_SIZE 0x20000
#define STM32F4xx_SECTOR_7_SIZE 0x20000
#define STM32F4xx_SECTOR_12_SIZE 0x4000
#define STM32F4xx_SECTOR_13_SIZE 0x4000
#define STM32F4xx_SECTOR_14_SIZE 0x4000
#define STM32F4xx_SECTOR_15_SIZE 0x4000
#define STM32F4xx_SECTOR_16_SIZE 0x10000
#define STM32F4xx_SECTOR_17_SIZE 0x20000
#define STM32F4xx_SECTOR_18_SIZE 0x20000
#define STM32F4xx_SECTOR_19_SIZE 0x20000

// STM32F4xx Banks ID list
#define STM32F4xx_BANK_1_ID 0
#define STM32F4xx_BANK_2_ID 1

// STM32F4xx Bank 1 Size
#define STM32F4xx_BANK_1_SIZE 0x80000
// STM32F4xx Bank 2 Size
#define STM32F4xx_BANK_2_SIZE 0x80000
// STM32F4xx Bank 1 start address
#define STM32F4xx_BANK_1_ADDR STM32F4xx_SECTOR_0_ADDR
// STM32F4xx Bank 2 start address
#define STM32F4xx_BANK_2_ADDR STM32F4xx_SECTOR_12_ADDR

#endif

// 2MB Flash
#else

// STM32F4xx sectors number
#define STM32F4xx_SECTORS_NUMBER 24

// STM32F4xx Sector start address list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_ADDR                                                                    \
        0x08000000 // Base address of Sector 0,  End address at 0x08003FFF, 16 Kbytes
// | 16 Kbytes
#define STM32F4xx_SECTOR_1_ADDR                                                                    \
        0x08004000 // Base address of Sector 1,  End address at 0x08007FFF, 16 Kbytes
// | 32 Kbytes
#define STM32F4xx_SECTOR_2_ADDR                                                                    \
        0x08008000 // Base address of Sector 2,  End address at 0x0800BFFF, 16 Kbytes
// | 48 Kbytes
#define STM32F4xx_SECTOR_3_ADDR                                                                    \
        0x0800C000 // Base address of Sector 3,  End address at 0x0800FFFF, 16 Kbytes
// | 64 Kbytes
#define STM32F4xx_SECTOR_4_ADDR                                                                    \
        0x08010000 // Base address of Sector 4,  End address at 0x0801FFFF, 64 Kbytes
// | 128 Kbytes
#define STM32F4xx_SECTOR_5_ADDR                                                                    \
        0x08020000 // Base address of Sector 5,  End address at 0x0803FFFF, 128 Kbytes
// | 256 Kbytes
#define STM32F4xx_SECTOR_6_ADDR                                                                    \
        0x08040000 // Base address of Sector 6,  End address at 0x0805FFFF, 128 Kbytes
// | 384 Kbytes
#define STM32F4xx_SECTOR_7_ADDR                                                                    \
        0x08060000 // Base address of Sector 7,  End address at 0x0807FFFF, 128 Kbytes
// | 512 Kbytes
#define STM32F4xx_SECTOR_8_ADDR                                                                    \
        0x08080000 // Base address of Sector 8,  End address at 0x0809FFFF, 128 Kbytes
// | 640 Kbytes
#define STM32F4xx_SECTOR_9_ADDR                                                                    \
        0x080A0000 // Base address of Sector 9,  End address at 0x080BFFFF, 128 Kbytes
// | 768 Kbytes
#define STM32F4xx_SECTOR_10_ADDR                                                                   \
        0x080C0000 // Base address of Sector 10, End address at 0x080EFFFF, 128 Kbytes
// | 986 Kbytes
#define STM32F4xx_SECTOR_11_ADDR                                                                   \
        0x080E0000 // Base address of Sector 11, End address at 0x080FFFFF, 128 Kbytes
// | 1024 Kbytes
#define STM32F4xx_SECTOR_12_ADDR                                                                   \
        0x08100000 // Base address of Sector 12, End address at 0x08103FFF, 16 Kbytes
// | 1040 Kbytes
#define STM32F4xx_SECTOR_13_ADDR                                                                   \
        0x08104000 // Base address of Sector 13, End address at 0x08107FFF, 16 Kbytes
// | 1056 Kbytes
#define STM32F4xx_SECTOR_14_ADDR                                                                   \
        0x08108000 // Base address of Sector 14, End address at 0x0810BFFF, 16 Kbytes
// | 1072 Kbytes
#define STM32F4xx_SECTOR_15_ADDR                                                                   \
        0x0810C000 // Base address of Sector 15, End address at 0x0810FFFF, 16 Kbytes
// | 1088 Kbytes
#define STM32F4xx_SECTOR_16_ADDR                                                                   \
        0x08110000 // Base address of Sector 16, End address at 0x0811FFFF, 64 Kbytes
// | 1152 Kbytes
#define STM32F4xx_SECTOR_17_ADDR                                                                   \
        0x08120000 // Base address of Sector 17, End address at 0x0813FFFF, 128 Kbytes
// | 1280 Kbytes
#define STM32F4xx_SECTOR_18_ADDR                                                                   \
        0x08140000 // Base address of Sector 18, End address at 0x0815FFFF, 128 Kbytes
// | 1408 Kbytes
#define STM32F4xx_SECTOR_19_ADDR                                                                   \
        0x08160000 // Base address of Sector 19, End address at 0x0817FFFF, 128 Kbytes
// | 1536 Kbytes
#define STM32F4xx_SECTOR_20_ADDR                                                                   \
        0x08180000 // Base address of Sector 20, End address at 0x0819FFFF, 128 Kbytes
// | 1664 Kbytes
#define STM32F4xx_SECTOR_21_ADDR                                                                   \
        0x081A0000 // Base address of Sector 21, End address at 0x081BFFFF, 128 Kbytes
// | 1792 Kbytes
#define STM32F4xx_SECTOR_22_ADDR                                                                   \
        0x081C0000 // Base address of Sector 22, End address at 0x081EFFFF, 128 Kbytes
// | 1920 Kbytes
#define STM32F4xx_SECTOR_23_ADDR                                                                   \
        0x081E0000 // Base address of Sector 23, End address at 0x081FFFFF, 128 Kbytes
// | 2048 Kbytes

// STM32F4xx Sectors size list (from STM32F4 datasheet)
#define STM32F4xx_SECTOR_0_SIZE 0x4000
#define STM32F4xx_SECTOR_1_SIZE 0x4000
#define STM32F4xx_SECTOR_2_SIZE 0x4000
#define STM32F4xx_SECTOR_3_SIZE 0x4000
#define STM32F4xx_SECTOR_4_SIZE 0x10000
#define STM32F4xx_SECTOR_5_SIZE 0x20000
#define STM32F4xx_SECTOR_6_SIZE 0x20000
#define STM32F4xx_SECTOR_7_SIZE 0x20000
#define STM32F4xx_SECTOR_8_SIZE 0x20000
#define STM32F4xx_SECTOR_9_SIZE 0x20000
#define STM32F4xx_SECTOR_10_SIZE 0x20000
#define STM32F4xx_SECTOR_11_SIZE 0x20000
#define STM32F4xx_SECTOR_12_SIZE 0x4000
#define STM32F4xx_SECTOR_13_SIZE 0x4000
#define STM32F4xx_SECTOR_14_SIZE 0x4000
#define STM32F4xx_SECTOR_15_SIZE 0x4000
#define STM32F4xx_SECTOR_16_SIZE 0x10000
#define STM32F4xx_SECTOR_17_SIZE 0x20000
#define STM32F4xx_SECTOR_18_SIZE 0x20000
#define STM32F4xx_SECTOR_19_SIZE 0x20000
#define STM32F4xx_SECTOR_20_SIZE 0x20000
#define STM32F4xx_SECTOR_21_SIZE 0x20000
#define STM32F4xx_SECTOR_22_SIZE 0x20000
#define STM32F4xx_SECTOR_23_SIZE 0x20000

// STM32F4xx Banks ID list
#define STM32F4xx_BANK_1_ID 0
#define STM32F4xx_BANK_2_ID 1

// STM32F4xx Bank 1 Size
#define STM32F4xx_BANK_1_SIZE 0x100000
// STM32F4xx Bank 2 Size
#define STM32F4xx_BANK_2_SIZE 0x100000
// STM32F4xx Bank 1 start address
#define STM32F4xx_BANK_1_ADDR STM32F4xx_SECTOR_0_ADDR
// STM32F4xx Bank 2 start address
#define STM32F4xx_BANK_2_ADDR STM32F4xx_SECTOR_12_ADDR

#endif

#else
#error                                                                                             \
   "Please select first the target STM32F4xx device used in your application (in stm32f4xx.h file)"
#endif

// STM32F4xx Internal Memory Flash driver
extern const FlashDriver stm32f4xxFlashDriver;

#endif //!_STM32F4xx_FLASH_DRIVER_H
