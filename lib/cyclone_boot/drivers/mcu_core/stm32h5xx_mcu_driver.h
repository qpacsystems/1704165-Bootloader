/**
 * @file stm32h5xx_mcu_driver.h
 * @brief STM32H5xx family MCU driver
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

#ifndef _ARM_DRIVER_H
#define _ARM_DRIVER_H

// Dependencies
#include "core/mcu.h"
#include "stdint.h"

// You must align the offset to the number of exception entries in the vector
//  table. The minimum alignment is 32 words, enough for up to 16 interrupts.
//  For more interrupts, adjust the alignment by rounding up to the next power
//  of two

#define MCU_VTOR_OFFSET 0x400 // Could be less according to the cortex-m device but
// 0x400 will work for every arm cortex-m devices
// (16 execptions + 240 interrupts (max arm interrupt number) = 256 words = 1024
// bytes = 0x400 bytes)

// C++ guard
#ifdef __cplusplus
extern "C"
{
#endif

// STM32H7xx mcu driver related functions
uint32_t mcuGetVtorOffset(void);
void mcuSystemReset(void);
void mcuJumpToApplication(uint32_t address) __attribute__((section(".code_in_ram")));

// C++ guard
#ifdef __cplusplus
}
#endif
#endif //!_ARM_DRIVER_H
