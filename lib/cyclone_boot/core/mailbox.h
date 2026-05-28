/**
 * @file mailbox.h
 * @brief Boot Mailbox mangement functions
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

#ifndef _MAILBOX_H
#define _MAILBOX_H

// Dependencies
#include "boot_config.h"
#include "core/cboot_error.h"
#include "debug.h"
#include "os_port.h"

// Bootloader mailbox version
#define BOOT_MBX_VERS_MAJOR 1
#define BOOT_MBX_VERS_MINOR 0
#define BOOT_MBX_VERS_PATCH 0
#define BOOT_MBX_VERSION                                                                           \
        (uint32_t)(((BOOT_MBX_VERS_MAJOR & 0xFF) << 16) | ((BOOT_MBX_VERS_MINOR & 0xFF) << 8) |        \
        (BOOT_MBX_VERS_MAJOR & 0xFF))

// Bootloader mailbox signture
#define BOOT_MBX_SIGNATURE 0x1b241671

// Bootloader mailbox flags
enum
{
   UPDATE_REQUESTED = 1 << 0,
   UPDATE_PERFORMED = 1 << 1,
   UPDATE_CONFIRMED = 1 << 2,
   FALLBACK_REQUESTED = 1 << 3,
   FALLBACK_PERFORMED = 1 << 4
};

// Bootloader mailbox NVM flags
#define BOOT_MBX_NVM_MAGIC                      0xBEEF
#define BOOT_MBX_NVM_FLAG_UPDATE_REQUESTED      0xF00D
#define BOOT_MBX_NVM_FLAG_UPDATE_PERFORMED      0xC0DE
#define BOOT_MBX_NVM_FLAG_UPDATE_CONFIRMED      0xFEED
#define BOOT_MBX_NVM_FLAG_FALLBACK_REQUESTED    0xB105
#define BOOT_MBX_NVM_FLAG_FALLBACK_PERFORMED    0xDEAD

// Bootloader mailbox maximum PSK size
#define BOOT_MBX_PSK_MAX_SIZE 32

#if ((defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) || defined(__GNUC__) ||            \
   defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__) || defined(__TASKING__) ||                  \
   defined(__CWCC__) || defined(__TI_ARM__))

/**
 * @brief Bootloader shared RAM mailbox structure
 **/

typedef __packed_struct
{
   uint32_t version;                    ///< Bootloader Mailbox version
   uint32_t signature;                  ///< Bootloader Mailbox signature
   uint32_t pskSize;                    ///< Bootloader Mailbox PSK size
   uint8_t psk[BOOT_MBX_PSK_MAX_SIZE];  ///< Bootloader Mailbox PSK key
   uint32_t flags;                      ///< Bootloader Mailbox event flags
   uint32_t boot_counter;               ///< Bootloader Mailbox boot counter
   uint8_t reserved[80];                ///< Reserved
}
BootMailBox;

#else

#undef interface
#undef __start_packed
#define __start_packed __pragma(pack(push, 1))
#undef __end_packed
#define __end_packed __pragma(pack(pop))
#define __weak

/**
 * @brief Bootloader shared RAM mailbox structure
 **/

__start_packed typedef struct
{
   uint32_t version;                    ///< Bootloader Mailbox version
   uint32_t signature;                  ///< Bootloader Mailbox signature
   uint32_t pskSize;                    ///< Bootloader Mailbox PSK size
   uint8_t psk[BOOT_MBX_PSK_MAX_SIZE];  ///< Bootloader Mailbox PSK key
   uint32_t flags;                      ///< Bootloader Mailbox event flags
   uint32_t boot_counter;               ///< Bootloader Mailbox boot counter
   uint8_t reserved[80];                ///< Reserved
} BootMailBox __end_packed;

#endif

// C++ guard
#ifdef __cplusplus
extern "C"
{
#endif

// CycloneBOOT IAP Mailbox functions
void mailBoxInit(void);

bool_t mailBoxIsUpdateRequested(void);
void mailBoxSetUpdateRequested(bool_t state);

bool_t mailBoxIsUpdatePerformed(void);
void mailBoxSetUpdatePerformed(bool_t state);

bool_t mailBoxIsUpdateConfirmed(void);
void mailBoxSetUpdateConfirmed(bool_t state);

bool_t mailBoxIsFallbackRequested(void);
bool_t mailBoxIsFallbackPerformed(void);
void mailBoxSetFallbackRequested(bool_t state);
void mailBoxSetFallbackPerformed(bool_t state);

void mailBoxIncrementBootCounter(void);
uint32_t mailBoxGetBootCounter(void);
void mailBoxClearBootCounter(void);

void mailBoxSetPsk(uint8_t *psk, uint32_t pskSize);
void mailBoxGetPsk(uint8_t *psk, uint32_t *pskSize);
void mailBoxClearPsk(void);

// Define weakly defined function to place boot flags in NVM
#if ((defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) || defined(__GNUC__) ||            \
   defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__) || defined(__TASKING__) ||                  \
   defined(__CWCC__) || defined(__TI_ARM__))
cboot_error_t mbx_set_nvm_flag(uint32_t flag, bool_t value);
bool_t mbx_get_nvm_flag(uint32_t flag);
#elif ((defined(_MSC_VER) || defined(_WIN32) || defined(WIN32) || defined(WIN64) ||                \
   defined(__unix__)))
extern cboot_error_t mbx_set_nvm_flag(uint32_t flag, bool_t value);
extern bool_t mbx_get_nvm_flag(uint32_t flag);
#endif

// C++ guard
#ifdef __cplusplus
}
#endif
#endif //!_MAILBOX_H
