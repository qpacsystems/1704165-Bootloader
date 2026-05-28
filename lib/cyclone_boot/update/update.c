/**
 * @file update.c
 * @brief CycloneBOOT IAP User API
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
#include "update/update.h"

#include "core/crc32.h"
#include "core/flash.h"
#include "image/image.h"
#include "image/image_process.h"
#include "memory/memory.h"
#include "update/update_misc.h"

#include <stdlib.h>
#if ((UPDATE_SINGLE_BANK_SUPPORT == ENABLED) &&                                                    \
   ((CIPHER_SUPPORT == ENABLED) && (IMAGE_OUTPUT_ENCRYPTED == ENABLED)) &&                       \
   (UPDATE_FALLBACK_SUPPORT == DISABLED))
#include "core/mailbox.h"
#endif
#include "boot_config.h"
#include "debug.h"

// Random data generation callback function
IapRandCallback updateRandCallback;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Update image anti-rollback callback prototype
bool_t updateAcceptUpdateImageCallback(ImageVersion currentAppVersion, ImageVersion updateAppVersion);

// Image Index related private functions
cboot_error_t updateCalculateOutputImageIdx(UpdateContext *context, uint16_t *imgIdx);
cboot_error_t updateGetUpdateSlot(UpdateContext *context, Slot **slot);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief This function returns the current version of CycloneBOOT IAP
 * @return String that hold CycloneBOOT version.
 **/

char_t *updateGetVersion(void) {
   return CYCLONE_BOOT_UPDATE_VERSION_STRING;
}

/**
 * @brief Initialize settings with default values
 * @param[in,out] settings Structure that contains IAP settings
 **/

void updateGetDefaultSettings(UpdateSettings *settings)
{
   // Clear image output crypto settings structure
   memset(settings, 0x00, sizeof(UpdateSettings));

#if (VERIFY_INTEGRITY_SUPPORT == ENABLED || UPDATE_SINGLE_BANK_SUPPORT == ENABLED)
   // IAP integrity setting
   settings->imageInCrypto.verifySettings.integrityAlgo = NULL;
#endif

#if VERIFY_AUTHENTICATION_SUPPORT == ENABLED
   // IAP authentication settings
   settings->imageInCrypto.verifySettings.authAlgo = VERIFY_AUTH_NONE;
   settings->imageInCrypto.verifySettings.authHashAlgo = NULL;
   settings->imageInCrypto.verifySettings.authKey = NULL;
   settings->imageInCrypto.verifySettings.authKeyLen = 0;
#endif

#if VERIFY_SIGNATURE_SUPPORT == ENABLED
   // IAP signature settings
   settings->imageInCrypto.verifySettings.signAlgo = VERIFY_SIGN_NONE;
   settings->imageInCrypto.verifySettings.signHashAlgo = NULL;
   settings->imageInCrypto.verifySettings.signKey = NULL;
   settings->imageInCrypto.verifySettings.signKeyLen = 0;
#endif

#if ((CIPHER_SUPPORT == ENABLED) &&                                                                \
   ((IMAGE_INPUT_ENCRYPTED == ENABLED) || (IMAGE_OUTPUT_ENCRYPTED == ENABLED)))
   // IAP cipher settings
   settings->imageInCrypto.cipherAlgo = NULL;
   settings->imageInCrypto.cipherMode = CIPHER_MODE_NULL;
   settings->imageInCrypto.cipherKey = NULL;
   settings->imageInCrypto.cipherKeyLen = 0;
#endif
}

/**
 * @brief Register RNG callback function
 * @param[in] callback RNG callback function
 * @return Error code
 **/

cboot_error_t updateRegisterRandCallback(IapRandCallback callback)
{
   // Check parameter
   if(callback == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Save callback function
   updateRandCallback = callback;

   // Successful processing
   return CBOOT_NO_ERROR;
}

/**
 * @brief Initialize IAP Application context
 * @param[in,out] context Pointer to the IAP Application context to be
 * initialized
 * @return Status code
 **/

cboot_error_t updateInit(UpdateContext *context, UpdateSettings *settings)
{
   cboot_error_t cerror;
#if ((UPDATE_SINGLE_BANK_SUPPORT == ENABLED))
   uint16_t newImgIdx;
#endif

   // Check Parameters validity
   if(context == NULL || settings == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Debug message
   TRACE_INFO("Initializing IAP...\r\n");

   // Clear the Update context
   memset(context, 0, sizeof(UpdateContext));

   // Save user settings
   context->settings = *settings;

   // Initialize memories
   cerror = memoryInit(context->settings.memories, NB_MEMORIES);
   // Is any error?
   if(cerror)
   {
      // Debug message
      TRACE_ERROR("Memory initialization failed!\r\n");
      return cerror;
   }

   context->memories[0] = settings->memories[0];
#if (UPDATE_SINGLE_BANK_SUPPORT == ENABLED && EXTERNAL_MEMORY_SUPPORT == ENABLED)
   context->memories[1] = settings->memories[1];
#endif

   // Link memories to the image process context
   // context->imageProcessCtx.memories = context->memories;
   context->imageProcessCtx.memories = context->settings.memories;

#if (UPDATE_ANTI_ROLLBACK_SUPPORT == ENABLED)
   // Set anti-rollback callback
   context->imageProcessCtx.imgAntiRollbackCallback = updateAcceptUpdateImageCallback;
   // Set current application version
   context->imageProcessCtx.currentAppVersion = settings->appVersion;
#else
   // Clear anti-rollback callback
   context->imageProcessCtx.imgAntiRollbackCallback = NULL;
#endif

   // Initialize image input context (will process receive update image)
   cerror = updateInitInputImage(&context->settings, context);
   // Is any error?
   if(cerror)
      return cerror;

   // Initialize image output context (will process the output binary or image)
   cerror = updateInitOutputImage(&context->settings, context);
   // Is any error?
   if(cerror)
      return cerror;

#if (UPDATE_SINGLE_BANK_SUPPORT == ENABLED)
   // Set index of output image
   cerror = updateCalculateOutputImageIdx(context, &newImgIdx);
   // Is any error?
   if(cerror)
      return CBOOT_ERROR_FAILURE;

   // context->imageOutput.imgIdx = newImgIdx;
   context->imageProcessCtx.outputImage.newImageIdx = newImgIdx;
#endif

   // Get slot to store output update image
   cerror = updateGetUpdateSlot(context, &context->imageProcessCtx.outputImage.activeSlot);
   // Is any error?
   if(cerror)
      return CBOOT_ERROR_FAILURE;

#if (UPDATE_SINGLE_BANK_SUPPORT == ENABLED)
   // Make sure the output slot type isn't binary
   context->imageProcessCtx.outputImage.activeSlot->cType &= ~SLOT_CONTENT_BINARY;
#else
   // Make sure to specify output slot type as binary
   context->imageProcessCtx.outputImage.activeSlot->cType |= SLOT_CONTENT_BINARY;
#endif

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Write receive firmware in the unused flash bank.
 * @param[in,out] context Pointer to the IAP application context
 * @param[in] data Firmware chunck of data to be written in flash bank
 * @param[in] length Length of the firmware chunck of data to be written
 * @return Status code
 **/

cboot_error_t updateProcess(UpdateContext *context, const void *data, size_t length)
{
   cboot_error_t cerror;
   uint_t n;
   uint8_t *pData;
   Image *inputImage;

   // Check parameters validity
   if(context == NULL || data == NULL || length == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Initialize variables
   n = 0;

   // Point to input image handler
   inputImage = &context->imageProcessCtx.inputImage;

   // Point to the beginning of the data
   pData = (uint8_t *)data;

   // Process the incoming data
   while(length > 0)
   {
      // Still room in buffer?
      if(inputImage->bufferLen < sizeof(inputImage->buffer))
      {
         // Fill buffer with input data
         n = MIN(length, sizeof(inputImage->buffer) - inputImage->bufferLen);
         memcpy(inputImage->bufferPos, pData, n);

         // Update buffer position and length
         inputImage->bufferPos += n;
         inputImage->bufferLen += n;

         // Update input data position and length
         pData += n;
         length -= n;

         // Process received image input data
         cerror = imageProcessInputImage(&context->imageProcessCtx);
         // Is any error?
         if(cerror)
         {
#if (UPDATE_SINGLE_BANK_SUPPORT == ENABLED)
            // Erase output image slot first bytes to make sure bootloader doesn't
            // consider it as a new valid update image if a reboot occurs
            // context->secondaryMem.driver->erase(
            //    context->imageOutput.slotInfo->addr, sizeof(ImageHeader));
            cerror = memoryEraseSlot(context->imageProcessCtx.outputImage.activeSlot, 0,
               sizeof(ImageHeader));
#endif
            // Is any error?
            if(cerror)
            {
               return cerror;
            }
         }
      }
      else
      {
         // Debug message
         TRACE_ERROR("Buffer would overflow!\r\n");
         return CBOOT_ERROR_BUFFER_OVERFLOW;
      }
   }

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Finalize firmware update. It performs :
 *    - Firmware integrity or authentification or signature validation.
 *    - Flash bank memory swap setup
 * If validation is done successfully the device would be ready for reboot,
 * if not an error will be raised.
 * The firmware validation can only be done if all the image
 * (header, firmware data and check data) has been processed successfully, if
 * not an error will be raised.
 * @param[in,out] context Pointer to the IAP application context
 * @return Error code
 **/

cboot_error_t updateFinalize(UpdateContext *context)
{
   cboot_error_t cerror;
   Image *imageIn;
#if (UPDATE_SINGLE_BANK_SUPPORT == ENABLED)
   Image *imageOut;
#else
   Memory *primaryMemory;
   MemoryInfo memInfo;
#endif
#if ((UPDATE_SINGLE_BANK_SUPPORT == ENABLED) &&                                                    \
   ((CIPHER_SUPPORT == ENABLED) && (IMAGE_OUTPUT_ENCRYPTED == ENABLED)) &&                       \
   (UPDATE_FALLBACK_SUPPORT == DISABLED))
   BootMailBox bMsg;
#endif
#if ((CIPHER_SUPPORT == ENABLED) && (IMAGE_INPUT_ENCRYPTED == ENABLED))
   bool_t magicNumberIsValid;
#endif

   // Check parameters validity
   if(context == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Debug message
   TRACE_INFO("Finalizing firmware update...\r\n");

   // Point to the image input context
   imageIn = (Image *)&context->imageProcessCtx.inputImage;
#if (UPDATE_SINGLE_BANK_SUPPORT == ENABLED)
   // Point to the image output context
   imageOut = (Image *)&context->imageProcessCtx.outputImage;
#endif

   // Ready to verify firmware image validity?
   if(imageIn->state == IMAGE_STATE_VALIDATE_APP)
   {
      // Verify firmware image validity (could integrity tag or
      // authentification tag or signature)
      cerror = verifyConfirm(&imageIn->verifyContext, imageIn->checkData, imageIn->checkDataLen);
      // Is any error?
      if(cerror)
      {
         // Debug message
         TRACE_ERROR("Firmware image is invalid!\r\n");

#if (UPDATE_SINGLE_BANK_SUPPORT == ENABLED)
         // Erase output image slot first bytes to make sure bootloader doesn't
         // consider it as a new valid update image if a reboot occurs
         memoryEraseSlot(imageOut->activeSlot, 0, sizeof(ImageHeader));
#endif

         // Return to IAP idle state
         imageIn->state = IMAGE_STATE_IDLE;
         // Return error code
         return CBOOT_ERROR_INVALID_IMAGE_APP;
      }
      else
      {
#if ((CIPHER_SUPPORT == ENABLED) && (IMAGE_INPUT_ENCRYPTED == ENABLED))
         // Check given magic number crc
         //- If it is valid it means that the cipher key provided by the user for
         // decryption is the same as the one used to encrypt update image
         //- Else it means that the cipher key provided by the user doesn't match
         // the one used to encrypt the update image and
         //   that the decrypted image data will be wrong. In that case the update
         //   MUST be invalidate.
         cerror = cipherCheckMagicNumberCrc(imageIn->magicNumberCrc, &magicNumberIsValid);
         if(cerror || !magicNumberIsValid)
         {
            // Debug message
            TRACE_ERROR("Firmware image is valid but cipher key used is invalid!\r\n");

#if (UPDATE_SINGLE_BANK_SUPPORT == ENABLED)
            // Erase output image slot first bytes to make sure bootloader doesn't
            // consider it as a new valid update image if a reboot occurs
            memoryEraseSlot(imageOut->activeSlot, 0, sizeof(ImageHeader));
#endif

            // Return to IAP idle state
            imageIn->state = IMAGE_STATE_IDLE;
            // Return error code
            return CBOOT_ERROR_INVALID_IMAGE_APP;
         }
         else
#endif
         {
            // Debug message
            TRACE_INFO("Firmware image is valid\r\n");
         }
      }

#if (UPDATE_SINGLE_BANK_SUPPORT == ENABLED)
#if (((CIPHER_SUPPORT == ENABLED) && (IMAGE_OUTPUT_ENCRYPTED == ENABLED)) &&                       \
      (UPDATE_FALLBACK_SUPPORT == DISABLED))
      // Setup bootloader mailbox to share PSK key used to encrypt output image in
      // external flash memory
      mailBoxInit();

      // Debug message
      TRACE_DEBUG("BootMailBox:\r\n");
      TRACE_DEBUG_ARRAY("- psk: ", imageOut->cipherEngine.key, imageOut->cipherEngine.keyLen);
      TRACE_DEBUG("- psk size: %d\r\n", imageOut->cipherEngine.keyLen);

      // Send PSK key to the Bootloader (SRAM)
      mailBoxSetPsk(imageOut->cipherEngine.key, imageOut->cipherEngine.keyLen);
#endif
#else
      // Get primary memory
      cerror = memoryGetMemoryByRole(context->settings.memories, NB_MEMORIES, MEMORY_ROLE_PRIMARY,
         &primaryMemory);
      // Is any error?
      if(cerror)
         return cerror;

      // Get memory info
      cerror = memoryGetInfo(primaryMemory, &memInfo);
      // Is any error?
      if(cerror)
         return cerror;

      // Check swap action is done later (after reset)
      if(memInfo.flags & FLASH_FLAGS_LATER_SWAP)
      {
         // Swap banks
         cerror = memoryExSwapBanks(primaryMemory);
         // Is any error?
         if(cerror)
            return cerror;
      }
#endif
      // Change IAP state
      imageIn->state = IMAGE_STATE_APP_REBOOT;
   }
   else
   {
      // Debug message
      TRACE_ERROR("Firmware image is not ready for verification!\r\n");

#if (UPDATE_SINGLE_BANK_SUPPORT == ENABLED)
      // Erase output image slot first bytes to make sure bootloader doesn't
      // consider it as a new valid update image if a reboot occurs
      memoryEraseSlot(imageOut->activeSlot, 0, sizeof(ImageHeader));
#endif
      // Return error code
      return CBOOT_ERROR_IMAGE_NOT_READY;
   }

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Reboot the device by triggering a system reset.
 * This reboot can only be performed if firmware validation has been done
 * succesfully. If not an error will be raised.
 * @param[in] context Pointer to the IAP context
 * @return Error code.
 **/

cboot_error_t updateReboot(UpdateContext *context)
{
   cboot_error_t cerror;
   MemoryInfo memInfo;
   Memory *primaryMemory;

   // Debug message
   TRACE_INFO("Rebooting device to finalize update...\r\n");

   // Check parameter validity
   if(context == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Point to the primary memory
   primaryMemory = (Memory *)&context->memories[0];

   if(context->imageProcessCtx.inputImage.state == IMAGE_STATE_APP_REBOOT)
   {
      // Get flash info
      cerror = memoryGetInfo(primaryMemory, &memInfo);
      // Is any error?
      if(cerror)
         return cerror;

      // Call clean up function
      updateResetMcuHook();

#if (UPDATE_SINGLE_BANK_SUPPORT == ENABLED)
      // Reboot system to complete firmware update procedure
      mcuSystemReset();
#else
      // Is swap action done later (after reset)?
      if(memInfo.flags & FLASH_FLAGS_LATER_SWAP)
      {
         // Reboot system to complete firmware update procedure
         mcuSystemReset();
      }
      else
      {
         // Swap banks
         cerror = memoryExSwapBanks(primaryMemory);
         // Is any error?
         if(cerror)
            return cerror;
      }
#endif
   }
   else
   {
      // Wrong state
      return CBOOT_ERROR_INVALID_STATE;
   }

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Confirm firmware update. It performs :
 *    - Firmware integrity or authentification or signature validation.
 *    - Flash bank memory swap setup
 * If validation is done successfully the device would be ready for reboot,
 * if not an error will be raised.
 * The firmware validation can only be done if all the image
 * (header, firmware data and check data) has been processed successfully, if
 * not an error will be raised.
 * @param[in,out] context Pointer to the IAP application context
 * @return Error code
 **/

cboot_error_t updateConfirm(void) {
   return CBOOT_ERROR_NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if (UPDATE_ANTI_ROLLBACK_SUPPORT == ENABLED)
/**
 * @brief This callback checks the version of firmware application inside the
 * received update image. If the version of the firmware application in the
 * update image is less than or equal to the version of the current firmware
 * application then the update will be discarded. If not the update image is
 * accepted. This function is used only the UPDATE_ANTI_ROLLBACK_SUPPORT feature
 * is activated.
 * @param[in] currentAppVersion Version of the current firmware application.
 * @param[in] updateAppVersion Version of the firmware application inside the
 * update image.
 * @return TRUE if the update image is accepted, FALSE otherwise.
 **/

bool_t updateAcceptUpdateImageCallback(ImageVersion currentAppVersion, ImageVersion updateAppVersion)
{
   // Return TRUE only if the update version is strictly greater than the current version
   if(updateAppVersion.major > currentAppVersion.major)
      return TRUE;
   if(updateAppVersion.major < currentAppVersion.major)
      return FALSE;

   if(updateAppVersion.minor > currentAppVersion.minor)
      return TRUE;
   if(updateAppVersion.minor < currentAppVersion.minor)
      return FALSE;

   if(updateAppVersion.revision > currentAppVersion.revision)
      return TRUE;
   if(updateAppVersion.revision < currentAppVersion.revision)
      return FALSE;

   if(updateAppVersion.buildNum > currentAppVersion.buildNum)
      return TRUE;

   return FALSE;  // Versions are equal
}

#endif

/**
 * @brief This function calculates the value of the output image index that will
 * be store in external memory. It first retrieve the value of the image that
 * contains the current application in internal memory, then it increment this
 * value by one and save it in the given output index pointer. This function is
 * used only when the UPDATE_SINGLE_BANK_MODE_SUPPORT is activated. In Single
 * Bank mode the update image is processed to check its validity while a new
 * ouput image is generated and stored in external memory. This output image will
 * then be processed by the bootloader to complete the update procedure. The
 * bootloader uses the index of the output image againts the index of the image
 * containing the current application firmware to determine if an update is
 * neeeded or not.
 * @param[in] context Pointer to the IAP context.
 * @param[out] imgIdx Calculated value of the output image index.
 * @return Error code.
 **/

cboot_error_t updateCalculateOutputImageIdx(UpdateContext *context, uint16_t *imgIdx)
{
   cboot_error_t cerror;
   ImageHeader imgHeader;
   Slot *appSlot;
   uint32_t curIdx;

   // Check paramter validity
   if(context == NULL || imgIdx == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Get primary memory slot that holds app
   cerror = memoryGetSlotByCType(&context->memories[0], SLOT_CONTENT_APP, &appSlot);
   // Is any error?
   if(cerror)
      return cerror;

   // Get image header from primary flash memory first slot
   cerror = updateGetImageHeaderFromSlot(appSlot, &imgHeader);

#if (UPDATE_STANDALONE_BOOT_MODE == ENABLED)
   if(cerror == CBOOT_ERROR_SLOT_EMPTY)
      cerror = CBOOT_NO_ERROR;
#endif

   // Is any error?
   if(cerror)
      return cerror;

   // Get image index from header
   curIdx = imgHeader.imgIndex;
   // Save incremented image index
   *imgIdx = ++curIdx;

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief This function selects the slot that will hold the output image. The
 * choice of the slot depends of the IAP mode: Dual or Single bank mode. In Dual
 * bank mode:
 * - the output image is the new application firmware binary that was in the
 * update image.
 * - the slected slot is always the other flash bank (the one that doesn't hold
 * the current application binary) In Single Bank mode:
 * - the output image is an image stored in external memory, containing the new
 * application binary comming from the update image and generating in a way that
 * the bootloader will be abled to process it.
 * - the selected slot is one of the available slot in external memory. It can
 * be:
 *     - if fallback support is not activated, the first and only one slot in
 * external memory
 *     - otherwise one of the slot among the two slots present in external
 * memory that doesn't hold the backup image of the current running application
 * @param[in] context Pointer to IAP context.
 * @param[out] slot Pointer to the slot that will be used to hold output image.
 * @return
 **/

cboot_error_t updateGetUpdateSlot(UpdateContext *context, Slot **slot)
{
#if (UPDATE_SINGLE_BANK_SUPPORT == ENABLED && UPDATE_FALLBACK_SUPPORT == ENABLED)
   cboot_error_t cerror;
   uint16_t imgIndex;
   Slot *tempSlot;
   ImageHeader header;
#endif

   // Check parameters validity
   if(context == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Dual bank mode
#if (UPDATE_DUAL_BANK_SUPPORT == ENABLED)
   // Select the second slot of primary flash memory that match the second flash
   // bank
   *slot = (Slot *)&context->settings.memories[0].slots[1];

// Single bank mode
#else

#if (UPDATE_STANDALONE_BOOT_MODE == ENABLED)
   *slot = &context->settings.memories[0].slots[0];
#else

// Fallback decativated
#if (UPDATE_FALLBACK_SUPPORT == DISABLED)
   // In single bank mode without fallback support only one
#if (EXTERNAL_MEMORY_SUPPORT == ENABLED)
   *slot = (Slot *)&context->settings.memories[1].slots[0];
#else
   *slot = (Slot *)&context->settings.memories[0].slots[1];
#endif

// Fallback activated
#else
   // Point to the primary flash memory slot
   tempSlot = (Slot *)&context->settings.memories[0].slots[0];

   // Get header from primary flash memory slot image
   cerror = updateGetImageHeaderFromSlot(tempSlot, &header);
   // Is any error?
   if(cerror)
   {
      return cerror;
   }
   else
   {
      // Save image index of the primary flash memory slot image
      imgIndex = header.imgIndex;

      // Point to the first secondary flash memory slot
#if EXTERNAL_MEMORY_SUPPORT == ENABLED
      tempSlot = (Slot *)&context->settings.memories[1].slots[0];
#else
      tempSlot = (Slot *)&context->settings.memories[0].slots[1];
#endif

      // Get header from the first secondary flash memory slot image
      cerror = updateGetImageHeaderFromSlot(tempSlot, &header);
      // Is any error?
      if(cerror && cerror != CBOOT_ERROR_INVALID_IMAGE_HEADER)
      {
         return cerror;
      }

      // We MUST select the slot from secondary flash memory driver which doesn't
      // store the backup image
      //  of the current running application. Which means that :
      //  - the image index of selected secondary slot is different from the
      //  primary flash memory slot image index
      //     or
      //  - the selected secondary slot doesn't contains a valid image (slot is
      //  empty or header is invalid)
      if(cerror == CBOOT_ERROR_INVALID_IMAGE_HEADER || imgIndex != header.imgIndex)
      {
         // Select first secondary flash memory slot
         //*slot = (Slot*)&context->secondaryMem.slots[0];
#if EXTERNAL_MEMORY_SUPPORT == ENABLED
         *slot = (Slot *)&context->settings.memories[1].slots[0];
#else
         *slot = (Slot *)&context->settings.memories[0].slots[1];
#endif
      }
      else
      {
         // Select second secondary flash memory slot
         //*slot = (Slot*)&context->secondaryMem.slots[1];
#if EXTERNAL_MEMORY_SUPPORT == ENABLED
         *slot = (Slot *)&context->settings.memories[1].slots[1];
#else
         *slot = (Slot *)&context->settings.memories[0].slots[2];
#endif
      }
   }
#endif
#endif

#endif
   // Successful process
   return CBOOT_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ((defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) || defined(__GNUC__) ||            \
   defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__) || defined(__TASKING__) ||                  \
   defined(__CWCC__) || defined(__TI_ARM__))
__weak_func void updateResetMcuHook(void)
{
   /*
    * This function will be called when the bootloader is initialized.
    * It can be redefined in user code.
    */
}
#endif