/**
 * @file mailbox.c
 * @brief Boot Mailbox mangement functions
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
#include "mailbox.h"

// Create a section to place the boot mailbox in RAM
#if defined(__CC_ARM)
BootMailBox bootMailBox __attribute__((__section__(".boot_mailbox"), zero_init));
#elif (defined(__GNUC__) && defined(__MINGW32__)) || (defined(__GNUC__) && defined(__MINGW64__))
extern BootMailBox bootMailBox;
#elif defined(__GNUC__)
BootMailBox bootMailBox __attribute__((section(".boot_mailbox")));
#elif defined(_MSC_VER)
extern BootMailBox bootMailBox;
#endif

static void mbx_set_flag(uint32_t flag, bool_t value)
{
#if (BOOT_MAILBOX_NVM_SUPPORT == DISABLED)
   if(value)
   {
      bootMailBox.flags |= flag;
   }
   else
   {
      bootMailBox.flags &= ~flag;
   }
#else
   if(flag == UPDATE_REQUESTED)
   {
      mbx_set_nvm_flag(BOOT_MBX_NVM_FLAG_UPDATE_REQUESTED, value);
   }
   else if(flag == UPDATE_PERFORMED)
   {
      mbx_set_nvm_flag(BOOT_MBX_NVM_FLAG_UPDATE_PERFORMED, value);
   }
   else if(flag == UPDATE_CONFIRMED)
   {
      mbx_set_nvm_flag(BOOT_MBX_NVM_FLAG_UPDATE_CONFIRMED, value);
   }
   else if(flag == FALLBACK_REQUESTED)
   {
      mbx_set_nvm_flag(BOOT_MBX_NVM_FLAG_FALLBACK_REQUESTED, value);
   }
   else if(flag == FALLBACK_PERFORMED)
   {
      mbx_set_nvm_flag(BOOT_MBX_NVM_FLAG_FALLBACK_PERFORMED, value);
   }
   else
   {
      // Unknown flag
   }
#endif
}

static bool_t mbx_get_flag(uint32_t flag)
{
#if (BOOT_MAILBOX_NVM_SUPPORT == DISABLED)
   uint32_t result = bootMailBox.flags & flag;

   if(result == UPDATE_REQUESTED)
   {
      return TRUE;
   }

   if(result == UPDATE_PERFORMED)
   {
      return TRUE;
   }

   if(result == UPDATE_CONFIRMED)
   {
      return TRUE;
   }


   if(result == FALLBACK_REQUESTED)
   {
      return TRUE;
   }

   if(result == FALLBACK_PERFORMED)
   {
      return TRUE;
   }

   return FALSE;
#else
   return mbx_get_nvm_flag(flag);
#endif
}

/**
 * @brief Initializes the shared bootloader mailbox if signature is not valid.
 **/

void mailBoxInit(void)
{
   if(bootMailBox.signature != BOOT_MBX_SIGNATURE)
   {
      TRACE_INFO("Initializing mailbox...\r\n");

      memset(&bootMailBox, 0, sizeof(bootMailBox));
      bootMailBox.signature = BOOT_MBX_SIGNATURE;
   }
}

/**
 * @brief Checks whether UPDATE_REQUESTED message is present in the mailbox
 * @return TRUE or FALSE
 **/
bool_t mailBoxIsUpdateRequested(void) {
   return mbx_get_flag(UPDATE_REQUESTED);
}

/**
 * @brief Posts UPDATE_REQUESTED message to the mailbox
 * @param[in] state state of the flag, TRUE or FALSE
 **/
void mailBoxSetUpdateRequested(bool_t state) {
   mbx_set_flag(UPDATE_REQUESTED, state);
}

/**
 * @brief Checks whether UPDATE_PERFORMED message is present in the mailbox
 * @return TRUE or FALSE
 **/
bool_t mailBoxIsUpdatePerformed(void) {
   return mbx_get_flag(UPDATE_PERFORMED);
}

/**
 * @brief Posts UPDATE_PERFORMED message to the mailbox
 * @param[in] state state of the flag, TRUE or FALSE
 **/
void mailBoxSetUpdatePerformed(bool_t state) {
   mbx_set_flag(UPDATE_PERFORMED, state);
}

/**
 * @brief Checks whether UPDATE_CONFIRMED message is present in the mailbox
 * @return TRUE or FALSE
 **/
bool_t mailBoxIsUpdateConfirmed(void) {
   return mbx_get_flag(UPDATE_CONFIRMED);
}

/**
 * @brief Posts UPDATE_REQUESTED message to the mailbox
 * @param[in] state state of the flag, TRUE or FALSE
 **/
void mailBoxSetUpdateConfirmed(bool_t state) {
   mbx_set_flag(UPDATE_CONFIRMED, state);
}

/**
 * @brief Checks whether FALLBACK_REQUESTED message is present in the mailbox
 * @return TRUE or FALSE
 **/
bool_t mailBoxIsFallbackRequested(void) {
   return mbx_get_flag(FALLBACK_REQUESTED);
}

/**
 * @brief Posts FALLBACK_REQUESTED message to the mailbox
 * @param[in] state state of the flag, TRUE or FALSE
 **/
void mailBoxSetFallbackRequested(bool_t state) {
   mbx_set_flag(FALLBACK_REQUESTED, state);
}

/**
 * @brief Checks whether FALLBACK_PERFORMED message is present in the mailbox
 * @return TRUE or FALSE
 **/
bool_t mailBoxIsFallbackPerformed(void) {
   return mbx_get_flag(FALLBACK_PERFORMED);
}

/**
 * @brief Posts FALLBACK_PERFORMED message to the mailbox
 * @param[in] state state of the flag, TRUE or FALSE
 **/
void mailBoxSetFallbackPerformed(bool_t state) {
   mbx_set_flag(FALLBACK_PERFORMED, state);
}

/**
 * @brief Increments the boot counter in the mailbox
 **/
void mailBoxIncrementBootCounter(void) {
   bootMailBox.boot_counter++;
}

/**
 * @brief Returns the current value of the boot counter in the mailbox
 **/
uint32_t mailBoxGetBootCounter(void) {
   return bootMailBox.boot_counter;
}

/**
 * @brief Clears the current value of the boot counter in the mailbox
 **/
void mailBoxClearBootCounter(void) {
   bootMailBox.boot_counter = 0;
}

/**
 * @brief Set Pre-Shared Key value of the mailbox
 * @param[in] psk pointer to the pre-shared key
 * @param[in] pskSize length of the pre-shared key
 **/
void mailBoxSetPsk(uint8_t *psk, uint32_t pskSize)
{
   if((pskSize != 0 && pskSize <= BOOT_MBX_PSK_MAX_SIZE) && psk != NULL)
   {
      bootMailBox.pskSize = pskSize;
      memcpy(bootMailBox.psk, psk, pskSize);
   }
   else
   {
      bootMailBox.pskSize = 0;
   }
}

/**
 * @brief Returns a copy of Pre-Shared Key value of the mailbox
 * @param[in] psk pointer to a buffer where pre-shared key will be copied
 * @param[in] pskSize pointer to a variable where the length of the pre-shared
 * key will be copied
 **/
void mailBoxGetPsk(uint8_t *psk, uint32_t *pskSize)
{
   *pskSize = bootMailBox.pskSize;
   if(*pskSize <= BOOT_MBX_PSK_MAX_SIZE && psk != NULL)
   {
      memcpy(psk, bootMailBox.psk, bootMailBox.pskSize);
   }
   else
   {
      *pskSize = 0;
   }
}

/**
 * @brief Clear the Pre-Shared Key from the mailbox
 **/
void mailBoxClearPsk(void) {
   memset(bootMailBox.psk, 0, bootMailBox.pskSize);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#if ((defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) || defined(__GNUC__) ||            \
   defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__) || defined(__TASKING__) ||                  \
   defined(__CWCC__) || defined(__TI_ARM__))
__weak_func cboot_error_t mbx_set_nvm_flag(uint32_t flag, bool_t value)
{
   return CBOOT_ERROR_NOT_IMPLEMENTED;
}
__weak_func bool_t mbx_get_nvm_flag(uint32_t flag) {
   return FALSE;
}
__weak_func cboot_error_t mbx_set_nvm_psk(uint8_t *psk, size_t pskSize)
{
   return CBOOT_ERROR_NOT_IMPLEMENTED;
}
__weak_func bool_t mbx_get_nvm_psk(uint8_t *psk, size_t pskSize)
{
   return CBOOT_ERROR_NOT_IMPLEMENTED;
}
#endif
