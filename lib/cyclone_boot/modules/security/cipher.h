/**
 * @file cipher.h
 * @brief CycloneBOOT Encryption & Decryption handling
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

#ifndef _CIPHER_H
#define _CIPHER_H

// Dependencies
#include "cipher/cipher_algorithms.h"
#include "core/cboot_error.h"

#ifndef CIPHER_SUPPORT
#define CIPHER_SUPPORT DISABLED
#elif ((CIPHER_SUPPORT != ENABLED) && (CIPHER_SUPPORT != DISABLED))
#error CIPHER_SUPPORT parameter is not valid!
#endif

// Magic number used to check cipher key
#define CIPHER_MAGIC_NUMBER "5ef41578fcfbb9a98ffc218dde463d44"
#define CIPHER_MAGIC_NUMBER_SIZE 16

// Cipher initialization vector maximum size
#define MAX_CIPHER_IV_SIZE MAX_CIPHER_BLOCK_SIZE

// C++ guard
#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Cipher engine structure definition
 **/

typedef struct
{
   CipherContext context;
   const CipherAlgo *algo;
   CipherMode mode;
   // const char_t *key;
   const uint8_t *key;
   size_t keyLen;
   uint8_t iv[MAX_CIPHER_IV_SIZE];
   size_t ivLen;
} CipherEngine;

// CycloneBOOT cipher engine related functions
//  cboot_error_t cipherInit(CipherEngine *engine, const CipherAlgo *algo,
//     CipherMode mode, const char_t *key, size_t keyLen);
cboot_error_t cipherInit(CipherEngine *engine, const CipherAlgo *algo, CipherMode mode,
   const uint8_t *key, size_t keyLen);
cboot_error_t cipherSetIv(CipherEngine *engine, uint8_t *iv, size_t ivLen);
cboot_error_t cipherEncryptData(CipherEngine *cipherEngine, uint8_t *data, size_t length);
cboot_error_t cipherDecryptData(CipherEngine *cipherEngine, uint8_t *data, size_t length);

cboot_error_t cipherCheckMagicNumberCrc(uint32_t magicNumberCrc, bool_t *magicNumberIsValid);
cboot_error_t cipherComputeMagicNumberCrc(uint32_t *magicNumberCrc);

// C++ guard
#ifdef __cplusplus
}
#endif

#endif // !_CIPHER_H
