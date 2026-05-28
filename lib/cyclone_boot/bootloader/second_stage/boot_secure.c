/**
 * @file boot_secure.c
 * @brief CycloneBOOT Bootloader Secure Boot related functions
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

/**
 * @brief Check signature of current running image.
 * @param[in] slot Pointer to the slot containing the image to be checked.
 * @return Error code.
 **/

#include "second_stage/boot_secure.h"

cboot_error_t bootCheckRuntimeImageSignature(BootContext *context, Slot *slot) {
   error_t error;
   cboot_error_t cerror;
   uint32_t addr;
   ImageHeader *header;
   FlashDriver *driver;
   const FlashInfo *info;
   Memory *memory;
   HashContext hashContext;
   size_t length;
   size_t n;
   uint8_t buffer[sizeof(ImageHeader)];
   uint8_t digest[MAX_HASH_DIGEST_SIZE] = {0};

#if (BOOT_RUNTIME_SIGNATURE_ECDSA == ENABLED)
   EcPublicKey ecPublicKey;
   EcCurve *curve;
   size_t signatureLength;
   EcdsaSignature ecSignature;
   const HashAlgo *signatureHashAlgo = SHA256_HASH_ALGO;
   uint8_t originalSignature[ECDSA_SIGNATURE_SIZE];
#endif

#if (BOOT_RUNTIME_SIGNATURE_RSA == ENABLED)
   RsaPublicKey rsaPublicKey;
   const HashAlgo *signatureHashAlgo = SHA256_HASH_ALGO;
   uint8_t originalSignature[RSA_SIGNATURE_SIZE];
#endif

   // Check parameter validity
   if(context == NULL || slot == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Point to slot memory descriptor
   memory = (Memory *) slot->memParent;

   // Get memory info
   driver = (FlashDriver *) memory->driver;
   error = driver->getInfo(&info);
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Read slot data
   error = driver->read(slot->addr, buffer, sizeof(buffer));
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Point to internal image header
   header = (ImageHeader *) buffer;

   // Check internal image header
   cerror = imageCheckHeader(header);
   // Is any error?
   if(cerror)
   {
      // Debug message
      TRACE_ERROR("Image header is not valid!\r\n");
      return cerror;
   }

   // Discard internal image header
   addr = slot->addr + sizeof(ImageHeader);

   // Save internal image data size
   length = header->dataSize;

#if BOOT_RUNTIME_SIGNATURE_ECDSA == ENABLED
   // Initialize public key & signature
   ecInitPublicKey(&ecPublicKey);
   ecdsaInitSignature(&ecSignature);
   // Read/decode public key
   error = pemImportEcPublicKey(&ecPublicKey, context->settings.verifySettings.signKey,
      context->settings.verifySettings.signKeyLen);
   if(error)
   {
      return CBOOT_ERROR_FAILURE;
   }
   // Read current signature
   addr += header->dataPadding;
   addr += header->binarySize;
   // Read given image binary crc
   error = driver->read(addr, originalSignature, ECDSA_SIGNATURE_SIZE);
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   //Import signature
   curve = ecPublicKey.curve;
   signatureLength = ((curve->orderSize + 7) / 8) * 2;
   error = ecdsaImportSignature(&ecSignature, curve, originalSignature, signatureLength, ECDSA_SIGNATURE_FORMAT_RAW);
   if(error)
   {
      return CBOOT_ERROR_FAILURE;
   }

   // Digest firmware contents
   // Initialize integrity algorithm
   signatureHashAlgo->init(&hashContext);

   // Process image binary data
   addr = slot->addr + sizeof(ImageHeader);
   addr += header->dataPadding;
   length = header->binarySize;

   while(length > 0)
   {
      // Prevent read operation to overflow buffer size
      n = MIN(sizeof(buffer), length);

      // Read image binary data
      error = driver->read(addr, buffer, n);
      // Is any error?
      if(error)
         return CBOOT_ERROR_FAILURE;

      // Update image binary data crc computation
      signatureHashAlgo->update(&hashContext, buffer, n);

      // Increment external flash memory word address
      addr += n;
      // Remaining bytes to be read
      length -= n;
   }

   // Finalize image binary data hash computation
   signatureHashAlgo->final(&hashContext, digest);

   // Perform signature verification
   error = ecdsaVerifySignature(&ecPublicKey, digest, signatureHashAlgo->digestSize, &ecSignature);
   if(error)
   {
      return CBOOT_ERROR_FAILURE;
   }

   // Release acquired resources
   ecFreePublicKey(&ecPublicKey);
   ecdsaFreeSignature(&ecSignature);

   return CBOOT_NO_ERROR;

#endif

#if BOOT_RUNTIME_SIGNATURE_RSA == ENABLED
   //Initialize public key
   rsaInitPublicKey(&rsaPublicKey);

   //Read/decode public key
   error = pemImportRsaPublicKey(&rsaPublicKey, context->settings.verifySettings.signKey,
      context->settings.verifySettings.signKeyLen);
   if(error)
   {
      return CBOOT_ERROR_FAILURE;
   }
   // Read current signature
   addr += header->dataPadding;
   addr += header->binarySize;
   // Read given image binary crc
   error = driver->read(addr, originalSignature, RSA_SIGNATURE_SIZE);
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;
   // Digest firmware contents
   // Initialize integrity algorithm
   signatureHashAlgo->init(&hashContext);

   // Process image binary data
   addr = slot->addr + sizeof(ImageHeader);
   addr += header->dataPadding;
   length = header->binarySize;

   while(length > 0)
   {
      // Prevent read operation to overflow buffer size
      n = MIN(sizeof(buffer), length);

      // Read image binary data
      error = driver->read(addr, buffer, n);
      // Is any error?
      if(error)
         return CBOOT_ERROR_FAILURE;

      // Update image binary data crc computation
      signatureHashAlgo->update(&hashContext, buffer, n);

      // Increment external flash memory word address
      addr += n;
      // Remaining bytes to be read
      length -= n;
   }

   // Finalize image binary data hash computation
   signatureHashAlgo->final(&hashContext, digest);

   size_t mod_len = mpiGetByteLength(&rsaPublicKey.n);
   error = rsassaPkcs1v15Verify(&rsaPublicKey, signatureHashAlgo, digest, originalSignature,
      mod_len);
   if(error)
   {
      TRACE_ERROR("Error verifying signature\r\n");
      return CBOOT_ERROR_FAILURE;
   }

   //Release EC public key
   rsaFreePublicKey(&rsaPublicKey);

   return CBOOT_NO_ERROR;
#endif

}
