/**
 * @file boot.c
 * @brief CycloneBOOT 1st Stage Bootloader management
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

#include "first_stage/boot.h"
#include "first_stage/boot_common.h"

/**
 * @brief Bootloader Task routine
 * @param[in] context Pointer to Bootloader context
 * @return cboot_error_t
 **/
cboot_error_t bootFsm(BootContext *context) {
   cboot_error_t error;
   bool_t updateFound = FALSE;
   uint8_t blImageIndex = 0;

   error = CBOOT_NO_ERROR;

   if(context == NULL || context->driver == NULL)
      return CBOOT_ERROR_FAILURE;

   if(context->slot_count == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   if(context->self_address == 0)
   {
      TRACE_ERROR("Invalid first stage bl address. Aborting.\r\n");
      return CBOOT_ERROR_INVALID_PARAMETERS;
   }

   if(context->bl_address == 0)
   {
      TRACE_ERROR("Invalid second stage bl address. Aborting.\r\n");
      return CBOOT_ERROR_INVALID_PARAMETERS;
   }

   // Making sure we are not accessing invalid memory
   for(size_t i = 0; i < context->slot_count; i++)
   {
      if(context->self_address == context->slots[i])
      {
         TRACE_ERROR("First stage bl address overlaps with slot %d. Aborting.\r\n", i);
         return CBOOT_ERROR_INVALID_PARAMETERS;
      }

      if(context->bl_address == context->slots[i])
      {
         TRACE_ERROR("Second stage bl address overlaps with slot %d. Aborting.\r\n", i);
         return CBOOT_ERROR_INVALID_PARAMETERS;
      }
   }

   // Fetching the index that will be compared later on to determine if an update should be performed
   error = getSecondStageBlIndex(context, &blImageIndex);
   if(error != CBOOT_NO_ERROR)
      return error;

   do
   {
      // Check slots to see if an update is available
      error = checkForSecondStageBlUpdate(context, blImageIndex, &updateFound);
      if(error != CBOOT_NO_ERROR)
         break;

      if(updateFound)
      {
         // If an update is available, copy it over the second stage slot
         error = updateSecondStageBl(context);
         if(error != CBOOT_NO_ERROR)
         {
            TRACE_ERROR("Failed to update second stage bl.\r\n");
            break;
            //TODO: implement a user definable callback function here
            //TODO: recovery here
         }
      }

      // If no update is available, verify the second stage slot
      error = bootCheckImage(context);
      if(error != CBOOT_NO_ERROR)
      {
         TRACE_ERROR("Failed to verify second stage bl integrity.\r\n");
         break;
         //TODO: implement a user definable callback function here
         //TODO: recovery here
      }

      // If all is well, jump to it
      jumpToSecondStageBl(context);

   } while(0);

   // At this point, we have already branched to the 2nd stage bl
   // This error code is useful for testing
   return error;

}

/**
 * @brief Get the image index of the second stage bootloader
 * @param[in] context Pointer to Bootloader context
 * @return cboot_error_t
 **/
cboot_error_t getSecondStageBlIndex(BootContext *context, uint8_t *index) {
   error_t error;
   cboot_error_t cerror;

   const FlashDriver *driver;
   ImageHeader header;

   if(context == NULL || context->driver == NULL)
   {
      return CBOOT_ERROR_INVALID_PARAMETERS;
   }

   TRACE_INFO("Reading second stage bootloader header...\r\n");
   TRACE_INFO("ADDR=0x%" PRIx32 "\r\n", context->bl_address);

   driver = context->driver;

   // Read slot data
   error = driver->read(context->bl_address, (uint8_t *)&header, sizeof(header));
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Check internal image header
   cerror = imageCheckHeader(&header);
   // Is any error?
   if(cerror)
   {
      // Debug message
      TRACE_ERROR("Image header is not valid!\r\n");
      return cerror;
   }

   if(index != NULL)
      *index = header.imgIndex;

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
   mcuJumpToApplication(context->bl_address + context->offset + mcuVtorOffset);

}

/**
 * @brief Iterates over the available download slots
 * @param[in] context Pointer to Bootloader context
 * @return bool_t TRUE or FALSE based on the presence of a bootloader update
 * **/
cboot_error_t checkForSecondStageBlUpdate(BootContext *context, uint8_t currentIndex, bool_t *updateFound) {
   error_t error;
   cboot_error_t cerror;
   ImageHeader header;
   *updateFound = FALSE;
   uint8_t bestIndex = currentIndex;

   if(context == NULL || context->driver == NULL || updateFound == NULL)
      return CBOOT_ERROR_FAILURE;

   TRACE_INFO("Checking for second stage bl update...\r\n");
   for(size_t i = 0; i < context->slot_count; i++)
   {
      TRACE_INFO("Checking slot 0x%" PRIxPTR "\r\n", (uintptr_t)context->slots[i]);

      // Read the header and determine if it's a bootloader update
      error = context->driver->read(context->slots[i], (uint8_t *)&header, sizeof(ImageHeader));
      // Is any error?
      if(error)
         return CBOOT_ERROR_FAILURE;

      // Check internal image header
      cerror = imageCheckHeader(&header);
      // Is any error?
      if(cerror)
      {
         // Debug message
         TRACE_ERROR("Image header is not valid or alternatively, no bootloader update is present.\r\n");
         continue;
      }

      if(header.imgType == IMAGE_TYPE_BOOT && header.imgIndex > bestIndex)
      {
         TRACE_INFO("Newer second stage bl found in slot %d (index %d).\r\n", i, header.imgIndex);
         bestIndex = header.imgIndex;
         context->update_address = context->slots[i];
         *updateFound = TRUE;
      }
      else
      {
         TRACE_INFO("No second stage bl update found.\r\n");
      }
   }

   return CBOOT_NO_ERROR;
}

/**
 * @brief Install the 2nd stage bootloader update.
 * @param[in] context Pointer to Bootloader context
 * @return cboot_error_t
 **/
cboot_error_t updateSecondStageBl(BootContext *context) {

   error_t error;
   size_t n = 0;
   size_t imgAppSize;
   uint32_t readAddr;
   uint32_t writeAddr;
   ImageHeader *header;
   Crc32Context integrityContext;
   const HashAlgo *integrityAlgo;
   const FlashDriver *internalDriver;
   const FlashInfo *internalDriverInfo;
   uint8_t buffer[512];

   // Check parameters validity?
   if(context == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   ////////////////////////////////////////////////////////////////////////////
   // Prepare an update process
   TRACE_INFO("Updating second stage bl...\r\n");

   // Get the slot start address
   readAddr = context->update_address;
   // Get the internal slot address
   writeAddr = context->bl_address;

   // Select CRC32 integrity algo
   integrityAlgo = CRC32_HASH_ALGO;

   // Point to the internal memory flash driver
   internalDriver = context->driver;
   if(internalDriver == NULL)
      return CBOOT_ERROR_FAILURE;

   // Get internal driver information
   error = internalDriver->getInfo(&internalDriverInfo);
   if(error)
      return CBOOT_ERROR_FAILURE;

   ////////////////////////////////////////////////////////////////////////////
   // Read the header of the image containing the new application firmware
   // Read update image slot for primary (internal) memory slot
   error = internalDriver->read(readAddr, buffer, sizeof(ImageHeader));
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Point to image header
   header = (ImageHeader *)buffer;

   // Save image application data size
   imgAppSize = header->dataSize;

   // Ensure the target does not overlap with the first stage BL
   uint32_t writeEnd = writeAddr + sizeof(ImageHeader) + imgAppSize;
   uint32_t selfEnd  = context->self_address + context->self_size;

   if(writeAddr < selfEnd && writeEnd > context->self_address)
   {
      TRACE_ERROR("Write target overlaps with first stage BL. Aborting.\r\n");
      return CBOOT_ERROR_FAILURE;
   }

   // Write a new image header into the primary (internal) memory slot
   error = internalDriver->write(writeAddr, (uint8_t *)header, sizeof(ImageHeader));
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Initialize CRC32 integrity algo context
   integrityAlgo->init(&integrityContext);

   // Start image check crc computation with the image header
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
      // Read update image data from the primary (internal) memory slot
      error = internalDriver->read(readAddr, buffer, n);
      // Is any error?
      if(error)
         return CBOOT_ERROR_FAILURE;

      // Update crc computation
      integrityAlgo->update(&integrityContext, buffer, n);

      // Avoid writing less that internal flash minimum write size
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

   // Write computed image check data in the primary (internal) memory slot
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

   TRACE_INFO("Second stage bl update complete.\r\n");

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
