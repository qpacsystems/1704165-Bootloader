/**
 * @file boot_common.h
 * @brief CycloneBOOT Bootloader common functions
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

#ifndef _BOOT_COMMON_H
#define _BOOT_COMMON_H

// Dependencies
#include "core/cboot_error.h"
#include "image/image.h"
#include "second_stage/boot.h"

// Initialization vector size of encrypted images in external flash memory
#define INIT_VECT_SIZE 16

// C++ guard
#ifdef __cplusplus
extern "C"
{
#endif

// CycloneBOOT Bootloader common related functions
void bootChangeState(BootContext *context, BootState newState);
cboot_error_t bootInitPrimaryMem(BootContext *context, BootSettings *settings);
cboot_error_t bootInitSecondaryMem(BootContext *context, BootSettings *settings);
cboot_error_t bootSelectUpdateImageSlot(BootContext *context, Slot *selectedSlot);
cboot_error_t bootUpdateApp(BootContext *context, Slot *slot);
cboot_error_t bootCheckImage(BootContext *context, Slot *slot);
cboot_error_t bootGetSlotImgHeader(Slot *slot, ImageHeader *header);
cboot_error_t bootCheckSlotAppResetVector(Slot *slot);

// C++ guard
#ifdef __cplusplus
}
#endif
#endif //_BOOT_COMMON_H
