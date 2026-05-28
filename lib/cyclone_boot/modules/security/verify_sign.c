/**
 * @file verify_sign.c
 * @brief CycloneBOOT Image data signature module
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
#include "security/verify_sign.h"

#if (VERIFY_SIGNATURE_SUPPORT == ENABLED)

// Private function forward declaration
#if (VERIFY_RSA_SUPPORT == ENABLED)
cboot_error_t signVerifyRsa(VerifyContext *context, uint8_t *verifyData, size_t verifyDataLength);
#endif
#if (VERIFY_ECDSA_SUPPORT == ENABLED)
cboot_error_t signVerifyEcdsa(VerifyContext *context, uint8_t *verifyData, size_t verifyDataLength);
#endif

/**
 * @brief Initialize signature material for further signature hash computation
 * and signature verification.
 * @param[in,out] context Pointer to the IAP context
 * @return Error code
 **/

cboot_error_t signInit(VerifyContext *context)
{
   error_t error;
   VerifySettings *settings;

#if (VERIFY_RSA_SUPPORT == ENABLED)
   RsaPublicKey publicKey;
#endif

   // Check parameter validity
   if(context == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Point to the verify settings
   settings = (VerifySettings *)&context->verifySettings;

   // Initialize status code
   error = NO_ERROR;

   // Check user settings
   if(settings->signAlgo == VERIFY_SIGN_NONE || settings->signHashAlgo == NULL ||
      settings->signKey == NULL || settings->signKeyLen == 0)
      return CBOOT_ERROR_INVALID_VALUE;

   // Is signature RSA algorithm?
   if(settings->signAlgo == VERIFY_SIGN_RSA)
   {
#if (VERIFY_RSA_SUPPORT == ENABLED)
      // Initialize signature algo context
      settings->signHashAlgo->init(context->checkContext);

      // Set digest length
      context->imageCheckDigestSize = settings->signHashAlgo->digestSize;

      // Initialize RSA public key
      rsaInitPublicKey(&publicKey);

      // Decode pem key file into RSA public key
      error = pemImportRsaPublicKey(&publicKey, settings->signKey, settings->signKeyLen);

      // Check status code
      if(!error)
      {
         // Set check data (signature) size
         context->checkDataSize = publicKey.n.size * sizeof(publicKey.n.size);

         // Free RSA public key
         rsaFreePublicKey(&publicKey);
      }
#else
      return CBOOT_ERROR_INVALID_PARAMETERS;
#endif
   }
   // Is signature ECDSA algorithm?
   else if(settings->signAlgo == VERIFY_SIGN_ECDSA)
   {
#if (VERIFY_ECDSA_SUPPORT == ENABLED)
      // Initialize signature algo context
      settings->signHashAlgo->init(context->checkContext);

      // Set digest length
      context->imageCheckDigestSize = settings->signHashAlgo->digestSize;

      // Check status code
      if(!error)
      {
         // Set check data (signature) size
         context->checkDataSize = 64;    // TODO: ECDSA_SIGNATURE_SIZE
      }
#else
      return CBOOT_ERROR_INVALID_PARAMETERS;
#endif
   }
   else
   {
      // Debug message
      TRACE_ERROR("Signature algorithm not supported!\r\n");
      return CBOOT_ERROR_NOT_IMPLEMENTED;
   }

   // Return status code
   return CBOOT_NO_ERROR;
}

/**
 * @brief Verify received image firmware signature.
 * @param[in,out] context Pointer to the IAP context
 * @return Error code
 **/

cboot_error_t signVerify(VerifyContext *context, uint8_t *verifyData, size_t verifyDataLength)
{
   cboot_error_t cerror;
   VerifySettings *settings;

   // Check parameters validity
   if(context == NULL || verifyData == NULL || verifyDataLength == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Point to the verify settings
   settings = (VerifySettings *)&context->verifySettings;

   // Initialize status code
   cerror = CBOOT_NO_ERROR;

   // Check authentification hash algo
   if(settings->signAlgo == VERIFY_SIGN_NONE)
      return CBOOT_ERROR_INVALID_VALUE;
   // Is user require RSA signature?
   if(settings->signAlgo == VERIFY_SIGN_RSA)
   {
#if (VERIFY_RSA_SUPPORT == ENABLED)
      // Verify RSA signature
      cerror = signVerifyRsa(context, verifyData, verifyDataLength);
#else
      return CBOOT_ERROR_INVALID_PARAMETERS;
#endif
   }
   // Is user require ECDSA signature?
   else if(settings->signAlgo == VERIFY_SIGN_ECDSA)
   {
#if (VERIFY_ECDSA_SUPPORT == ENABLED)
      // Verify ECDSA signature
      cerror = signVerifyEcdsa(context, verifyData, verifyDataLength);
#else
      return CBOOT_ERROR_INVALID_PARAMETERS;
#endif
   }
   else
   {
      // Debug message
      TRACE_ERROR("Signature algorithm not supported!\r\n");
      cerror = CBOOT_ERROR_NOT_IMPLEMENTED;
   }

   // Return status code
   return cerror;
}

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

/**
 * @brief Verify RSA signature.
 * @param[in,out] context Pointer to the IAP context
 * @return Error code
 **/

#if (VERIFY_RSA_SUPPORT == ENABLED)
cboot_error_t signVerifyRsa(VerifyContext *context, uint8_t *verifyData, size_t verifyDataLength)
{
   error_t error;
   VerifySettings *settings;

   RsaPublicKey publicKey;

   // Check parameter validity
   if(context == NULL || verifyData == NULL || verifyDataLength == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Point to the verify settings
   settings = (VerifySettings *)&context->verifySettings;

   // Check signature user settings
   if(settings->signHashAlgo == NULL || settings->signKey == NULL || settings->signKeyLen == 0)
      return CBOOT_ERROR_INVALID_VALUE;

   // Initialize RSA public key
   rsaInitPublicKey(&publicKey);

   // Import PEM RSA public key
   error = pemImportRsaPublicKey(&publicKey, settings->signKey, settings->signKeyLen);
   // Is any error?
   if(error)
   {
      // Debug message
      TRACE_ERROR("RSA public key import failed!\r\n");
      return CBOOT_ERROR_FAILURE;
   }

   error = rsassaPkcs1v15Verify(&publicKey, settings->signHashAlgo, context->imageCheckDigest,
      verifyData, verifyDataLength);
   // Is any error?
   if(error)
   {
      // Debug message
      TRACE_ERROR("RSA signature verification failed!\r\n");
      return CBOOT_ERROR_FAILURE;
   }

   // Successful process
   return CBOOT_NO_ERROR;
}
#endif

/**
 * @brief Verify ECDSA signature.
 * @param[in,out] context Pointer to the IAP context
 * @return Error code
 **/

#if (VERIFY_ECDSA_SUPPORT == ENABLED)
cboot_error_t signVerifyEcdsa(VerifyContext *context, uint8_t *verifyData, size_t verifyDataLength)
{
   error_t error;
   VerifySettings *settings;

   EcPublicKey ecPublicKey;
   EcdsaSignature ecdsaSignature;

   // Intialize status code
   error = NO_ERROR;

   // Check parameter validity
   if(context == NULL || verifyData == NULL || verifyDataLength == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Point to the verify settings
   settings = (VerifySettings *)&context->verifySettings;

   // Check signature user settings
   if(settings->signHashAlgo == NULL || settings->signKey == NULL || settings->signKeyLen == 0)
      return CBOOT_ERROR_INVALID_VALUE;

   // Initialize EC public keys
   ecInitPublicKey(&ecPublicKey);
   // Initialize ECDSA signature
   ecdsaInitSignature(&ecdsaSignature);

   // Beginning of exceptions handling
   do
   {

      // Decode the PEM file that contains the EC public key
      error = pemImportEcPublicKey(&ecPublicKey, settings->signKey, settings->signKeyLen);

      // Is any error?
      if(error)
         break;

      error = ecdsaImportSignature(&ecdsaSignature, SECP256K1_CURVE, verifyData, verifyDataLength,
         ECDSA_SIGNATURE_FORMAT_RAW);

      if(error)
         break;

      // Verify EDCSA signature
      error = ecdsaVerifySignature(&ecPublicKey, context->imageCheckDigest,
         context->imageCheckDigestSize, &ecdsaSignature);

      // Is any error?
      if(error)
      {
         // Debug message
         TRACE_ERROR("ECDSA signature verification failed!\r\n");
         break;
      }
   } while(0);

   // Release previously allocated resources
   ecFreePublicKey(&ecPublicKey);
   ecdsaFreeSignature(&ecdsaSignature);

   // Return Status code
   if(error)
   {
      return CBOOT_ERROR_FAILURE;
   }
   else
   {
      return CBOOT_NO_ERROR;
   }
}
#endif

#endif
