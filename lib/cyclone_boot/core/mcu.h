/**
 * @file mcu.h
 * @brief CycloneBOOT MCU layer
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

#ifndef _MCU_H
#define _MCU_H

// Dependencies
#include "boot_config.h"
#include "os_port.h"

#include <stdint.h>

#ifndef BOOT_CUSTOM_DEINIT_SUPPORT
#define BOOT_CUSTOM_DEINIT_SUPPORT DISABLED
#elif ((BOOT_CUSTOM_DEINIT_SUPPORT != ENABLED) && (BOOT_CUSTOM_DEINIT_SUPPORT != DISABLED))
#error BOOT_CUSTOM_DEINIT_SUPPORT parameter is not valid!
#endif

// CycloneBOOT MCU Driver Major version
#define MCU_DRIVER_VERSION_MAJOR 0x01
// CycloneBOOT MCU Driver Minor version
#define MCU_DRIVER_VERSION_MINOR 0x00
// CycloneBOOT MCU Driver Revision version
#define MCU_DRIVER_VERSION_PATCH 0x00
// CycloneBOOT MCU Driver version
#define MCU_DRIVER_VERSION                                                                         \
        (uint32_t)(((MCU_DRIVER_VERSION_MAJOR & 0xFF) << 16) |                                         \
        ((MCU_DRIVER_VERSION_MINOR & 0xFF) << 8) | (MCU_DRIVER_VERSION_PATCH & 0xFF))

// C++ guard
#ifdef __cplusplus
extern "C"
{
#endif

// CycloneBOOT mcu layer related functions
extern uint32_t mcuGetVtorOffset(void);
extern void mcuSystemReset(void);

#if defined(__CC_ARM)
void mcuJumpToApplication(uint32_t address) __attribute__((section(".code_in_ram")));
#elif (defined(__GNUC__) && defined(__MINGW32__)) || (defined(__GNUC__) && defined(__MINGW64__))
void mcuJumpToApplication(uint32_t address) __attribute__((section(".code_in_ram")));
#elif defined(__GNUC__)
void mcuJumpToApplication(uint32_t address) __attribute__((section(".code_in_ram")));
#elif defined(_MSC_VER)
extern void mcuJumpToApplication(uint32_t address);
#endif

// C++ guard
#ifdef __cplusplus
}
#endif
#endif //!_MCU_H
