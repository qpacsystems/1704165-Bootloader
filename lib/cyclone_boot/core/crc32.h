/**
 * @file crc32.h
 * @brief CycloneBOOT CRC32 cryptographic integrity tag
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

#ifndef _CRC32_H
#define _CRC32_H

// Dependencies
#include "core/crypto.h"

// CRC32 block size
#define CRC32_BLOCK_SIZE 64
// CRC32 digest size
#define CRC32_DIGEST_SIZE 4
// Minimum length of the padding string
#define CRC32_MIN_PAD_SIZE 9
// CRC32 algorithm object identifier
#define CRC32_OID crc32Oid
// Common interface for hash algorithms
#define CRC32_HASH_ALGO (&crc32HashAlgo)

// C++ guard
#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief CRC32 algorithm context
 **/

typedef struct
{
   uint32_t digest;      ///< CRC32 digest
} Crc32Context;

// CRC32 related constants
extern const HashAlgo crc32HashAlgo;

// CycloneBOOT CRC32 related functions
error_t crc32Compute(const void *data, size_t length, uint8_t *digest);
void crc32Init(Crc32Context *context);
void crc32Update(Crc32Context *context, const void *data, size_t length);
void crc32Final(Crc32Context *context, uint8_t *digest);
void crc32FinalRaw(Crc32Context *context, uint8_t *digest);
void crc32ProcessBlock(Crc32Context *context);

// C++ guard
#ifdef __cplusplus
}
#endif

#endif
