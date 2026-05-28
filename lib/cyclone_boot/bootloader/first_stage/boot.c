/**
 * @file boot.c
 * @brief CycloneBOOT 1st Stage Bootloader management
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

#include "core/flash.h"
#include "core/mcu.h"
#include "core/crc32.h"
#include "image/image.h"
#include "boot.h"

/**
 * @brief Bootloader Task routine
 * @param[in] context Pointer to Bootloader context
 * @return cboot_error_t
 **/
cboot_error_t bootFsm(BootContext *context) {
   cboot_error_t error;

   error = CBOOT_NO_ERROR;

   if(context->address == context->slots[0])
   {
      TRACE_ERROR("Second stage bl address and download slot address is same. Aborting.\r\n");
      return CBOOT_ERROR_INVALID_PARAMETERS;
   }

   do
   {
      // Check slots to see if an update is available
      // If an update is available, copy it over the second stage slot, verify and reboot
      if(checkForSecondStageBlUpdate(context))
      {
         error = updateSecondStageBl(context);
         if(error != CBOOT_NO_ERROR)
         {
            TRACE_ERROR("Failed to update second stage bl.\r\n");
            break;
            //TODO: probably have a user definable callback function here
         }
      }

      // If no update is available, verify second stage slot and jump to it
      error = checkSecondStageBlIntegrity(context);
      if(error != CBOOT_NO_ERROR)
      {
         TRACE_ERROR("Failed to verify second stage bl integrity.\r\n");
         break;
         //TODO: probably have a user definable callback function here
      }

      jumpToSecondStageBl(context);

   } while(0);

   return error;

}

/**
 * @brief Calculate a CRC32 checksum over the memory slot containing the 2nd stage bootloader
 * @param[in] context Pointer to Bootloader context
 * @return cboot_error_t
 **/
cboot_error_t checkSecondStageBlIntegrity(BootContext *context) {
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
   TRACE_INFO("ADDR=0x%" PRIxPTR "\r\n", context->address);

   if(context->driver == NULL)
   {
      return CBOOT_ERROR_INVALID_PARAMETERS;
   }

   driver = context->driver;

   // Read slot data
   error = driver->read(context->address, buffer, sizeof(buffer));
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
   addr = context->address + sizeof(ImageHeader);

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

/**
 * @brief Branching to the 2nd stage bootloader
 * @param[in] context Pointer to Bootloader context
 **/
void jumpToSecondStageBl(BootContext *context) {
   TRACE_INFO("Jumping to second stage bl...\r\n");

   // Get MCU VTOR offset
   uint32_t mcuVtorOffset = mcuGetVtorOffset();

   // Jump to application at given address
   mcuJumpToApplication(context->address + context->offset + mcuVtorOffset);

}

/**
 * @brief Iterates over the available download slots
 * @param[in] context Pointer to Bootloader context
 * @return bool_t TRUE or FALSE based on the presence of a bootloader update
 **/
bool_t checkForSecondStageBlUpdate(BootContext *context) {
   error_t error;
   cboot_error_t cerror;
   ImageHeader *header;
   uint8_t buffer[sizeof(ImageHeader)];

   TRACE_INFO("Checking for second stage bl update...\r\n");
   for(size_t i = 0; i < context->slot_count; i++)
   {
      TRACE_INFO("Checking slot 0x%" PRIxPTR "\r\n", (uintptr_t)context->slots[i]);
      // Read the header and determine if it's a bootloader update
      error = context->driver->read(context->slots[i], buffer, sizeof(ImageHeader));
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
         TRACE_ERROR("Image header is not valid or alternatively, no bootloader update is present.\r\n");
         return FALSE;
      }

      if(header->imgType == IMAGE_TYPE_BOOT)
      {
         TRACE_INFO("Second stage bl update found.\r\n");
         context->update_address = context->slots[i];
         return TRUE;
      }

   }
   return FALSE;
}

/**
 * @brief Install the 2nd stage bootloader update.
 * @param[in] context Pointer to Bootloader context
 * @return cboot_error_t
 **/
cboot_error_t updateSecondStageBl(BootContext *context) {

   error_t error;
   size_t n;
   size_t imgAppSize;
   uint32_t readAddr;
   uint32_t writeAddr;
   ImageHeader *header;
   Crc32Context integrityContext;
   const HashAlgo *integrityAlgo;
   FlashDriver *internalDriver;
   const FlashInfo *internalDriverInfo;
   uint8_t buffer[512];

   // Check parameters validity?
   if(context == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   ////////////////////////////////////////////////////////////////////////////
   // Prepare an update process
   TRACE_INFO("Updating second stage bl...\r\n");

   // Get slot start address
   readAddr = context->update_address;
   // Get internal stlot address
   writeAddr = context->address;

   // Select CRC32 integrity algo
   integrityAlgo = CRC32_HASH_ALGO;

   // Point to the internal memory flash driver
   internalDriver = (FlashDriver *)context->driver;
   // Get internal driver information
   error = internalDriver->getInfo(&internalDriverInfo);
   if(error)
      return CBOOT_ERROR_FAILURE;

   ////////////////////////////////////////////////////////////////////////////
   // Read header of the image containing the new application firmware
   // Read update image slot for primary (internal) memory slot
   error = internalDriver->read(readAddr, buffer, sizeof(ImageHeader));
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Point to image header
   header = (ImageHeader *)buffer;

   // Write new image header into primary (internal) memory slot
   error = internalDriver->write(writeAddr, (uint8_t *)header, sizeof(ImageHeader));
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Save image application data size
   imgAppSize = header->dataSize;

   // Initialize CRC32 integrity algo context
   integrityAlgo->init(&integrityContext);

   // Start image check crc computation with image header
   integrityAlgo->update(&integrityContext, (uint8_t *)&header->headCrc, CRC32_DIGEST_SIZE);

   // Update write address
   writeAddr += sizeof(ImageHeader);

   ////////////////////////////////////////////////////////////////////////////
   // Transfer new application firmware data from the image slot
   //  in internal flash to the image slot in internal flash.

   // Get new image application data iv start address
   readAddr = context->update_address + sizeof(ImageHeader);

   // Loop through image application padding
   while(imgAppSize > 0)
   {
      n = MIN(sizeof(buffer), imgAppSize);
      // Read update image data from primary (internal) memory slot
      error = internalDriver->read(readAddr, buffer, n);
      // Is any error?
      if(error)
         return CBOOT_ERROR_FAILURE;

      // Update crc computation
      integrityAlgo->update(&integrityContext, buffer, n);

      // Avoid to write less that internal flash minimum write size
      if((n % internalDriverInfo->writeSize) == 0)
      {
         // Write image application data in primary (internal) memory slot
         error = internalDriver->write(writeAddr, buffer, n);
         // Is any error?
         if(error)
            return CBOOT_ERROR_FAILURE;
         writeAddr += n;
      }
      else
      {
         // Just for sanity
      }

      // writeAddr += n;
      readAddr += n;
      imgAppSize -= n;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Generate an image CRC32 integrity check section

   // Reset n if it was a multiple of internal flash minimum write size
   if((n > 0) && ((n % internalDriverInfo->writeSize) == 0))
   {
      n = 0;
   }

   // Finalize crc32 integrity algo computation
   integrityAlgo->final(&integrityContext, buffer + n);

   // Debug message
   TRACE_DEBUG("\r\n");
   TRACE_DEBUG("New image application CRC:\r\n");
   TRACE_DEBUG_ARRAY("CRC RAW: ", buffer + n, integrityAlgo->digestSize);

   // Write computed image check data in primary (internal) memory slot
   error = internalDriver->write(writeAddr, buffer, n + integrityAlgo->digestSize);
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   memset(buffer, 0, sizeof(buffer));

   // Remove the header of the installed update so that it does not get installed again upon Reboot
   error = internalDriver->write(context->update_address, buffer, sizeof(ImageHeader));
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Successful process
   return CBOOT_NO_ERROR;
}

#if ((defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) || defined(__GNUC__) ||            \
   defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__) || defined(__TASKING__) ||                  \
   defined(__CWCC__) || defined(__TI_ARM__))

__weak_func void bootHandleError(void)
{
   /*
    * This function will be called when the bootloader encounters a generic error
    * state. It can be redefined in user code.
    */
}

#endif
