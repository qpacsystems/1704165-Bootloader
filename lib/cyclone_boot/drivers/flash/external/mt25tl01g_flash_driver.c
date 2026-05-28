/**
 * @file mt25tl01g_flash_driver.c
 * @brief CycloneBOOT MT25TL01G Flash Driver
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
#define TRACE_LEVEL CBOOT_DRIVER_TRACE_LEVEL

// Dependencies
#include "mt25tl01g_flash_driver.h"

#include "core/flash.h"
#include "debug.h"
#ifdef USE_STM32H750B_DISCO
#include "stm32h750b_discovery_qspi.h"
#else
#include "stm32h743i_eval_qspi.h"
#endif
// Memory driver private related functions
error_t mt25tl01gFlashDriverInit(void);
error_t mt25tl01gFlashDriverDeInit(void);
error_t mt25tl01gFlashDriverGetInfo(const FlashInfo **info);
error_t mt25tl01gFlashDriverGetStatus(FlashStatus *status);
error_t mt25tl01gFlashDriverWrite(uint32_t address, uint8_t *data, size_t length);
error_t mt25tl01gFlashDriverRead(uint32_t address, uint8_t *data, size_t length);
error_t mt25tl01gFlashDriverErase(uint32_t address, size_t length);
bool_t mt25tl01gFlashDriverSectorAddr(uint32_t address);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Memory Information
 **/

const FlashInfo mt25tl01gFlashDriverInfo = {
   FLASH_DRIVER_VERSION, MT25TL01G_NAME, FLASH_TYPE_EXTERNAL_QSPI,
   // MEM_CLASS_FLASH,
   MT25TL01G_ADDR, MT25TL01G_SIZE, MT25TL01G_WRITE_SIZE, MT25TL01G_READ_SIZE, 0, 0, 0, 0, 0
};

/**
 * @brief Memory Driver
 **/

const FlashDriver mt25tl01gFlashDriver = {
   mt25tl01gFlashDriverInit,
   mt25tl01gFlashDriverDeInit,
   mt25tl01gFlashDriverGetInfo,
   mt25tl01gFlashDriverGetStatus,
   mt25tl01gFlashDriverWrite,
   mt25tl01gFlashDriverRead,
   mt25tl01gFlashDriverErase,
   NULL,
   NULL,
   mt25tl01gFlashDriverSectorAddr,
};

/**
 * @brief Initialize Flash Memory.
 * @return Error code
 **/

error_t mt25tl01gFlashDriverInit(void)
{
   int32_t status;

   // Debug message
   TRACE_INFO("Initializing %s memory...\r\n", MT25TL01G_NAME);

   status = BSP_QSPI_Init();

   if(status != QSPI_OK)
   {
      TRACE_ERROR("Failed to initialize QSPI NOR Flash!\r\n");
      return ERROR_FAILURE;
   }

   // Successfully processed
   return NO_ERROR;
}

/**
 * @brief Initialize Flash Memory.
 * @return Error code
 **/

error_t mt25tl01gFlashDriverDeInit(void) {
   return ERROR_NOT_IMPLEMENTED;
}

/**
 * @brief Get Flash Memory information.
 * @param[in,out] info Pointer to the Memory information structure to be
 * returned
 * @return Error code
 **/

error_t mt25tl01gFlashDriverGetInfo(const FlashInfo **info)
{
   // Set Memory information pointer
   *info = (const FlashInfo *)&mt25tl01gFlashDriverInfo;

   // Successfully processed
   return NO_ERROR;
}

/**
 * @brief Get Flash Memory status.
 * @param[in,out] status Pointer to the Memory status to be returned
 * @return Error code
 **/

error_t mt25tl01gFlashDriverGetStatus(FlashStatus *status)
{
   uint8_t flag;

   // Check parameter validity
   if(status == NULL)
      return ERROR_INVALID_PARAMETER;

   // Get QSPI Flash Memory error flags status
   flag = BSP_QSPI_GetStatus();
   // Is any error flag set?
   if(flag == QSPI_OK)
   {
      // Set Flash memory status
      *status = FLASH_STATUS_OK;
   }
   else if(flag == QSPI_BUSY)
   {
      // Set Flash memory status
      *status = FLASH_STATUS_BUSY;
   }
   else
   {
      // Set Flash memory status
      *status = FLASH_STATUS_ERR;
   }

   // Successfully processed
   return NO_ERROR;
}

/**
 * @brief Write data in Flash Memory at the given address.
 * @param[in] address Address in Flash Memory to write to
 * @param[in] data Pointer to the data to write
 * @param[in] length Number of data bytes to write in
 * @return Error code
 **/

error_t mt25tl01gFlashDriverWrite(uint32_t address, uint8_t *data, size_t length)
{
   error_t error;
   uint32_t status;
   uint32_t topAddress;
   const uint8_t *p;
   uint8_t word[4];
   size_t n;

   // Precompute the top address
   topAddress = MT25TL01G_ADDR + MT25TL01G_SIZE;

   // Check address validity
#ifdef MT25TL01G_MEMORY_MAPPED
   if((address >= MEMORY_XIP_ADDR(topAddress)) || (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > MEMORY_XIP_ADDR(topAddress))
      return ERROR_INVALID_PARAMETER;
#else
   if((address >= topAddress) || (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#endif

   // Cast data pointer
   p = (const uint8_t *)data;

   // Perform write operation
   while(length > 0)
   {
      // Prevent to write more than 4 bytes at a time
      n = MIN(sizeof(word), length);

      // Check if remaining bytes is less than 4 (32bits word)
      if(n < sizeof(uint32_t))
         memset(word, 0, sizeof(word));

      // Copy n bytes
      memcpy(word, p, n);

      // Is address match sector start address?
      if(address % MT25TL01G_SUBSECTORS_SIZE == 0)
      {

         // Erases the specified block
         status = BSP_QSPI_Erase_Block(MEMORY_NO_XIP_ADDR(address));
         // Is any error?
         if(status != QSPI_OK)
            return ERROR_FAILURE;
      }

      // Program 32-bit word in flash memory
      status = BSP_QSPI_Write(word, MEMORY_NO_XIP_ADDR(address), sizeof(uint32_t));
      if(status != QSPI_OK)
      {
         TRACE_ERROR("Failed to write in flash memory!\r\n");
         error = ERROR_FAILURE;
         return error;
      }
      // Advance data pointer
      p += n;
      // Increment word address
      address += n;
      // Remaining bytes to be written
      length -= n;
   }

   // Successful process
   return NO_ERROR;
}

/**
 * @brief Read data from Memory at the given address.
 * @param[in] address Address in Memory to read from
 * @param[in] data Buffer to store read data
 * @param[in] length Number of data bytes to read out
 * @return Error code
 **/

error_t mt25tl01gFlashDriverRead(uint32_t address, uint8_t *data, size_t length)
{
   int32_t status;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = MT25TL01G_ADDR + MT25TL01G_SIZE;

#ifdef MT25TL01G_MEMORY_MAPPED
   // Check address validity
   if(address >= MEMORY_XIP_ADDR(topAddress))
      return ERROR_INVALID_PARAMETER;

   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > MEMORY_XIP_ADDR(topAddress))
      return ERROR_INVALID_PARAMETER;
#else
   // Check address validity
   if(address >= topAddress)
      return ERROR_INVALID_PARAMETER;

   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#endif

   // Perform read operation
   status = BSP_QSPI_Read(data, MEMORY_NO_XIP_ADDR(address), length);
   if(status != QSPI_OK)
   {
      TRACE_ERROR("Failed to read from flash memory!\r\n");
      return ERROR_FAILURE;
   }
   // Successfully processed
   return NO_ERROR;
}

/**
 * @brief Erase data from Memory at the given address.
 * The erase operation will be done sector by sector according to
 * the given memory address and size.
 * @param[in] address Memory start erase address
 * @param[in] length Number of data bytes to be erased
 * @return Error code
 **/

error_t mt25tl01gFlashDriverErase(uint32_t address, size_t length)
{
   uint32_t status;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = MT25TL01G_ADDR + MT25TL01G_SIZE;

#ifdef MT25TL01G_MEMORY_MAPPED
   // Check address validity
   if(address >= MEMORY_XIP_ADDR(topAddress))
      return ERROR_INVALID_PARAMETER;

   // Check parameters validity (is data in flash)
   if(address + length > MEMORY_XIP_ADDR(topAddress))
      return ERROR_INVALID_PARAMETER;
#else
   // Check address validity
   if(address >= topAddress)
      return ERROR_INVALID_PARAMETER;

   // Check parameters validity (is data in flash)
   if(address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#endif
   // Be sure address match a memory flash subsector start address
   if(address % MT25TL01G_SUBSECTORS_SIZE != 0)
   {
      length += address % MT25TL01G_SUBSECTORS_SIZE;
      address -= address % MT25TL01G_SUBSECTORS_SIZE;
   }

   // Perform erase operation
   while(length > 0)
   {
      // Erases the specified block
      status = BSP_QSPI_Erase_Block(MEMORY_NO_XIP_ADDR(address));
      if(status != QSPI_OK)
      {
         TRACE_ERROR("Failed to erase flash memory block!\r\n");
         return ERROR_FAILURE;
      }
      // Increment word address
      address += MT25TL01G_SUBSECTORS_SIZE;
      // Remaining bytes to be erased
      length -= MIN(length, MT25TL01G_SUBSECTORS_SIZE);
   }

   // Successful process
   return NO_ERROR;
}

bool_t mt25tl01gFlashDriverSectorAddr(uint32_t address) {
   return TRUE;
}
