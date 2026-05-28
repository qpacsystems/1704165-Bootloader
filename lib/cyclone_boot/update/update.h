/**
 * @file update.h
 * @brief CycloneBOOT IAP User API
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

#ifndef _UPDATE_H
#define _UPDATE_H

// Dependencies
#include "boot_config.h"
#include "core/crc32.h"
#include "core/crypto.h"
#include "core/flash.h"
#include "hash/hash_algorithms.h"
#include "image/image.h"
#include "memory/memory.h"

#include <stdint.h>
#if ((CIPHER_SUPPORT == ENABLED) &&                                                                \
   ((IMAGE_INPUT_ENCRYPTED == ENABLED) || (IMAGE_OUTPUT_ENCRYPTED == ENABLED)))
#include "security/cipher.h"
#endif
#include "core/cboot_error.h"
#include "error.h"
#include "security/verify.h"

// CycloneBOOT Version string
#define CYCLONE_BOOT_UPDATE_VERSION_STRING "2.5.4"
// CycloneBOOT Major version
#define CYCLONE_BOOT_UPDATE_MAJOR_VERSION 2
// CycloneBOOT Minor version
#define CYCLONE_BOOT_UPDATE_MINOR_VERSION 5
// CycloneBOOT Revision number
#define CYCLONE_BOOT_UPDATE_REV_NUMBER 4

#define IMAGE_MAX_CHECK_DATA_SIZE 512

// Update single bank internal flash memory mode support
#ifndef UPDATE_SINGLE_BANK_SUPPORT
#define UPDATE_SINGLE_BANK_SUPPORT DISABLED
#elif (UPDATE_SINGLE_BANK_SUPPORT != ENABLED && UPDATE_SINGLE_BANK_SUPPORT != DISABLED)
#error UPDATE_SINGLE_BANK_SUPPORT parameter is not valid!
#endif

// Update dual bank internal flash memory mode support
#ifndef UPDATE_DUAL_BANK_SUPPORT
#define UPDATE_DUAL_BANK_SUPPORT ENABLED
#elif (UPDATE_DUAL_BANK_SUPPORT != ENABLED && UPDATE_DUAL_BANK_SUPPORT != DISABLED)
#error UPDATE_DUAL_BANK_SUPPORT parameter is not valid!
#endif

// Update fallback support
#ifndef UPDATE_FALLBACK_SUPPORT
#define UPDATE_FALLBACK_SUPPORT DISABLED
#elif (UPDATE_FALLBACK_SUPPORT != ENABLED && UPDATE_FALLBACK_SUPPORT != DISABLED)
#error UPDATE_FALLBACK_SUPPORT parameter is not valid!
#endif

// Update anti-rollback support
#ifndef UPDATE_ANTI_ROLLBACK_SUPPORT
#define UPDATE_ANTI_ROLLBACK_SUPPORT DISABLED
#elif (UPDATE_ANTI_ROLLBACK_SUPPORT != ENABLED && UPDATE_ANTI_ROLLBACK_SUPPORT != DISABLED)
#error UPDATE_ANTI_ROLLBACK_SUPPORT parameter is not valid!
#endif

// Acceptable internal memory mode
#if ((UPDATE_SINGLE_BANK_SUPPORT == ENABLED && UPDATE_DUAL_BANK_SUPPORT == ENABLED) ||             \
   (UPDATE_SINGLE_BANK_SUPPORT == DISABLED && UPDATE_DUAL_BANK_SUPPORT == DISABLED))
#error Exactly one of the following parameters MUST be enabled: \
   UPDATE_SINGLE_BANK_SUPPORT - \
   UPDATE_DUAL_BANK_SUPPORT
#endif

// Acceptable encryption of the output image activation
#if (((CIPHER_SUPPORT == ENABLED) && (IMAGE_OUTPUT_ENCRYPTED == ENABLED)) &&                       \
   (UPDATE_SINGLE_BANK_SUPPORT == DISABLED))
#error Encryption of the output image is available only in Singel Bank mode!
#endif

// Add update encryption related dependencies
#if ((CIPHER_SUPPORT == ENABLED) &&                                                                \
   ((IMAGE_INPUT_ENCRYPTED == ENABLED) || (IMAGE_OUTPUT_ENCRYPTED == ENABLED)))
#include "security/cipher.h"
#endif

#if (UPDATE_SINGLE_BANK_SUPPORT == ENABLED)
#if (VERIFY_INTEGRITY_SUPPORT == DISABLED)
// Force activation of VERIFY_INTEGRITY_SUPPORT when UPDATE_SINGLE_BANK_SUPPORT
// is activated.
#error Integrity support (VERIFY_INTEGRITY_SUPPORT) MUST be enabled when using CycloneBOOT in Single Bank Mode!
#endif
#endif

// C++ guard
#ifdef __cplusplus
extern "C"
{
#endif

// Forward declaration of UpdateContext structure
struct _UpdateContext;
#define UpdateContext struct _UpdateContext

/**
 * @brief Random data generation callback function
 **/

typedef cboot_error_t (*IapRandCallback)(uint8_t *data, size_t length);

/**
 *@brief Supported authentication algorithms
 **/

typedef enum
{
   UPDATE_AUTH_NONE,
   UPDATE_AUTH_HMAC
} UpdateAuthAlgo;

/**
 *@brief Supported signature algorithms
 **/

typedef enum
{
   UPDATE_SIGN_NONE,
   UPDATE_SIGN_RSA,
   UPDATE_SIGN_ECDSA
} UpdateSignAlgo;

/**
 * @brief Update Crypto settings
 **/

typedef struct
{
#if ((CIPHER_SUPPORT == ENABLED) &&                                                                \
   ((IMAGE_INPUT_ENCRYPTED == ENABLED) || (IMAGE_OUTPUT_ENCRYPTED == ENABLED)))
   const CipherAlgo *cipherAlgo;      ///< Image cipher algorithm
   CipherMode cipherMode;             ///< Image cipher mode
   // const char_t *cipherKey;                        ///<Image cipher key
   const uint8_t *cipherKey;      ///< Image cipher key
   size_t cipherKeyLen;           ///< Image cipher key size
#endif
   VerifySettings verifySettings;      ///< Various crypto settings for image verification
} UpdateCryptoSettings;

/**
 * @brief Update user settings
 **/

typedef struct
{
#if (UPDATE_ANTI_ROLLBACK_SUPPORT == ENABLED)
   ImageVersion appVersion;      ///< Version of the current running application
#endif
   UpdateCryptoSettings
      imageInCrypto;       ///< Cryptographic settings used to manage an update image
#if (UPDATE_SINGLE_BANK_SUPPORT == ENABLED)
   UpdateCryptoSettings imageOutCrypto;      ///< Cryptographic settings used to
                                             ///< generate the update image
#if (IMAGE_OUTPUT_ENCRYPTED == ENABLED)
   // const char_t *psk;                           ///<PSK key used to encrypt
   // the output image
   const uint8_t *psk;      ///< PSK key used to encrypt the output image
   uint32_t pskSize;        ///< Size of the PSK key used to encrypt the output image
#endif
#endif
   Memory memories[NB_MEMORIES];
} UpdateSettings;

/**
 * @brief Update states
 **/

typedef enum
{
   UPDATE_STATE_IDLE,
   UPDATE_STATE_RECV_APP_HEADER,
   UPDATE_STATE_RECV_APP_DATA,
   UPDATE_STATE_RECV_APP_CHECK,
   UPDATE_STATE_VALIDATE_APP,
   UPDATE_STATE_APP_REBOOT,
   UPDATE_STATE_WRITE_APP_INIT,
   UPDATE_STATE_WRITE_APP_HEADER,
   UPDATE_STATE_WRITE_APP_DATA,
   UPDATE_STATE_WRITE_APP_CHECK,
   UPDATE_STATE_WRITE_APP_END
} UpdateState;

/**
 * @brief Update context
 **/

struct _UpdateContext
{
   UpdateSettings settings;      ///< Update user settings
   Memory memories[NB_MEMORIES];
   ImageProcessContext imageProcessCtx;
};

// CycloneBOOT Update application related functions
void updateGetDefaultSettings(UpdateSettings *settings);
char_t *updateGetVersion(void);
cboot_error_t updateRegisterRandCallback(IapRandCallback callback);

cboot_error_t updateInit(UpdateContext *context, UpdateSettings *settings);
cboot_error_t updateProcess(UpdateContext *context, const void *data, size_t length);
cboot_error_t updateFinalize(UpdateContext *context);
cboot_error_t updateReboot(UpdateContext *context);
cboot_error_t updateConfirm(void);

// Extern device MCU related function
extern uint32_t mcuGetVtorOffset(void);
extern void mcuSystemReset(void);
extern void mcuJumpToApplication(uint32_t address);

// Random data generation callback function
extern IapRandCallback updateRandCallback;


#if ((defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) || defined(__GNUC__) ||            \
   defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__) || defined(__TASKING__) ||                  \
   defined(__CWCC__) || defined(__TI_ARM__))
void updateResetMcuHook(void);
#elif ((defined(_MSC_VER) || defined(_WIN32) || defined(WIN32) || defined(WIN64) ||                \
   defined(__unix__)))
void updateResetMcuHook(void);
#endif


// C++ guard
#ifdef __cplusplus
}
#endif

#endif // !_UPDATE_H
