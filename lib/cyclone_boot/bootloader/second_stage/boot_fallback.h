/**
 * @file boot_fallback.h
 * @brief CycloneBOOT Bootloader fallback managment
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

#ifndef _BOOT_FALLBACK_H
#define _BOOT_FALLBACK_H

// Dependencies
#include "core/cboot_error.h"
#include "second_stage/boot.h"

// C++ guard
#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
   TRIGGER_STATUS_IDLE,
   TRIGGER_STATUS_RAISED
} TriggerStatus;

// CycloneBOOT Bootloader Fallback related functions
cboot_error_t fallbackTask(BootContext *context, Memory *memories);

#if ((defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) || defined(__GNUC__) ||            \
   defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__) || defined(__TASKING__) ||                  \
   defined(__CWCC__) || defined(__TI_ARM__))
cboot_error_t fallbackTriggerInit(void);
cboot_error_t fallbackTriggerGetStatus(TriggerStatus *status);
#elif ((defined(_MSC_VER) || defined(_WIN32) || defined(WIN32) || defined(WIN64) ||                \
   defined(__unix__)))
extern cboot_error_t fallbackTriggerInit(void);
extern cboot_error_t fallbackTriggerGetStatus(TriggerStatus *status);
#endif

// C++ guard
#ifdef __cplusplus
}
#endif
#endif //_BOOT_FALLBACK_H
