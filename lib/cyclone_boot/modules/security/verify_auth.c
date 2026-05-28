/**
 * @file verify_auth.c
 * @brief CycloneBOOT Image data authentication module
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

// Switch to the appropriate trace level
#define TRACE_LEVEL CBOOT_TRACE_LEVEL

// Dependencies
#include "security/verify_auth.h"

#if (VERIFY_AUTHENTICATION_SUPPORT == ENABLED)

/**
 * @brief Initialize authentification material for further authentification tag
 * computation on received firmware data and verification against
 * received image authentification tag.
 * @param[in] context Pointer to the verification context
 * @return Error code
 **/

cboot_error_t authInit(VerifyContext *context)
{
   error_t error;
   cboot_error_t cerror;
   VerifySettings *settings;

   // Check parameter validity
   if(context == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Point to the verify settings
   settings = (VerifySettings *)&context->verifySettings;

   // Initialize status code
   cerror = CBOOT_NO_ERROR;

   // Check user settings
   if(settings->authHashAlgo == NULL || settings->authAlgo == VERIFY_AUTH_NONE)
      return CBOOT_ERROR_INVALID_IMAGE_VERIFY_METHOD;

   // Is hmac authentification algo?
   if(settings->authAlgo == VERIFY_AUTH_HMAC)
   {
      // Initialize authentification hmac context
      error = hmacInit((HmacContext *)context->checkContext, settings->authHashAlgo,
         settings->authKey, settings->authKeyLen);
      // Is any error?
      if(error)
         cerror = CBOOT_ERROR_FAILURE;

      // Set digest length
      context->imageCheckDigestSize = ((HmacContext *)context->checkContext)->hash->digestSize;

      // Set check data (authentification tag) size
      context->checkDataSize = context->imageCheckDigestSize;
   }
   else
   {
      // Debug message
      TRACE_ERROR("Authentification algorithm not supported!\r\n");
      cerror = CBOOT_ERROR_NOT_IMPLEMENTED;
   }

   // Return status code
   return cerror;
}

/**
 * @brief Update authentification tag computation on received firmware data
 * bloc.
 * @param[in,out] context Pointer to the verification context
 * @param[in] data Firmware data bloc to process.
 * @param[in] length Length of the firmware data bloc to process.
 * @return Error code
 **/

cboot_error_t authUpdateTag(VerifyContext *context, const uint8_t *data, size_t length)
{
   cboot_error_t cerror;
   VerifySettings *settings;

   // Check parameter validity
   if(context == NULL || data == NULL || length == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Point to the verify settings
   settings = (VerifySettings *)&context->verifySettings;

   // Initialize status code
   cerror = CBOOT_NO_ERROR;

   // Check authentification hash algo
   if(settings->authAlgo == VERIFY_AUTH_NONE)
      return CBOOT_ERROR_INVALID_IMAGE_VERIFY_METHOD;

   // Is hmac authentification algo?
   if(settings->authAlgo == VERIFY_AUTH_HMAC)
   {
      // Update authentification hmac tag
      hmacUpdate((HmacContext *)context->checkContext, data, length);
   }
   else
   {
      // Debug message
      TRACE_ERROR("Authentification algorithm not supported!\r\n");
      cerror = CBOOT_ERROR_NOT_IMPLEMENTED;
   }

   // Return status code
   return cerror;
}

/**
 * @brief Finalize authentification tag computation on received firmware data.
 * @param[in,out] context Pointer to the verification context
 * @return Error code
 **/

cboot_error_t authFinalizeTag(VerifyContext *context)
{
   cboot_error_t cerror;
   VerifySettings *settings;

   // Check parameter validity
   if(context == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Point to the verify settings
   settings = (VerifySettings *)&context->verifySettings;

   // Initialize status code
   cerror = CBOOT_NO_ERROR;

   // Check authentification hash algo
   if(settings->authAlgo == VERIFY_AUTH_NONE)
      return CBOOT_ERROR_INVALID_IMAGE_VERIFY_METHOD;

   // Is hmac authentification algo?
   if(settings->authAlgo == VERIFY_AUTH_HMAC)
   {
      // Compute final authentification hmac tag
      hmacFinal((HmacContext *)context->checkContext, context->imageCheckDigest);
   }
   else
   {
      // Debug message
      TRACE_ERROR("Authentification algorithm not supported!\r\n");
      cerror = CBOOT_ERROR_NOT_IMPLEMENTED;
   }

   // Return status code
   return cerror;
}

#endif
