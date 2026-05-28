/**
 * @file cipher.c
 * @brief CycloneBOOT Encryption & Decryption handling
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

// Switch to the appropriate trace level
#define TRACE_LEVEL CBOOT_TRACE_LEVEL

// Dependencies
#include "cipher.h"

#include "cipher_modes/cbc.h"
#include "debug.h"
#include "update/update.h"

#if (CIPHER_SUPPORT == ENABLED)

/**
 * @brief Initialize cipher engine context.
 * @param[in] engine Pointer to the cipher Engine context to initialize
 * @param[in] algo Cipher algorithm to be used for encryption/decryption.
 * @param[in] mode Cipher mode to be used with cipher algorithm
 * @param[in] key Pointer to the cipher key
 * @param[in] keyLen Length of the cipher key
 * @return Error code
 **/

// cboot_error_t cipherInit(CipherEngine *engine, const CipherAlgo *algo,
//    CipherMode mode, const char_t *key, size_t keyLen)
cboot_error_t cipherInit(CipherEngine *engine, const CipherAlgo *algo, CipherMode mode,
   const uint8_t *key, size_t keyLen)
{
   error_t error;

   // Check parameter validity
   if(engine == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Check cipher engine fields
   if(algo == NULL || mode == CIPHER_MODE_NULL || key == NULL || keyLen == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Reset cipher engine contents
   memset(engine, 0, sizeof(CipherEngine));

   // Set cipher engine algorithm
   engine->algo = algo;
   // Set cipher engine mode
   engine->mode = mode;
   // Set cipher engine key
   engine->key = key;
   // Set cipher engine key length
   engine->keyLen = keyLen;

   // Initialize cipher engine context
   error =
      engine->algo->init((void *)&engine->context, (const uint8_t *)engine->key, engine->keyLen);
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Set cipher iv length
   engine->ivLen = engine->algo->blockSize;

   // Return status code
   return CBOOT_NO_ERROR;
}

/**
 * @brief Set cipher initialization vector.
 * @param[in] engine Pointer to the cipher Engine context
 * @param[in] iv Initialization vector to use for encryption
 * @param[in] ivLen Length of the cipher initialization vector
 * @return Error code
 **/

cboot_error_t cipherSetIv(CipherEngine *engine, uint8_t *iv, size_t ivLen)
{
   // Check parameters
   if(engine == NULL || iv == NULL || ivLen == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Save cipher engine iv
   memcpy(engine->iv, iv, ivLen);

   // Successfull process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Encrypt given data using cipher engine.
 * @param[in] engine Pointer to the cipher Engine context
 * @param[in,out] data Buffer that initially contains plaintext data and then
 * will hold ciphertext data resulting from encryption
 * @param[in] length Length of the plaintext data buffer
 * @return Error code
 **/

cboot_error_t cipherEncryptData(CipherEngine *engine, uint8_t *data, size_t length)
{
   error_t error;

   // Check parameters validity
   if(engine == NULL || data == NULL || length == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Check cipher engine iv
   if(engine->ivLen == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Check cipher engine mode
   if(engine->mode == CIPHER_MODE_NULL)
   {
      return CBOOT_ERROR_INVALID_PARAMETERS;
   }
   else if(engine->mode == CIPHER_MODE_CBC)
   {
      // Encrypt plaintext data using CBC mode
      error = cbcEncrypt(engine->algo, (void *)&engine->context, engine->iv, data, data, length);
      // Is any error?
      if(error)
      {
         return CBOOT_ERROR_FAILURE;
      }
      else
      {
         return CBOOT_NO_ERROR;
      }
   }
   else
   {
      // Debug message
      TRACE_ERROR("Cipher mode not supported!\r\n");
      return CBOOT_ERROR_NOT_IMPLEMENTED;
   }
}

/**
 * @brief Decrypt given encrypted data using cipher engine.
 * @param[in] engine Pointer to the cipher Engine context
 * @param[in,out] data Buffer that initially contains encrypted data and then
 * will hold plaintext data resulting for decryption
 * @param[in] length Length of the plaintext data buffer
 * @return Error code
 **/

cboot_error_t cipherDecryptData(CipherEngine *engine, uint8_t *data, size_t length)
{
   error_t error;

   // Check parameters validity
   if(engine == NULL || data == NULL || length == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Check cipher engine iv
   if(engine->ivLen == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Check cipher engine mode
   if(engine->mode == CIPHER_MODE_NULL)
   {
      return CBOOT_ERROR_INVALID_PARAMETERS;
   }
   else if(engine->mode == CIPHER_MODE_CBC)
   {
      // Decrypt ciphertext data using CBC mode
      error = cbcDecrypt(engine->algo, (void *)&engine->context, engine->iv, data, data, length);
      // Is any error?
      if(error)
      {
         return CBOOT_ERROR_FAILURE;
      }
      else
      {
         return CBOOT_NO_ERROR;
      }
   }
   else
   {
      // Debug message
      TRACE_ERROR("Cipher mode not supported!\r\n");
      // Forward error;
      return CBOOT_ERROR_NOT_IMPLEMENTED;
   }
}

/**
 * @brief
 *
 * @param magicNumberCrc
 * @param keyIsValid
 * @return cboot_error_t
 **/

cboot_error_t cipherCheckMagicNumberCrc(uint32_t magicNumberCrc, bool_t *magicNumberIsValid)
{
   uint32_t computedMagicNumberCrc;

   // Check parameters
   if(magicNumberIsValid == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Set default magic number validity to FALSE
   *magicNumberIsValid = FALSE;

   // Compute CRC on the given magic number
   if(!CRC32_HASH_ALGO->compute(CIPHER_MAGIC_NUMBER, CIPHER_MAGIC_NUMBER_SIZE,
      (uint8_t *)&computedMagicNumberCrc))
   {
      // Check computed magic number crc against the given magic number crc
      if(memcmp((uint8_t *)&magicNumberCrc, (uint8_t *)&computedMagicNumberCrc,
         CRC32_DIGEST_SIZE) == 0)
      {
         // Magic number is valid
         *magicNumberIsValid = TRUE;
      }
   }

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief
 *
 * @param magicNumberCrc
 * @return cboot_error_t
 **/

cboot_error_t cipherComputeMagicNumberCrc(uint32_t *magicNumberCrc)
{
   error_t error;

   // Check parameters
   if(magicNumberCrc == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Compute magic number crc
   error = CRC32_HASH_ALGO->compute(CIPHER_MAGIC_NUMBER, CIPHER_MAGIC_NUMBER_SIZE,
      (uint8_t *)magicNumberCrc);
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Succesful process
   return CBOOT_NO_ERROR;
}

#endif
