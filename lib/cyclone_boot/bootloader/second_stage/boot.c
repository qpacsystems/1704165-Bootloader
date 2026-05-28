/**
 * @file boot.c
 * @brief CycloneBOOT 2nd Stage Bootloader management
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
#define TRACE_LEVEL TRACE_LEVEL_INFO

// Dependencies
#include "second_stage/boot.h"

#include "core/flash.h"
#include "image/image.h"
#include "second_stage/boot_common.h"
#include "second_stage/boot_fallback.h"
#if ((BOOT_FALLBACK_SUPPORT == DISABLED) && (BOOT_EXT_MEM_ENCRYPTION_SUPPORT == ENABLED) ||        \
   (BOOT_COUNTER_SUPPORT == ENABLED))
#include "core/mailbox.h"
#endif
#include "core/cboot_error.h"
#include "core/mailbox.h"
#include "debug.h"

// Bootloader private-related functions
cboot_error_t bootGetCipherKey(BootContext *context);
cboot_error_t bootJumpToApp(BootContext *context);

// State-handling functions
static cboot_error_t handleFallbackTriggers(BootContext *context);
static cboot_error_t handleIdleState(BootContext *context);
static cboot_error_t handleRunAppState(BootContext *context);
static cboot_error_t handleUpdateAppState(BootContext *context);

static void handleImageCheckError(BootContext *context, cboot_error_t cerror);
static bool_t isValidPskSize(size_t size);

#if (BOOT_FALLBACK_SUPPORT == ENABLED)
static cboot_error_t handleFallbackAppState(BootContext *context);
static cboot_error_t checkAutoFallbackTrigger(BootContext *context);
static cboot_error_t checkManualFallbackTrigger(BootContext *context);
#endif
/**
 * @brief Initialize bootloader settings with default values
 * @param[in,out] settings Structure that contains Bootloader settings
 **/

void bootGetDefaultSettings(BootSettings *settings)
{
   if(settings == NULL)
      return;

   // Clear bootloader user settings structure
   memset(settings, 0x00, sizeof(BootSettings));
}

/**
 * @brief Initialize bootloader context
 * @param[in,out] context Bootloader context
 * @param[in] settings Bootloader user settings
 * @return Status code
 **/

cboot_error_t bootInit(BootContext *context, BootSettings *settings)
{
   cboot_error_t cerror;

   // Check parameter validity
   if(context == NULL || settings == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Set context fields to zero
   memset(context, 0, sizeof(BootContext));

   // Save bootloader user settings
   memcpy(&context->settings, settings, sizeof(BootSettings));

   // User defined initialization hook
   bootInitHook();

   // Initialize a primary flash driver and slots
   cerror = bootInitPrimaryMem(context, settings);
   // Is any error?
   if(cerror)
      return cerror;

#if (BOOT_FALLBACK_SUPPORT == ENABLED && BOOT_FALLBACK_AUTO_MODE == ENABLED)
   mailBoxInit();
#endif

#if (EXTERNAL_MEMORY_SUPPORT == ENABLED)
   // Initialize a secondary (external) flash driver and slots
   cerror = bootInitSecondaryMem(context, settings);
   // Is any error?
   if(cerror)
      return cerror;

#if (BOOT_EXT_MEM_ENCRYPTION_SUPPORT == ENABLED)
   // Check the cipher key used to decode data in secondary flash (external
   // memory)
   if(settings->psk == NULL || settings->pskSize == 0 || (settings->pskSize > sizeof(context->psk)))
   {
      return CBOOT_ERROR_INVALID_PARAMETERS;
   }
   else
   {
      // Store the cipher key used to decode data in secondary flash (external
      // memory)
      memcpy(context->psk, settings->psk, settings->pskSize);
      context->pskSize = settings->pskSize;
   }
#endif
#endif

#if (BOOT_FALLBACK_SUPPORT == ENABLED && BOOT_FALLBACK_MANUAL_MODE == ENABLED)
   // Initialize fallback trigger
   cerror = fallbackTriggerInit();
   // Is any error?
   if(cerror)
      return cerror;
#endif

   // Initialize bootloader state
   context->state = BOOT_STATE_IDLE;

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Bootloader Task routine
 * @param[in] context Pointer to Bootloader context
 * @return cboot_error_t
 **/

cboot_error_t bootFsm(BootContext *context)
{
   cboot_error_t cerror = CBOOT_NO_ERROR;

#if (BOOT_FALLBACK_SUPPORT == ENABLED)
   // Handle fallback trigger if supported
   // TODO: consider whether this should only run in IDLE or guarded with a flag to avoid retriggers
   cerror = handleFallbackTriggers(context);
   if(cerror != CBOOT_NO_ERROR)
   {
      return cerror;
   }
#endif

   do
   {
      // TODO: If a handler sets busy = TRUE and returns CBOOT_NO_ERROR, the next iteration starts with a clean cerror.
      // But if a handler sets busy = TRUE and returns an error, the loop continues
      context->busy = FALSE;

      switch(context->state)
      {
      case BOOT_STATE_IDLE:
         cerror = handleIdleState(context);
         break;

      case BOOT_STATE_RUN_APP:
         cerror = handleRunAppState(context);
         break;

      case BOOT_STATE_UPDATE_APP:
         cerror = handleUpdateAppState(context);
         break;

#if (BOOT_FALLBACK_SUPPORT == ENABLED)
      case BOOT_STATE_FALLBACK_APP:
         cerror = handleFallbackAppState(context);
         break;
#endif

      case BOOT_STATE_ERROR:
         bootHandleGenericError();
         cerror = CBOOT_ERROR_FAILURE;
         break;

      default:
         cerror = CBOOT_ERROR_INVALID_STATE;
         break;
      }

   } while(context->busy);

   return cerror;
}

/**
 * @brief Get PSK cipher key used to encrypt output image in external flash
 * memory.
 * @param[in,out] context Pointer to the bootloader context
 **/

cboot_error_t bootGetCipherKey(BootContext *context)
{
#if ((BOOT_FALLBACK_SUPPORT == DISABLED) && (BOOT_EXT_MEM_ENCRYPTION_SUPPORT == ENABLED))

   // Debug message
   TRACE_INFO("Retrieving cipher key...\r\n");

   bool_t hasMailboxKey = (context->settings.psk == NULL && context->settings.pskSize == 0);
   bool_t hasSettingsKey = (context->settings.psk != NULL && context->settings.pskSize != 0);

   if(!hasMailboxKey && !hasSettingsKey)
   {
      TRACE_ERROR("Retrieving cipher key failed!\r\n");
      return CBOOT_ERROR_INVALID_PARAMETERS;
   }

   // Begin of handling block
   if(hasMailboxKey)
   {
      uint8_t psk[BOOT_MBX_PSK_MAX_SIZE] = {0};
      size_t pskSize = 0;

      // Get message from shared SRAM (contains a PSK)
      mailBoxGetPsk(psk, (uint32_t *)&pskSize);

      // Check PSK used to decode data in secondary flash (external memory)
      if(!isValidPskSize(pskSize))
      {
         TRACE_ERROR("Retrieving cipher key failed!\r\n");
         memset(psk, 0, sizeof(psk));
         return CBOOT_ERROR_INVALID_PARAMETERS;
      }

      // Store cipher key used to decode data in secondary flash (external
      // memory)
      memcpy(context->psk, psk, pskSize);
      context->pskSize = pskSize;
      // Make sure to reset the message from shared RAM memory
      mailBoxClearPsk();
      memset(psk, 0, sizeof(psk));
   }
   else
   {
      // Get PSK from the settings
      if(isValidPskSize(context->settings.pskSize))
      {
         memcpy(context->psk, context->settings.psk, context->settings.pskSize);
         context->pskSize = context->settings.pskSize;
      }
      else
      {
         TRACE_ERROR("Retrieving cipher key failed!\r\n");
         return CBOOT_ERROR_INVALID_PARAMETERS;
      }
   }

   return CBOOT_NO_ERROR;

#else
   // Return error code
   (void)context;
   TRACE_ERROR("bootGetCipherKey called in unsupported configuration.\r\n");
   return CBOOT_ERROR_NOT_IMPLEMENTED;
#endif
}

static bool_t isValidPskSize(size_t size) {
   return (size == 16 || size == 32);
}

/**
 * @brief Jump to the application binary inside the current image in internal
 * flash.
 * @param[in] context Pointer to the bootloader context
 * @return Status code
 **/
//TODO: should use a consistent approach using context->selectedSlot instead of directly
/*
 * e.g.,
 * cboot_error_t bootJumpToApp(BootContext *context)
   {
   if (context == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   if (context->memoriesCount == 0 || context->memories[0].driver == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   mcuJumpToApplication(context->selectedSlot.addr + mcuGetVtorOffset());

   //Never reached
   return CBOOT_NO_ERROR;
   }*/
cboot_error_t bootJumpToApp(BootContext *context)
{
   error_t error;
   const FlashInfo *info;
   uint32_t mcuVtorOffset;
   FlashDriver *driver;

   // Check parameter validity
   if(context == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Get primary flash memory information
   driver = (FlashDriver *)context->memories[0].driver;

   //TODO: also add a nb memories check here
   if(context->memories[0].driver == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   error = driver->getInfo(&info);
   // Is any error?
   if(error)
      return CBOOT_ERROR_FAILURE;

   // Get MCU VTOR offset
   mcuVtorOffset = mcuGetVtorOffset();

   // Jump to the application at the given address
   mcuJumpToApplication(info->flashAddr + BOOT_OFFSET + mcuVtorOffset);

   // Never reached
   return CBOOT_NO_ERROR;
}

static cboot_error_t handleFallbackTriggers(BootContext *context)
{
   cboot_error_t cerror = CBOOT_NO_ERROR;

#if (BOOT_FALLBACK_MANUAL_MODE == ENABLED)
   cerror = checkManualFallbackTrigger(context);
   if(cerror != CBOOT_NO_ERROR)
      return cerror;
#endif

#if (BOOT_FALLBACK_AUTO_MODE == ENABLED)
   cerror = checkAutoFallbackTrigger(context);
   if(cerror != CBOOT_NO_ERROR)
      return cerror;
#endif

   return cerror;
}

static cboot_error_t checkAutoFallbackTrigger(BootContext *context) {

#if (BOOT_COUNTER_SUPPORT == ENABLED)
   if(mailBoxGetBootCounter() >= BOOT_COUNTER_MAX_ATTEMPTS)
   {
      TRACE_INFO("Max boot attempts reached. Initiating fallback...\r\n");
      mailBoxClearBootCounter();
      bootChangeState(context, BOOT_STATE_FALLBACK_APP);
   }
#endif

#if (BOOT_FLAG_SUPPORT == ENABLED)
   //TODO: verify this is initialized to 0 on a clean boot.
   if((!mailBoxIsUpdateConfirmed() && mailBoxGetBootCounter() != 0) ||
      mailBoxIsFallbackRequested())
   {
      TRACE_INFO("Fallback due to unconfirmed update or explicit request...\r\n");
      mailBoxClearBootCounter();
      bootChangeState(context, BOOT_STATE_FALLBACK_APP);
   }
#endif

   return CBOOT_NO_ERROR;
}

static cboot_error_t checkManualFallbackTrigger(BootContext *context) {
   cboot_error_t cerror;
   TriggerStatus trigStatus;
   cerror = fallbackTriggerGetStatus(&trigStatus);
   if(cerror)
   {
      TRACE_ERROR("Failed to get fallback trigger status.\r\n");
      bootHandleFallbackError();
      return cerror;
   }

   if(trigStatus == TRIGGER_STATUS_RAISED || mailBoxIsFallbackRequested())
   {
      TRACE_INFO("Fallback trigger detected. Initiating fallback...\r\n");
      mailBoxSetFallbackRequested(FALSE);
      bootChangeState(context, BOOT_STATE_FALLBACK_APP);
   }

   return CBOOT_NO_ERROR;
}


static cboot_error_t handleIdleState(BootContext *context)
{
   bootIdleStateHook();

   cboot_error_t cerror = bootSelectUpdateImageSlot(context, &context->selectedSlot);

   if(cerror == CBOOT_ERROR_FIRMWARE_CORRUPTED)
   {
#if BOOT_FALLBACK_SUPPORT == ENABLED
      bootChangeState(context, BOOT_STATE_FALLBACK_APP);
#else
      bootChangeState(context, BOOT_STATE_ERROR);
      TRACE_ERROR("Firmware corrupted, no fallback available.\r\n");
#endif
      return cerror;
   }

   if(cerror || context->selectedSlot.memParent == NULL)
   {
      TRACE_ERROR("No valid update image found.\r\n");
      bootNoValidUpdatesHook();
   }
   else
   {
      TRACE_DEBUG("Selected slot: addr=0x%08lX, size=0x%08X\r\n",
         (unsigned long)context->selectedSlot.addr, context->selectedSlot.size);

      uint8_t result;
      //TODO: hardcoded without bounds check
      memoryCompareSlot(&context->selectedSlot, &context->memories[0].slots[0], &result);
      bootChangeState(context, result ? BOOT_STATE_UPDATE_APP : BOOT_STATE_RUN_APP);
   }

   return cerror;
}

static cboot_error_t handleRunAppState(BootContext *context)
{
   cboot_error_t cerror;

   //TODO: add bounds checking and NULL check for the fields
   //TODO: make the slot selection policy more clear
   //TODO: make it dynamic to handle more advanced use cases (option)
   /*
    * e.g., cerror = bootSelectActiveSlot(context, &context->selectedSlot);
          if (cerror) return cerror;
    */
   context->selectedSlot = context->memories[0].slots[0];

   TRACE_INFO("No update available. Checking current application...\r\n");

   cerror = bootCheckImage(context, &context->selectedSlot);
   if(cerror)
   {
      handleImageCheckError(context, cerror);
      return cerror;
   }
#if (BOOT_RUNTIME_INTEGRITY_CHECK_SUPPORT == ENABLED || BOOT_RUNTIME_SIGNATURE_CHECK_SUPPORT == ENABLED)
   cerror = bootCheckRuntimeImage(context, &context->selectedSlot);
   if(cerror)
   {
      handleImageCheckError(context, cerror);
      return cerror;
   }
#endif
   cerror = bootCheckSlotAppResetVector(&context->selectedSlot);
   if(cerror)
   {
      bootChangeState(context, BOOT_STATE_ERROR);
      return cerror;
   }

   TRACE_INFO("Application image is valid. Booting...\r\n");

#if (BOOT_FALLBACK_SUPPORT == ENABLED && BOOT_FALLBACK_AUTO_MODE == ENABLED)
   mailBoxIncrementBootCounter();
#endif

#if (BOOT_FALLBACK_SUPPORT == ENABLED)
   if(mailBoxIsFallbackPerformed())
   {
      TRACE_INFO("Fallback has been performed. Booting from previous version...\r\n");
      bootFallbackPerformedHook();
   }
#endif

   uint32_t appStartAddr = context->selectedSlot.addr + mcuGetVtorOffset();
   mcuJumpToApplication(appStartAddr);

   return cerror;
}

static cboot_error_t handleUpdateAppState(BootContext *context)
{
   cboot_error_t cerror;
   TRACE_INFO("Checking update application image...\r\n");

#if (BOOT_EXT_MEM_ENCRYPTION_SUPPORT == ENABLED)
   cerror = bootGetCipherKey(context);
   if(cerror)
   {
      TRACE_ERROR("Failed to retrieve cipher key.\r\n");
      bootChangeState(context, BOOT_STATE_RUN_APP);
      return CBOOT_NO_ERROR;
   }
#endif

   cerror = bootCheckImage(context, &context->selectedSlot);
   if(cerror)
   {
      bootChangeState(context, BOOT_STATE_RUN_APP);
      //TODO: Consider returning the actual error and letting the FSM decide, or at minimum logging it before discarding
      TRACE_INFO("Update image check failed (0x%X). Falling back to run app.\r\n", cerror);
      return CBOOT_NO_ERROR; //TODO: intentional, document why
   }

   TRACE_INFO("Starting update procedure...\r\n");
   cerror = bootUpdateApp(context, &context->selectedSlot);

   if(cerror)
   {
      TRACE_INFO("Update app failed (0x%X).\r\n", cerror);
      //TODO: need a hook here and possibliy more information and a safe way out to boot (or fallback)
      bootChangeState(context, BOOT_STATE_ERROR);
   }
   else
   {
#if (BOOT_FALLBACK_SUPPORT == ENABLED && BOOT_FALLBACK_AUTO_MODE == ENABLED)
      mailBoxSetUpdatePerformed(TRUE);
      mailBoxSetUpdateConfirmed(FALSE);
      mailBoxClearBootCounter();
#endif
      TRACE_INFO("Update complete. Rebooting...\r\n");
      mcuSystemReset();
   }

   return cerror; // unreachable
}

#if (BOOT_FALLBACK_SUPPORT == ENABLED)
static cboot_error_t handleFallbackAppState(BootContext *context)
{
   cboot_error_t cerror = fallbackTask(context, context->memories);

   if(cerror)
   {
      TRACE_INFO("Fallback failed.\r\n");
      bootHandleFallbackError();
      bootChangeState(context, BOOT_STATE_RUN_APP);
   }
   else
   {
      mailBoxSetUpdateConfirmed(FALSE);
      mailBoxSetFallbackPerformed(TRUE);
      mailBoxClearBootCounter();
      TRACE_INFO("Fallback complete. Rebooting...\r\n");
      mcuSystemReset();
   }

   return cerror;
}
#endif

static void handleImageCheckError(BootContext *context, cboot_error_t cerror)
{
#if (BOOT_FALLBACK_SUPPORT == ENABLED)
   if(cerror == CBOOT_ERROR_FIRMWARE_CORRUPTED)
      bootChangeState(context, BOOT_STATE_FALLBACK_APP);
#else
   bootChangeState(context, BOOT_STATE_ERROR);
#endif
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#if ((defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) || defined(__GNUC__) ||            \
   defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__) || defined(__TASKING__) ||                  \
   defined(__CWCC__) || defined(__TI_ARM__))
__weak_func void bootInitHook(void)
{
   /*
    * This function will be called when the bootloader is initialized.
    * It can be redefined in user code.
    */
}

__weak_func void bootIdleStateHook(void)
{
   /*
    * This function will be called when the bootloader enter to IDLE state.
    * It can be redefined in user code. Note: this function can be called more
    * than once.
    */
}
__weak_func void bootNoValidUpdatesHook(void)
{
   /*
    * This function will be called when the bootFsm determines there are not
    * valid images to be installed. It can be redefined in user code. Note: this
    * function can be called more than once, at each start up.
    */
}
__weak_func void bootJumpingToApplicationHook(void)
{
   /*
    * This function will be called before the bootloader jumps to the application.
    * It can be redefined in user code. Note: this
    * function can be called more than once, at each start up.
    */
}

__weak_func void bootFallbackPerformedHook(void) {
   /*
    * This function will be called if a fallback has been performed previously. It is invoked just before jumping
    * to the user application.
    * It can be redefined in user code. Note: this
    * function can be called more than once, at each start up.
    */
}
__weak_func void bootHandleFallbackError(void)
{
   /*
    * This function will be called when the bootloader cannot complete the
    * fallback process. It can be redefined in user code.
    */
}

__weak_func void bootHandleGenericError(void)
{
   /*
    * This function will be called when the bootloader encounters a generic error
    * state. It can be redefined in user code.
    */
}
#endif
