/**
 * @file boot_common.c
 * @brief Bootloader common functions
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
#define TRACE_LEVEL BOOT_TRACE_LEVEL

// Dependencies
#if !defined(_WIN32) && !defined(__linux__) && !defined(__FreeBSD__)
#include "cmsis_compiler.h"
#endif

#include "core/crc32.h"
#include "debug.h"
#include "error.h"
#include "image/image.h"
#include "image/image_utils.h"
#include "second_stage/boot.h"
#include "second_stage/boot_common.h"

// Include crypto header files needed for image decryption
#if (BOOT_EXT_MEM_ENCRYPTION_SUPPORT == ENABLED)
#include "cipher/aes.h"
#include "cipher/cipher_algorithms.h"
#include "cipher_modes/cbc.h"
#include "core/crypto.h"
#include "security/cipher.h"
#endif

#if defined(_WIN32)
#define __attribute__(x)
#endif

bool_t bootCheckNoSlotOverlap(Slot *s1, Slot *s2);

/**
 * @brief Intialize bootloader primary flash memory.
 * @param[in,out] context Pointer the bootloader context.
 * @param[in] settings Bootloader user settings used to initialize primary
 * flash.
 * @return Error code
 **/

cboot_error_t bootInitPrimaryMem(BootContext *context, BootSettings *settings)
{
   error_t error;
   Memory *primaryMemory;
   FlashDriver *flashDriver;
   const FlashInfo *flashInfo;
   bool_t ret;

   // Check parameters validity
   if(context == NULL || settings == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Check a primary flash driver is valid
   if(settings->memories[0].driver == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Point to the primary memory context
   primaryMemory = (Memory *)&context->memories[0];

   // Point to a memory driver
   flashDriver = (FlashDriver *)settings->memories[0].driver;

   // Initialize primary (internal) a memory flash driver
   error = flashDriver->init();
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Get memory driver information
   error = flashDriver->getInfo(&flashInfo);
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Check if user primary flash slot 0 address matches a flash sector address
   ret = flashDriver->isSectorAddr(settings->memories[0].slots[0].addr);
   if(!ret)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Check primary flash slot 0 fits in primary flash
   if((settings->memories[0].slots[0].addr + settings->memories[0].slots[0].size) >
      (flashInfo->flashAddr + flashInfo->flashSize))
      return CBOOT_ERROR_INVALID_PARAMETERS;

#if EXTERNAL_MEMORY_SUPPORT == DISABLED
   // Check if user primary flash slot 1 address matches a flash sector address
   ret = flashDriver->isSectorAddr(settings->memories[0].slots[1].addr);
   if(!ret)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Check primary flash slot 1 fits in primary flash
   if((settings->memories[0].slots[1].addr + settings->memories[0].slots[1].size) >
      (flashInfo->flashAddr + flashInfo->flashSize))
      return CBOOT_ERROR_INVALID_PARAMETERS;

#if BOOT_FALLBACK_SUPPORT == ENABLED
   // Check if user primary flash slot 2 address matches a flash sector address
   ret = flashDriver->isSectorAddr(settings->memories[0].slots[2].addr);
   if(!ret)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Check primary flash slot 2 fits in primary flash
   if((settings->memories[0].slots[2].addr + settings->memories[0].slots[2].size) >
      (flashInfo->flashAddr + flashInfo->flashSize))
      return CBOOT_ERROR_INVALID_PARAMETERS;
#endif
#endif

   // Initialize primary memory
   primaryMemory->memoryType = settings->memories[0].memoryType;
   primaryMemory->driver = settings->memories[0].driver;
   primaryMemory->nbSlots = settings->memories[0].nbSlots;

   // Set the primary flash memory slot 0 which hold current running application
   // This slot MUST be located after the bootloader at the beginning of the next
   // available flash sector
   primaryMemory->slots[0].type = settings->memories[0].slots[0].type;
   primaryMemory->slots[0].cType = settings->memories[0].slots[0].cType;
   primaryMemory->slots[0].addr = settings->memories[0].slots[0].addr;
   primaryMemory->slots[0].size = settings->memories[0].slots[0].size;
   primaryMemory->slots[0].memParent = &context->memories[0];

#if EXTERNAL_MEMORY_SUPPORT == DISABLED
   // Set the primary flash memory slot 1 which hold update image (and back image
   // if fallback is activated)
   primaryMemory->slots[1].type = settings->memories[0].slots[1].type;
   primaryMemory->slots[1].cType = settings->memories[0].slots[1].cType;
   primaryMemory->slots[1].addr = settings->memories[0].slots[1].addr;
   primaryMemory->slots[1].size = settings->memories[0].slots[1].size;
   primaryMemory->slots[1].memParent = &context->memories[0];

   // Making sure the two primary slots 0 and 1 does not overlap
   ret = bootCheckNoSlotOverlap(&primaryMemory->slots[0], &primaryMemory->slots[1]);
   if(ret)
   {
      return CBOOT_ERROR_INVALID_ADDRESS;
   }

#if BOOT_FALLBACK_SUPPORT == ENABLED
   // Set the primary flash memory slot 2 which hold update image and back image
   // if fallback is activated
   primaryMemory->slots[2].type = settings->memories[0].slots[2].type;
   primaryMemory->slots[2].cType = settings->memories[0].slots[2].cType;
   primaryMemory->slots[2].addr = settings->memories[0].slots[2].addr;
   primaryMemory->slots[2].size = settings->memories[0].slots[2].size;
   primaryMemory->slots[2].memParent = &context->memories[0];

   // Making sure this third slot does not overlap with first primary slot
   ret = bootCheckNoSlotOverlap(&primaryMemory->slots[0], &primaryMemory->slots[2]);
   if(ret)
   {
      return CBOOT_ERROR_INVALID_ADDRESS;
   }

   // Making sure this third slot does not overlap with first primary slot
   ret = bootCheckNoSlotOverlap(&primaryMemory->slots[1], &primaryMemory->slots[2]);
   if(ret)
   {
      return CBOOT_ERROR_INVALID_ADDRESS;
   }
#endif
#endif

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Initialize bootloader secondary (external) flash memory.
 * @param[in,out] context Pointer the bootloader context.
 * @param[in] settings Bootloader user settings used to initialize secondary
 * flash.
 * @return Error code
 **/

cboot_error_t bootInitSecondaryMem(BootContext *context, BootSettings *settings)
{
   error_t error;
   Memory *secondaryMemory;
   FlashDriver *flashDriver;
   const FlashInfo *flashInfo;
   bool_t ret;

   // Check parameters validity
   if(context == NULL || settings == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Check if the secondary flash driver is valid
   if(settings->memories[1].driver == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Point to the secondary memory context
   secondaryMemory = (Memory *)&context->memories[1];

   // Set secondary flash memory driver
   secondaryMemory->driver = settings->memories[1].driver;

   // Point to a memory driver
   flashDriver = (FlashDriver *)secondaryMemory->driver;

   // Initialize a secondary (internal) memory flash driver
   error = flashDriver->init();
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Get memory driver information
   error = flashDriver->getInfo(&flashInfo);
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Check if user secondary flash slot 1 address matches a flash sector address
   ret = flashDriver->isSectorAddr(settings->memories[1].slots[0].addr);
   if(!ret)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Check secondary flash slot 1 fits in secondary flash
   if((settings->memories[1].slots[0].addr + settings->memories[1].slots[0].size) >
      (flashInfo->flashAddr + flashInfo->flashSize))
      return CBOOT_ERROR_INVALID_PARAMETERS;

#if (BOOT_FALLBACK_SUPPORT == ENABLED)
   // Check if user secondary flash slot 2 address matches a flash sector address
   ret = flashDriver->isSectorAddr(settings->memories[1].slots[1].addr);
   if(!ret)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Check secondary flash slot 2 fits in secondary flash
   if((settings->memories[1].slots[1].addr + settings->memories[1].slots[1].size) >
      (flashInfo->flashAddr + flashInfo->flashSize))
      return CBOOT_ERROR_INVALID_PARAMETERS;
#endif

   // Set secondary flash memory slot 1 which will hold the new update image
   // If fallback support is enabled, slot 1 could also hold the
   // backup image of the current running application
   secondaryMemory->memoryType = settings->memories[1].memoryType;
   secondaryMemory->driver = settings->memories[1].driver;
   secondaryMemory->nbSlots = settings->memories[1].nbSlots;

   secondaryMemory->slots[0].type = settings->memories[1].slots[0].type;
   secondaryMemory->slots[0].cType = settings->memories[1].slots[0].cType;
   secondaryMemory->slots[0].addr = settings->memories[1].slots[0].addr;
   secondaryMemory->slots[0].size = settings->memories[1].slots[0].size;
   secondaryMemory->slots[0].memParent = secondaryMemory;

#if (BOOT_FALLBACK_SUPPORT == ENABLED)
   // Set secondary flash memory slot 2 which will hold the new update image
   // or the backup image of the current running application
   secondaryMemory->slots[1].type = settings->memories[1].slots[1].type;
   secondaryMemory->slots[1].cType = settings->memories[1].slots[1].cType;
   secondaryMemory->slots[1].addr = settings->memories[1].slots[1].addr;
   secondaryMemory->slots[1].size = settings->memories[1].slots[1].size;
   secondaryMemory->slots[1].memParent = secondaryMemory;

   // Making sure the two secondary slots does not overlap
   ret = bootCheckNoSlotOverlap(&secondaryMemory->slots[0], &secondaryMemory->slots[1]);
   if(ret)
   {
      return CBOOT_ERROR_INVALID_ADDRESS;
   }
#endif

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Select the slot in external memory that hold the the update image.
 * @param[in] context Pointer to the bootloader context
 * @param[out] selectedSlot Pointer to the slot containing the update image.
 * @erturn Error code.
 **/

cboot_error_t bootSelectUpdateImageSlot(BootContext *context, Slot *selectedSlot)
{
   cboot_error_t cerror;
   uint_t i;
   Slot tmpSlot;
   ImageHeader tmpImgHeader;
   uint32_t tmpImgIndex;
#if (BOOT_ANTI_ROLLBACK_SUPPORT == ENABLED)
   ImageVersion tmpImgDataVers;
   ImageVersionComparisonFlag tmpImgCompFlag;
#endif

   // Initialize status code
   cerror = CBOOT_NO_ERROR;

   // Check parameter validity
   if(context == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Point to the primary flash memory slot (contains current image application)
   tmpSlot = context->memories[0].slots[0];

   // Get the header of the image inside the current slot
   cerror = bootGetSlotImgHeader(&tmpSlot, &tmpImgHeader);

   // Recover error code
   // Here no need to proceed further as current firmware is corrupted.
   if(cerror != CBOOT_NO_ERROR)
      return CBOOT_ERROR_FIRMWARE_CORRUPTED;

   // Check image header of the first primary slot is valid
   if(!cerror)
   {
      // Save image index number from current slot
      tmpImgIndex = tmpImgHeader.imgIndex;
#if (BOOT_ANTI_ROLLBACK_SUPPORT == ENABLED)
      // Save image data version from current slot
      tmpImgDataVers = tmpImgHeader.dataVers;
#endif

      // Save current selected slot
      *selectedSlot = tmpSlot;

#if (BOOT_FALLBACK_SUPPORT == ENABLED)
      // Loop through memory slots
      for(i = 0; i < 2; i++)
#else
      // Loop through memory slot(s)
      for(i = 0; i < 1; i++)
#endif
      {
#if EXTERNAL_MEMORY_SUPPORT == ENABLED
         // Point to the current indexed secondary flash memory slot
         tmpSlot = context->memories[1].slots[i];
#else
         // Point to the current indexed primary flash memory slot
         tmpSlot = context->memories[0].slots[i + 1];
#endif
         // Get the header of the image inside the current slot
         cerror = bootGetSlotImgHeader(&tmpSlot, &tmpImgHeader);
         // Is any error?
         if(cerror)
         {
            // Discard error
            cerror = CBOOT_NO_ERROR;
         }
         else
         {
#if (BOOT_ANTI_ROLLBACK_SUPPORT == ENABLED)
            // Is temporary image more recent than the image of the listed slot?
            // If anti-rollback support is activated then temporary image index and
            //  image firmware version MUST both be more recent than the listed
            //  the image index and image firmware version of the listed slot.
            imageCompareFirmwareVersions(&tmpImgHeader.dataVers, &tmpImgDataVers,
               &tmpImgCompFlag);
            if((tmpImgHeader.imgIndex > tmpImgIndex) &&
               (tmpImgCompFlag == IMAGE_VERSION_SUPERIOR))
#else
            // Is temporary image more recent than the image of the listed slot?
            // If anti-rollback support is not activated, then the temporary image
            // index
            //  MUST be more recent than the listed the image index of the listed
            //  slot.
            if(tmpImgHeader.imgIndex > tmpImgIndex)
#endif
            {
               // Save image index number from current listed slot
               tmpImgIndex = tmpImgHeader.imgIndex;
#if (BOOT_ANTI_ROLLBACK_SUPPORT == ENABLED)
               // Save image data version from current listed slot
               tmpImgDataVers = tmpImgHeader.dataVers;
#endif

               // Update selected slot
               *selectedSlot = tmpSlot;
            }
         }
      }
   }

   // Is any error?
   if(cerror)
   {
      // Unselect slot
      selectedSlot = NULL;
   }

   // Return status code
   return cerror;
}

/**
 * @brief Update current application. Basically it decrypt/copy an image
 * from the external flash memory into the internal flash memory.
 * @param[in] context Pointer to Bootloader context
 * @param[in] slot Pointer to the slot in the external flash memory that
 * contains the new application
 * @return Status code
 **/

cboot_error_t bootUpdateApp(BootContext *context, Slot *slot)
{
   error_t error;
   size_t n;
   size_t imgAppSize;
   uint32_t readAddr;
   uint32_t writeAddr;
   ImageHeader *header;
   Crc32Context integrityContext;
   const HashAlgo *integrityAlgo;
   Memory *intMem;
   FlashDriver *internalDriver;
   const FlashInfo *internalDriverInfo;
#if (EXTERNAL_MEMORY_SUPPORT == ENABLED)
   Memory *extMem;
   FlashDriver *externalDriver;
#endif

#if (BOOT_EXT_MEM_ENCRYPTION_SUPPORT == ENABLED)
   AesContext cipherContext;
   const CipherAlgo *cipherAlgo;
   uint8_t iv[INIT_VECT_SIZE];
#endif
   uint8_t buffer[512];

   // Check parameters validity?
   if(context == NULL || slot == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   ////////////////////////////////////////////////////////////////////////////
   // Prepare an update process

   // Point to the internal slot memory descriptor
   intMem = &context->memories[0];
#if (EXTERNAL_MEMORY_SUPPORT == ENABLED)
   // Point to the slot memory descriptor
   extMem = (Memory *)slot->memParent;
#endif

   // Get slot start address
   readAddr = slot->addr;
   // Get internal stlot address
   writeAddr = intMem->slots[0].addr;

   // Select CRC32 integrity algo
   integrityAlgo = CRC32_HASH_ALGO;

#if (EXTERNAL_MEMORY_SUPPORT == ENABLED)
   // Point to the external memory flash driver
   externalDriver = (FlashDriver *)extMem->driver;
#endif
   // Point to the internal memory flash driver
   internalDriver = (FlashDriver *)intMem->driver;
   // Get internal driver information
   error = internalDriver->getInfo(&internalDriverInfo);
   if(error)
      return CBOOT_ERROR_FAILURE;

   ////////////////////////////////////////////////////////////////////////////
   // Read header of the image containing the new application firmware

#if (EXTERNAL_MEMORY_SUPPORT == ENABLED)
   // Read update image slot for secondary (external) memory slot
   error = externalDriver->read(readAddr, buffer, sizeof(ImageHeader));
#else
   // Read update image slot for primary (internal) memory slot
   error = internalDriver->read(readAddr, buffer, sizeof(ImageHeader));
#endif
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
   //  in external flash to the image slot in internal flash.

   // Get new image application data iv start address
   readAddr = slot->addr + sizeof(ImageHeader);

#if (BOOT_EXT_MEM_ENCRYPTION_SUPPORT == ENABLED)
#if (EXTERNAL_MEMORY_SUPPORT == ENABLED)
   // Read iv from external flash memory image slot
   error = externalDriver->read(readAddr, iv, INIT_VECT_SIZE);
#else
   // Read iv from internal flash memory image slot
   error = internalDriver->read(readAddr, iv, INIT_VECT_SIZE);
#endif
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Discard iv
   readAddr += INIT_VECT_SIZE;

   // Debug message
   TRACE_DEBUG("\r\n");
   TRACE_DEBUG("Original IV:\r\n");
   TRACE_DEBUG_ARRAY("IV RAW: ", iv, INIT_VECT_SIZE);

   // Select AES cipher algo
   cipherAlgo = AES_CIPHER_ALGO;

   // Initialize AES cipher algo context
   error = cipherAlgo->init(&cipherContext, (uint8_t *)context->psk, context->pskSize);
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

#if (EXTERNAL_MEMORY_SUPPORT == ENABLED)
   // Read update image data from secondary (external) memory slot
   error = externalDriver->read(readAddr, buffer, AES_BLOCK_SIZE);
#else
   // Read update image data from primary (internal) memory slot
   error = internalDriver->read(readAddr, buffer, AES_BLOCK_SIZE);
#endif
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Decipher data
   error = cbcDecrypt(cipherAlgo, &cipherContext, iv, buffer, buffer, AES_BLOCK_SIZE);
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Discard image cipher magic number
   readAddr += AES_BLOCK_SIZE;

#endif

   // Loop through image application padding
   while(imgAppSize > 0)
   {
      n = MIN(sizeof(buffer), imgAppSize);

#if (EXTERNAL_MEMORY_SUPPORT == ENABLED)
      // Read update image data from secondary (external) memory slot
      error = externalDriver->read(readAddr, buffer, n);
#else
      // Read update image data from primary (internal) memory slot
      error = internalDriver->read(readAddr, buffer, n);
#endif
      // Is any error?
      if(error)
         return CBOOT_ERROR_FAILURE;

#if (BOOT_EXT_MEM_ENCRYPTION_SUPPORT == ENABLED)
      // Decipher data
      error = cbcDecrypt(cipherAlgo, &cipherContext, iv, buffer, buffer, n);
      // Is any error?
      if(error)
         return CBOOT_ERROR_FAILURE;
#endif

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

   // Successful process
   return CBOOT_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Check image validity within the given slot.
 * @param[in] slot Pointer to the slot containing the image to be checked.
 * @return Error code.
 **/

cboot_error_t bootCheckImage(BootContext *context, Slot *slot)
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

#if ((EXTERNAL_MEMORY_SUPPORT == ENABLED) && (BOOT_EXT_MEM_ENCRYPTION_SUPPORT == ENABLED))
   AesContext cipherContext;
   const CipherAlgo *cipherAlgo;
   uint8_t iv[INIT_VECT_SIZE];
   bool_t magicNumberIsValid;
   uint32_t magicNumberCrc;
#endif

   // Check parameter validity
   if(context == NULL || slot == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Point to slot memory descriptor
   memory = (Memory *)slot->memParent;

   // Get memory info
   driver = (FlashDriver *)memory->driver;
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

#if ((EXTERNAL_MEMORY_SUPPORT == ENABLED) && (BOOT_EXT_MEM_ENCRYPTION_SUPPORT == ENABLED))
   // This part should be done only if support for external memory and encryption
   // of external memory are enabled.
   //  In other case:
   //     - if encryption of external memory is enabled but not support of
   //     external memory -> should return a compilation error
   //     - if both support for external memory and encryption of external memory
   //     are disabled then don't do this part
   //     - if support of external memory is enabled but not support of
   //     encryption of external memory then don't do this part

   // Is image in external flash memory?
   if((info->flashType != FLASH_TYPE_INTERNAL) && (slot->cType & SLOT_CONTENT_UPDATE))
   {
      // Image in external flash memory are encrypted and the iv vector used to
      // encrypt
      //  the image application is part of the check data calculation
      // Add iv size
      length += INIT_VECT_SIZE;

      // Read image cipher iv
      error = driver->read(addr, iv, INIT_VECT_SIZE);
      // Is any error?
      if(error)
         return CBOOT_ERROR_FAILURE;

      // Select AES cipher algo
      cipherAlgo = AES_CIPHER_ALGO;

      // Initialize AES cipher algo context
      error = cipherAlgo->init(&cipherContext, (uint8_t *)context->psk, context->pskSize);
      // Is any error?
      if(error)
         return CBOOT_ERROR_FAILURE;

      // Read encrypted padded cipher image magic number crc
      error = driver->read(addr + INIT_VECT_SIZE, buffer, AES_BLOCK_SIZE);
      // Is any error?
      if(error)
         return CBOOT_ERROR_FAILURE;

      // Decipher padded cipher image magic number crc
      error = cbcDecrypt(cipherAlgo, &cipherContext, iv, buffer, buffer, AES_BLOCK_SIZE);
      // Is any error?
      if(error)
         return CBOOT_ERROR_FAILURE;

      // Save cipher image magic number crc for later check
      magicNumberCrc = *(uint32_t *)buffer;

      // Encrypted padded cipher image magic number crc is also part of the check
      // data calculation Add encrypted padded cipher image magic number crc size
      length += AES_BLOCK_SIZE;
   }
#endif

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

#if ((EXTERNAL_MEMORY_SUPPORT == ENABLED) && (BOOT_EXT_MEM_ENCRYPTION_SUPPORT == ENABLED))
   // Is image in external flash memory?
   if((info->flashType != FLASH_TYPE_INTERNAL) && (slot->cType & SLOT_CONTENT_UPDATE))
   {
      // Check cipher image magic number crc
      cerror = cipherCheckMagicNumberCrc(magicNumberCrc, &magicNumberIsValid);
      // Is any error or cipher image magic number crc is invalid?
      if(cerror || !magicNumberIsValid)
      {
         // Debug message
         TRACE_ERROR("Image cipher magic number crc is not valid!\r\n");
         TRACE_ERROR("The wrong cipher key has been used!\r\n");
         return CBOOT_ERROR_FAILURE;
      }
   }
#endif

   // Successfully processed
   return CBOOT_NO_ERROR;
}

/**
 * @brief Get header from the image inside the given slot.
 * @param[in] slot Pointer to the slot that contains the image header
 * @param[out] header Pointer that will hold the retrieved image header.
 * @return Error code.
 **/

cboot_error_t bootGetSlotImgHeader(Slot *slot, ImageHeader *header)
{
   error_t error;
   cboot_error_t cerror;
   uint8_t buffer[sizeof(ImageHeader)];
   ImageHeader *tmpHeader;
   Memory *memory;
   FlashDriver *driver;

   // Initialize status code
   cerror = CBOOT_NO_ERROR;

   // Check parameter validity
   if(slot == NULL)
      return cerror;

   memory = (Memory *)slot->memParent;
   driver = (FlashDriver *)memory->driver;

   // Read first slot data that should correspond to the image header
   error = driver->read(slot->addr, buffer, sizeof(buffer));
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Extract image header from the read data
   cerror = imageGetHeader(buffer, sizeof(buffer), &tmpHeader);
   // Is any error?
   if(cerror)
      return cerror;

   // Save image header
   memcpy(header, tmpHeader, sizeof(ImageHeader));

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Check reset vector of the current application firmware binary.
 * @param[in] slot Pointer to the slot that contains the application firmware
 * binary.
 * @return Error code.
 **/

cboot_error_t bootCheckSlotAppResetVector(Slot *slot)
{
   error_t error;
   cboot_error_t cerror;
   uint32_t resetVector;
   uint32_t resetVectorAddrOffset;

   Memory *memory;
   FlashDriver *driver;

   // Initialize status code
   cerror = CBOOT_NO_ERROR;

   // Check parameter validity
   if(slot == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   memory = (Memory *)slot->memParent;
   driver = (FlashDriver *)memory->driver;

   // Compute reset vector address offset (slot app start address offset + 4)
   resetVectorAddrOffset = mcuGetVtorOffset() + 0x4;

   // Check reset vector of the current application
   error = driver->read(slot->addr + resetVectorAddrOffset, (uint8_t *)&resetVector,
      sizeof(resetVector));

   // Check there is no error?
   if(!error)
   {
      // Is reset vector invalid (wrong value or outside of memory)?
      if((resetVector == 0xFFFFFFFF) ||
         !(slot->addr <= resetVector && resetVector <= slot->addr + slot->size))
      {
         // Raised an error
         cerror = CBOOT_ERROR_FAILURE;    // ERROR INVALID RESET VECTOR
      }
   }

   // Return status code
   return cerror;
}

/**
 * @brief Update Bootloader state
 * @param[in] context Pointer to the Bootloader context
 * @param[in] newState New state to switch to
 **/

void bootChangeState(BootContext *context, BootState newState)
{
   // Update Bootloader state
   context->state = newState;
   context->busy = TRUE;
}

/**
 * @brief Verify slot addresses to make sure no slot overlaps occur
 * @param[in] s1 Slot 1
 * @param[in] s2 Slot 2
 * @return TRUE if slots overlap, else FALSE
 **/

bool_t bootCheckNoSlotOverlap(Slot *s1, Slot *s2)
{
   uint32_t slot1_end = s1->addr + s1->size;
   uint32_t slot2_end = s2->addr + s2->size;

   // Check if the given two slot overlap
   if((s2->addr >= s1->addr && s2->addr < slot1_end) ||
      (s1->addr >= s2->addr && s1->addr < slot2_end))
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}
