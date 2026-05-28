/**
 * @file boot.h
 * @brief CycloneBOOT 2nd Stage Bootloader management
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

// Dependencies
#include "boot_config.h"
#include "cipher/aes.h"
#include "cipher_modes/cbc.h"
#include "core/cboot_error.h"
#include "core/crypto.h"
#include "core/flash.h"

// default offset
#ifndef BOOT_OFFSET
#define BOOT_OFFSET 0x10000
#endif

// Enable fallback support
#ifndef BOOT_FALLBACK_SUPPORT
#define BOOT_FALLBACK_SUPPORT DISABLED
#elif ((BOOT_FALLBACK_SUPPORT != ENABLED) && (BOOT_FALLBACK_SUPPORT != DISABLED))
#error BOOT_FALLBACK_SUPPORT parameter is not valid
#elif ((BOOT_FALLBACK_SUPPORT == DISABLED) &&                                                      \
   (BOOT_FALLBACK_AUTO_MODE == ENABLED || BOOT_FALLBACK_MANUAL_MODE == ENABLED))
#error BOOT_FALLBACK_SUPPORT is DISABLED while BOOT_FALLBACK_XXX_MODE is ENABLED
#endif

#if (BOOT_FALLBACK_AUTO_MODE == ENABLED)
#if (BOOT_FLAG_SUPPORT == DISABLED && BOOT_COUNTER_SUPPORT == DISABLED)
#error when BOOT_FALLBACK_AUTO_MODE is ENABLED, also ENABLE BOOT_FLAG_SUPPORT and/or BOOT_COUNTER_SUPPORT
#endif
#if (BOOT_COUNTER_SUPPORT == ENABLED && BOOT_COUNTER_MAX_ATTEMPTS <= 0)
#error invalid counter value (should be > 0)
#endif
#endif

#ifndef BOOT_FALLBACK_AUTO_MODE
#if (BOOT_COUNTER_SUPPORT == ENABLED || BOOT_FLAG_SUPPORT == ENABLED)
#error BOOT_FALLBACK_AUTO_MODE is DISABLED while BOOT_XXX_SUPPORT is ENABLED
#endif
#endif

#if (BOOT_FALLBACK_SUPPORT == ENABLED)
#if (BOOT_FALLBACK_AUTO_MODE == DISABLED && BOOT_FALLBACK_MANUAL_MODE == DISABLED)
#error BOOT_FALLBACK_SUPPORT is ENABLED BOOT_FALLBACK_XXX_MODE is DISABLED
#endif
#endif

// Enable anti-rollback support
#ifndef BOOT_ANTI_ROLLBACK_SUPPORT
#define BOOT_ANTI_ROLLBACK_SUPPORT DISABLED
#elif ((BOOT_ANTI_ROLLBACK_SUPPORT != ENABLED) && (BOOT_ANTI_ROLLBACK_SUPPORT != DISABLED))
#error BOOT_ANTI_ROLLBACK_SUPPORT parameter is not valid
#endif

// Enable external memory encryption support
#ifndef BOOT_EXT_MEM_ENCRYPTION_SUPPORT
#define BOOT_EXT_MEM_ENCRYPTION_SUPPORT DISABLED
#elif ((BOOT_EXT_MEM_ENCRYPTION_SUPPORT != ENABLED) &&                                             \
   (BOOT_EXT_MEM_ENCRYPTION_SUPPORT != DISABLED))
#error BOOT_EXT_MEM_ENCRYPTION_SUPPORT parameter is not valid
#endif

// C++ guard
#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Bootloader States definition
 **/

typedef enum
{
   BOOT_STATE_IDLE,
   BOOT_STATE_RUN_APP,
   BOOT_STATE_UPDATE_APP,
   BOOT_STATE_FALLBACK_APP,
   BOOT_STATE_ERROR
} BootState;

/**
 * @brief Bootloader user settings structure
 **/

typedef struct
{
#if (BOOT_EXT_MEM_ENCRYPTION_SUPPORT == ENABLED) /* \
                                                    && (BOOT_FALLBACK_SUPPORT == ENABLED))*/
   const char_t *psk;                            ///< Secondary flash slot cipher key
   size_t pskSize;                               ///< Secondary flash slot cipher key size
#endif

   Memory memories[NB_MEMORIES];
} BootSettings;

/**
 * @brief Bootloader Context structure
 **/
typedef struct
{
   BootState state;            ///< Bootloader state
   BootSettings settings;      ///< Bootloader user settings
   Memory memories[NB_MEMORIES];
#if (BOOT_EXT_MEM_ENCRYPTION_SUPPORT == ENABLED)
   uint8_t psk[32];      ///< Cipher PSK key for image decryption
   size_t pskSize;       ///< Cipher PSK key size
#endif
   Slot selectedSlot;
   bool_t busy;
} BootContext;

// Bootloader related functions
cboot_error_t bootFsm(BootContext *context);
void bootGetDefaultSettings(BootSettings *settings);
cboot_error_t bootInit(BootContext *context, BootSettings *settings);
cboot_error_t bootCreateBackupSlot(BootContext *context);
cboot_error_t bootUpdateApp(BootContext *context, Slot *slot);

#if ((defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) || defined(__GNUC__) ||            \
   defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__) || defined(__TASKING__) ||                  \
   defined(__CWCC__) || defined(__TI_ARM__))
void bootInitHook(void);
void bootIdleStateHook(void);
void bootNoValidUpdatesHook(void);
void bootJumpingToApplicationHook(void);
void bootFallbackPerformedHook(void);
void bootHandleFallbackError(void);
void bootHandleGenericError(void);
#elif ((defined(_MSC_VER) || defined(_WIN32) || defined(WIN32) || defined(WIN64) ||                \
   defined(__unix__)))
extern void bootInitHook(void);
extern void bootIdleStateHook(void);
extern void bootNoValidUpdatesHook(void);
extern void bootJumpingToApplicationHook(void);
extern void bootFallbackPerformedHook(void);
extern void bootHandleFallbackError(void);
extern void bootHandleGenericError(void);
#endif

// Extern device MCU related function
extern uint32_t mcuGetVtorOffset(void);
extern void mcuSystemReset(void);
extern void mcuJumpToApplication(uint32_t address);

// C++ guard
#ifdef __cplusplus
}
#endif
#endif //_BOOT_H
