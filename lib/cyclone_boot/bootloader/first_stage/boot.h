/**
 * @file boot.h
 * @brief CycloneBOOT 1st Stage Bootloader managment
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

#ifndef _BOOT_H
#define _BOOT_H

#include "core/flash.h"
#include "core/cboot_error.h"
#include "debug.h"

typedef struct {
   const FlashDriver *driver;
   uintptr_t address;  //Address of second stage bl
   uint32_t offset;
   uint32_t *slots;
   size_t slot_count;
   uintptr_t update_address;  //Address of the slot containing bl update
} BootContext;

cboot_error_t bootFsm(BootContext *context);

cboot_error_t checkSecondStageBlIntegrity(BootContext *context);
bool_t checkForSecondStageBlUpdate(BootContext *context);
cboot_error_t updateSecondStageBl(BootContext *context);
void jumpToSecondStageBl(BootContext *context);

#if ((defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) || defined(__GNUC__) ||            \
   defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__) || defined(__TASKING__) ||                  \
   defined(__CWCC__) || defined(__TI_ARM__))
void bootHandleError(void);
#elif ((defined(_MSC_VER) || defined(_WIN32) || defined(WIN32) || defined(WIN64) ||                \
   defined(__unix__)))
extern void bootHandleError(void);
#endif

#endif
