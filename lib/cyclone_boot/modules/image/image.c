/**
 * @file image.c
 * @brief CycloneBOOT Image managment
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
#include "image.h"

#include "core/crc32.h"
#include "core/crypto.h"
#include "debug.h"
#include "memory/memory.h"

#include <stdlib.h>
#if BOOT_XIP_SUPPORT == ENABLED
#include "helper.h"
#endif
/**
 * @brief Check Internal Image header validity
 * @param[in] header Pointer to the internal image header to be checked
 * @return Status code
 **/

cboot_error_t imageCheckHeader(ImageHeader *header)
{
   error_t error;
   uint32_t computedCrc;

   // Check parameter validity
   if(header == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Compute image header crc
   error = CRC32_HASH_ALGO->compute((uint8_t *)header, sizeof(ImageHeader) - CRC32_DIGEST_SIZE,
      (uint8_t *)&computedCrc);
   if(error)
   {
      // Debug message
      TRACE_ERROR("Failed to compute image header crc!\r\n");
      return CBOOT_ERROR_FAILURE;
   }

   // Check image header integrity
   if(header->headCrc != computedCrc)
      return CBOOT_ERROR_INVALID_IMAGE_HEADER;

   // Check image header version
   if(header->headVers != IMAGE_HEADER_VERSION)
      return CBOOT_ERROR_INVALID_IMAGE_HEADER_VERSION;

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Get image header from data buffer
 * @param[in] buffer Data buffer containing the image header
 * @param[in] bufferLen Data buffer length
 * @param[out] header Pointer to the header structure to be returned
 * @return Status code
 **/

cboot_error_t imageGetHeader(uint8_t *buffer, size_t bufferLen, ImageHeader **header)
{
   cboot_error_t cerror;
   ImageHeader *tempHeader;

   // Check parameters validity
   if(buffer == NULL || bufferLen == 0 || header == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   if(bufferLen < sizeof(ImageHeader))
      return CBOOT_ERROR_INVALID_LENGTH;

   // Point to the image header
   tempHeader = (ImageHeader *)buffer;

   // Check image header
   cerror = imageCheckHeader(tempHeader);
   // Is any error?
   if(cerror)
      return cerror;

   // Save image header
   *header = tempHeader;

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Compute image header crc and store it in the image header crc field.
 * The given input header will also be the outout image header.
 * @param[in/out] header Pointer the header on which to calculate the crc.
 * @return Error code
 **/

cboot_error_t imageComputeHeaderCrc(ImageHeader *header)
{
   error_t error;

   // Check parameters validity
   if(header == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Compute image header crc
   error = CRC32_HASH_ALGO->compute((uint8_t *)header, sizeof(ImageHeader) - CRC32_DIGEST_SIZE,
      (uint8_t *)&header->headCrc);
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Successfull process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Verifies the complete image integrity.
 * @param[in/out] image Pointer to the image we want to verify.
 * @return Error code
 **/
cboot_error_t imageCheckIntegrity(Image *image)
{
   error_t error;
   cboot_error_t cerror;
   uint32_t addr;
   size_t n;
   size_t length;
   ImageHeader *header;
   Memory *memory;
   const FlashInfo *info;
   HashAlgo *crcAlgo;
   Crc32Context crcContext;
   uint8_t digest[CRC32_DIGEST_SIZE];
   uint8_t buffer[sizeof(ImageHeader)];
   FlashDriver *driver;
   Slot *slot;

   // Check parameter validity
   if(image == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Point to slot memory descriptor
   slot = image->activeSlot;
   memory = (Memory *)slot->memParent;

#if BOOT_XIP_SUPPORT == ENABLED

   flashActivateXiPMode(1);

   header = (ImageHeader *)slot->addr;

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

   // Check image size
   if(length + sizeof(ImageHeader) > slot->size)
   {
      // Debug message
      TRACE_ERROR("Image size is invalid!\r\n");
      return CBOOT_ERROR_INVALID_LENGTH;
   }

   // Point to the CRC32 algorithm
   crcAlgo = (HashAlgo *)CRC32_HASH_ALGO;
   // Initialize CRC algorithm
   crcAlgo->init(&crcContext);
   // Start image check computation with image header crc
   crcAlgo->update(&crcContext, (uint8_t *)&header->headCrc, CRC32_DIGEST_SIZE);
   // check the image body
   crcAlgo->update(&crcContext, (uint8_t *)addr, header->dataSize);
   // Finalize image binary data crc computation
   crcAlgo->final(&crcContext, digest);

   addr += header->dataSize;

   // Compare given against computed image binary crc
   if(memcmp((uint32_t *)addr, digest, CRC32_DIGEST_SIZE) != 0)
   {
      // Debug message
      TRACE_ERROR("Image binary data is not valid!\r\n");
      TRACE_DEBUG("Computed check CRC: ");
      TRACE_DEBUG_ARRAY("", digest, CRC32_DIGEST_SIZE);
      TRACE_DEBUG("Given Check CRC: ");
      TRACE_DEBUG_ARRAY("", buffer, CRC32_DIGEST_SIZE);
      return CBOOT_ERROR_FIRMWARE_CORRUPTED;
   }

#else
   // Get memory info
   driver = (FlashDriver *)memory->driver;
   if(driver == NULL)
      return CBOOT_ERROR_ABORTED;


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
   header = (ImageHeader *)buffer;

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

   // Check image size
   if(length + sizeof(ImageHeader) > slot->size)
   {
      // Debug message
      TRACE_ERROR("Image size is invalid!\r\n");
      return CBOOT_ERROR_INVALID_LENGTH;
   }

   // Point to the CRC32 algorithm
   crcAlgo = (HashAlgo *)CRC32_HASH_ALGO;
   // Initialize CRC algorithm
   crcAlgo->init(&crcContext);
   // Start image check computation with image header crc
   crcAlgo->update(&crcContext, (uint8_t *)&header->headCrc, CRC32_DIGEST_SIZE);

   // Process image binary data
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
      crcAlgo->update(&crcContext, buffer, n);

      // Increment external flash memory word address
      addr += n;
      // Remaining bytes to be read
      length -= n;
   }

   // Finalize image binary data crc computation
   crcAlgo->final(&crcContext, digest);

   // Read given image binary crc
   error = driver->read(addr, buffer, CRC32_DIGEST_SIZE);
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Compare given against computed image binary crc
   if(memcmp(buffer, digest, CRC32_DIGEST_SIZE) != 0)
   {
      // Debug message
      TRACE_ERROR("Image binary data is not valid!\r\n");
      TRACE_DEBUG("Computed check CRC: ");
      TRACE_DEBUG_ARRAY("", digest, CRC32_DIGEST_SIZE);
      TRACE_DEBUG("Given Check CRC: ");
      TRACE_DEBUG_ARRAY("", buffer, CRC32_DIGEST_SIZE);
      return CBOOT_ERROR_FIRMWARE_CORRUPTED;
   }
#endif
   // Successfully processed
   return CBOOT_NO_ERROR;
}
