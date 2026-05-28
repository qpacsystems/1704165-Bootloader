/**
 * @file boot_fallback.c
 * @brief CycloneBOOT Bootloader fallback managment
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
#include "boot_fallback.h"

#include "debug.h"
#include "image/image.h"
#include "second_stage/boot.h"
#include "second_stage/boot_common.h"

// CycloneBOOT Bootloader fallback private related functions
cboot_error_t fallbackFindSlotWithEquivImg(BootContext *context, Slot **slotEquivImg,
   Memory *memories);
cboot_error_t fallbackCompareSlots(Slot *slot1, Slot *slot2, int8_t *res);
cboot_error_t fallbackDeleteSlot(Slot *slot);
cboot_error_t fallbackRestoreBackupSlot(BootContext *context, Slot *slot);

/**
 * @brief Fallback Task routine
 * @param[in] context Pointer to the Bootloader context
 * @return Status Code
 **/

cboot_error_t fallbackTask(BootContext *context, Memory *memories)
{
   cboot_error_t cerror;
   Slot *slotEquivImg;
   Slot *slotBackupImg;
   Slot *appSlot;
   Slot *fallbackSlotOne;
   Slot *fallbackSlotTwo;
   int8_t res;

   ImageHeader fallbackSlotOneHdr;
   ImageHeader fallbackSlotTwoHdr;

   // Initialize variables
   cerror = CBOOT_NO_ERROR;
   slotEquivImg = NULL;
   slotBackupImg = NULL;
   appSlot = &memories[0].slots[0];
#if EXTERNAL_MEMORY_SUPPORT == ENABLED
   fallbackSlotOne = &memories[1].slots[0];
   fallbackSlotTwo = &memories[1].slots[1];
#else
   fallbackSlotOne = &memories[0].slots[1];
   fallbackSlotTwo = &memories[0].slots[2];
#endif

   // Beginning of handling block
   do
   {
      // Check the current app image in (internal flash slot)
      cerror = bootCheckImage(context, appSlot);
      // Is any error?
      if(cerror)
      {
         cerror = CBOOT_ERROR_FIRMWARE_CORRUPTED;

         // Get Indexes.
         int slotOneValid = 0;
         int slotTwoValid = 0;
         cerror = bootGetSlotImgHeader(fallbackSlotOne, &fallbackSlotOneHdr);
         if((cerror == CBOOT_NO_ERROR) &&
            (bootCheckImage(context, fallbackSlotOne) == CBOOT_NO_ERROR))
         {
            slotOneValid = 1;
         }
         cerror = bootGetSlotImgHeader(fallbackSlotTwo, &fallbackSlotTwoHdr);
         if((cerror == CBOOT_NO_ERROR) &&
            (bootCheckImage(context, fallbackSlotTwo) == CBOOT_NO_ERROR))
         {
            slotTwoValid = 1;
         }
         // Restore the one with the highest index (meaning it is the copy of the previous
         // firmware)
         if((slotOneValid == 0) && (slotTwoValid == 0))
         {
            cerror = CBOOT_ERROR_FIRMWARE_CORRUPTED;
         }
         else if(slotOneValid == 0)
         {
            TRACE_INFO("Chosen Slot Two with Image Index: %d\r\n", fallbackSlotTwoHdr.imgIndex);
            cerror = fallbackRestoreBackupSlot(context, fallbackSlotTwo);
         }
         else if(slotTwoValid == 0)
         {
            TRACE_INFO("Chosen Slot One with Image Index: %d\r\n", fallbackSlotOneHdr.imgIndex);
            cerror = fallbackRestoreBackupSlot(context, fallbackSlotOne);
         }
         else if(fallbackSlotOneHdr.imgIndex > fallbackSlotTwoHdr.imgIndex)
         {
            TRACE_INFO("Chosen Slot One with Image Index: %d\r\n", fallbackSlotOneHdr.imgIndex);
            cerror = fallbackRestoreBackupSlot(context, fallbackSlotOne);
         }
         else
         {
            TRACE_INFO("Chosen Slot Two with Image Index: %d\r\n", fallbackSlotTwoHdr.imgIndex);
            cerror = fallbackRestoreBackupSlot(context, fallbackSlotTwo);
         }

         // Is any error?
         if(cerror)
            break;
      }
      else
      {
         // Check update/backup image 1 (in fallback flash slot 1)
         cerror = bootCheckImage(context, fallbackSlotOne);
         // Is any error?
         if(cerror)
            break;

         // Check update/backup image 2 (in fallback flash slot 2)
         cerror = bootCheckImage(context, fallbackSlotTwo);
         // Is any error?
         if(cerror)
            break;

         // Find image in fallback slots that is equivalent to the current app image
         // in internal flash
         cerror = fallbackFindSlotWithEquivImg(context, &slotEquivImg, memories);
         // If any error or slot with equivalent image isn't found?
         if(cerror || slotEquivImg == NULL)
         {
            cerror = CBOOT_ERROR_ABORTED;
            break;
         }

         // Check that the other fallback slot contains an image (in the other
         // external flash slot) that has an index inferior ot the current image in
         // internal flash If it is not the case then goto error state

         // Select the remaining slot that should contain the backup image of the
         // previous valid application
         if(slotEquivImg == fallbackSlotOne)
         {
            slotBackupImg = fallbackSlotTwo;
         }
         else
         {
            slotBackupImg = fallbackSlotOne;
         }

         // Check that the remaining slot (containing the backup image) hold an image
         // older that the current
         //  image in the primary flash slot. If it is not the case, then there is no
         //  backup image to perform a fallback
         cerror = fallbackCompareSlots(slotBackupImg, appSlot, &res);

         if(cerror || res >= 0)
         {
            cerror = CBOOT_ERROR_ABORTED;
            break;
         }

         // Delete the fallback slot that contains the image equivalent of the
         // current app image
         cerror = fallbackDeleteSlot(slotEquivImg);
         // Is any error?
         if(cerror)
            break;

         // Restore the image in the remaining fallback slot (backup of the previous
         // valid app)
         cerror = fallbackRestoreBackupSlot(context, slotBackupImg);
         // Is any error?
         if(cerror)
            break;
      }
   } while(0);

   // Return status code
   return cerror;
}

/**
 * @brief Delete the given slot. In other words it erase the content of the
 * given slot.
 * @param[in] slot Pointer to the slot to be deleted.
 * @return Error code.
 **/

cboot_error_t fallbackDeleteSlot(Slot *slot)
{
   error_t error;
   Memory *memory;
   FlashDriver *flashDrv;

   // Point to the slot flash driver
   memory = (Memory *)slot->memParent;
   flashDrv = (FlashDriver *)memory->driver;

   // Erase slot data
   error = flashDrv->erase(slot->addr, slot->size);
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

/**
 * @brief Restore the image contained in the backup slot.
 * It will extract the firmware application from the image inside the backup
 * slot. Then a new image that old the back application firmware will be
 * generated in internal memory slot. It will allow the system to boot to the
 * backup application firmware.
 * @param[in] slot Pointer to the slot to be deleted.
 * @return Error code.
 **/

cboot_error_t fallbackRestoreBackupSlot(BootContext *context, Slot *slot)
{
   // Restore the application firmware backup by updating the system using
   // the slot containing the backup image that hold the application firmware
   // backup.
   return bootUpdateApp(context, slot);
}

/**
 * @brief Compare two given slots together with respect to the index
 * of the images inside the slots.
 * - If the image index of the first slot (slot1) is strictly inferior
 *   to the image index of the first slot (slot1) then result will be -1.
 * - If the image index of the first slot (slot1) is equal
 *   to the image index of the first slot (slot1) then result will be 0.
 * - Otherwise the result will be 1.
 * @param[in] slot1 Pointer to the first slot to be compared with.
 * @param[in] slot2 Pointer to the second slot to be compared with.
 * @param[ou] res Result of the slot comparison.
 * @return Error code.
 **/

cboot_error_t fallbackCompareSlots(Slot *slot1, Slot *slot2, int8_t *res)
{
   cboot_error_t cerror;
   uint32_t saveImgIdx;
   ImageHeader imgHeader;

   // Initialize status code
   cerror = CBOOT_NO_ERROR;

   // Check parameters validity
   if(slot1 == NULL || slot2 == NULL || res == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Get image header from the first slot.
   cerror = bootGetSlotImgHeader(slot1, &imgHeader);
   // Check there is no error
   if(!cerror)
   {
      // Save index of the image inside the first slot for
      //  later comparison.
      saveImgIdx = imgHeader.imgIndex;

      // Get image header from the second slot.
      cerror = bootGetSlotImgHeader(slot2, &imgHeader);
      // Check there is no error
      if(!cerror)
      {
         // Is the image index from the first slot strictly
         //  inferior to the image index from the second slot?
         if(saveImgIdx < imgHeader.imgIndex)
         {
            *res = -1;
         }
         // Is image index from the first slot equal the image index from the
         // second slot?
         else if(saveImgIdx == imgHeader.imgIndex)
         {
            *res = 0;
         }
         // Is the image index from the first slot strictly
         //  superior to the image index from the second slot?
         else
         {
            *res = 1;
         }
      }
   }

   // Return state code
   return cerror;
}

/**
 * @brief Search for the fallback slot that contains the image equivalent
 * of the current image in internal flash slot. "Equivalent" means that the
 * image to be found will have the same index than the current image in internal
 * flash.
 * @param[in] context Pointer to bootloader context.
 * @param[out] slotEquivImg Pointer to the slot that holding the equivalent
 * image.
 * @return Error code.
 **/

cboot_error_t fallbackFindSlotWithEquivImg(BootContext *context, Slot **slotEquivImg,
   Memory *memories)
{
   cboot_error_t cerror;
   uint_t i;
   uint32_t currImgIdx;
   ImageHeader imgHeader;
   bool_t foundSlot;
   Slot appSlot;

   cerror = CBOOT_NO_ERROR;
   foundSlot = FALSE;

   appSlot = memories[0].slots[0];
   appSlot.memParent = &memories[0];

   // Check parameters validity
   if(context == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Get image header from the internal flash memory slot.
   cerror = bootGetSlotImgHeader(&appSlot, &imgHeader);
   // Check there is no error
   if(!cerror)
   {
      // Save index of the current image in internal flash
      currImgIdx = imgHeader.imgIndex;

#if EXTERNAL_MEMORY_SUPPORT == ENABLED
      // Loop through the external flash slot list
      for(i = 0; i < memories[1].nbSlots; i++)
      {

         // Get image header from the listed slot
         cerror = bootGetSlotImgHeader(&memories[1].slots[i], &imgHeader);
#else
      // Loop through the internal flash slot list
      for(i = 1; i < memories[0].nbSlots; i++)
      {
         cerror = bootGetSlotImgHeader(&memories[0].slots[i], &imgHeader);
#endif
         // Check there is no error
         if(cerror)
         {
            break;
         }
         else
         {
            // Is the index of the current image being equal to the index of the
            // image from the listed slot?
            if(imgHeader.imgIndex == currImgIdx)
            {
               // Equivalent slot is found.
               foundSlot = TRUE;
               // Saving equivalent slot pointer.
#if EXTERNAL_MEMORY_SUPPORT == ENABLED
               *slotEquivImg = &memories[1].slots[i];
#else
               *slotEquivImg = &memories[0].slots[i];
#endif
               break;
            }
         }
      }

      // Is the equivalent slot not found?
      if(!foundSlot)
      {
         // Raise an error
         cerror = CBOOT_ERROR_FAILURE;
      }
   }

   // Return status code
   return cerror;
}

#if ((defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) || defined(__GNUC__) ||            \
   defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__) || defined(__TASKING__) ||                  \
   defined(__CWCC__) || defined(__TI_ARM__))

/**
 * @brief Fallback Trigger Initialisation callback.
 * It is declared week and must be declared in user codde space.
 * @return Status Code
 **/

__weak_func cboot_error_t fallbackTriggerInit(void)
{
   // This function Should not be modified when the callback is needed,
   //  the fallbackTriggerInit must be implemented in the user file

   return CBOOT_NO_ERROR;
}

/**
 * @brief Fallback Trigger Status callback.
 * It is declared week and must be declared in user codde space.
 * @param[in] status Trigger status to be returned (Raised or Idle)
 * @return Status Code
 **/

__weak_func cboot_error_t fallbackTriggerGetStatus(TriggerStatus *status)
{
   // This function Should not be modified when the callback is needed,
   // the fallbackTriggerGetStatus must be implemented in the user file

   return CBOOT_NO_ERROR;
}
#endif
