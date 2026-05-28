/**
 * @file boot_common.c
 * @brief CycloneBOOT 1st Stage Bootloader Utility Functions
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

#include "boot_common.h"

/**
 * @brief Calculate a CRC32 checksum over the memory slot containing the 2nd stage bootloader
 * @param[in] context Pointer to Bootloader context
 * @return cboot_error_t
 **/
cboot_error_t bootCheckImage(BootContext *context) {
   error_t error;
   cboot_error_t cerror;
   uint32_t addr;
   size_t length;
   size_t n;

   HashAlgo *crcAlgo;
   Crc32Context crcContext;
   uint8_t digest[CRC32_DIGEST_SIZE];

   const FlashDriver *driver;
   ImageHeader *header;
   uint8_t buffer[sizeof(ImageHeader)];

   TRACE_INFO("Verifying second stage bootloader...\r\n");
   TRACE_INFO("ADDR=0x%" PRIxPTR "\r\n", context->bl_address);

   if(context->driver == NULL)
   {
      return CBOOT_ERROR_INVALID_PARAMETERS;
   }

   driver = context->driver;

   // Read slot data
   error = driver->read(context->bl_address, buffer, sizeof(buffer));
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
   addr = context->bl_address + sizeof(ImageHeader);

   // Save internal image data size
   length = header->dataSize;

   // Point to the CRC32 algorithm
   crcAlgo = (HashAlgo *) CRC32_HASH_ALGO;
   // Initialize CRC algorithm
   crcAlgo->init(&crcContext);
   // Start image check computation with image header crc
   crcAlgo->update(&crcContext, (uint8_t *) &header->headCrc, CRC32_DIGEST_SIZE);

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

   return CBOOT_NO_ERROR;
}
