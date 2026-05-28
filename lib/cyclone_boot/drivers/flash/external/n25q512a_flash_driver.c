/**
 * @file n25q512a_flash_driver.c
 * @brief CycloneBOOT N25Q512A Flash Driver
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
#include "n25q512a_flash_driver.h"

#include "core/flash.h"
#include "debug.h"
#include "stm32f769i_eval_qspi.h"

// Memory driver private related functions
error_t n25q512aFlashDriverInit(void);
error_t n25q512aFlashDriverDeInit(void);
error_t n25q512aFlashDriverGetInfo(const FlashInfo **info);
error_t n25q512aFlashDriverGetStatus(FlashStatus *status);
error_t n25q512aFlashDriverWrite(uint32_t address, uint8_t *data, size_t length);
error_t n25q512aFlashDriverRead(uint32_t address, uint8_t *data, size_t length);
error_t n25q512aFlashDriverErase(uint32_t address, size_t length);
bool_t n25q512aFlashDriverSectorAddr(uint32_t address);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Memory Information
 **/

const FlashInfo n25q512aFlashDriverInfo = {
   FLASH_DRIVER_VERSION, N25Q512A_NAME, FLASH_TYPE_EXTERNAL_QSPI,
   // MEM_CLASS_FLASH,
   N25Q512A_ADDR, N25Q512A_SIZE, N25Q512A_WRITE_SIZE, N25Q512A_READ_SIZE, 0, 0, 0, 0, 0
};

/**
 * @brief Memory Driver
 **/

const FlashDriver n25q512aFlashDriver = {
   n25q512aFlashDriverInit,
   n25q512aFlashDriverDeInit,
   n25q512aFlashDriverGetInfo,
   n25q512aFlashDriverGetStatus,
   n25q512aFlashDriverWrite,
   n25q512aFlashDriverRead,
   n25q512aFlashDriverErase,
   NULL,
   NULL,
   n25q512aFlashDriverSectorAddr,
};

/**
 * @brief Initialize Flash Memory.
 * @return Error code
 **/

error_t n25q512aFlashDriverInit(void)
{
   uint8_t status;

   // Debug message
   TRACE_INFO("Initializing %s memory...\r\n", N25Q512A_NAME);

   status = BSP_QSPI_Init();
   if(status != QSPI_OK)
   {
      TRACE_ERROR("Failed to initialize QSPI NOR Flash!\r\n");
      return ERROR_FAILURE;
   }

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Initialize Flash Memory.
 * @return Error code
 **/

error_t n25q512aFlashDriverDeInit(void) {
   return ERROR_NOT_IMPLEMENTED;
}

/**
 * @brief Get Flash Memory information.
 * @param[in,out] info Pointeur to the Memory information structure to be
 * returned
 * @return Error code
 **/

error_t n25q512aFlashDriverGetInfo(const FlashInfo **info)
{
   // Set Memory information pointeur
   *info = (const FlashInfo *)&n25q512aFlashDriverInfo;

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Get Flash Memory status.
 * @param[in,out] status Pointeur to the Memory status to be returned
 * @return Error code
 **/

error_t n25q512aFlashDriverGetStatus(FlashStatus *status)
{
   uint8_t flag;

   // Check parameter vailidity
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

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Write data in Flash Memory at the given address.
 * @param[in] address Address in Flash Memory to write to
 * @param[in] data Pointeur to the data to write
 * @param[in] length Number of data bytes to write in
 * @return Error code
 **/

error_t n25q512aFlashDriverWrite(uint32_t address, uint8_t *data, size_t length)
{
   error_t error;
   uint8_t status;
   uint32_t topAddress;
   const uint8_t *p;
   uint8_t word[4];
   size_t n;

   // Precompute the top address
   topAddress = N25Q512A_ADDR + N25Q512A_SIZE;

   // Check address validity
   if((address >= topAddress) || (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;

   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;

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
      if(address % N25Q512A_SUBSECTORS_SIZE == 0)
      {
         // Erases the specified block
         status = BSP_QSPI_Erase_Block(address);
         // Is any error?
         if(status != QSPI_OK)
            return ERROR_FAILURE;
      }

      // Program 32-bit word in flash memory
      status = BSP_QSPI_Write(word, address, sizeof(uint32_t));
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

error_t n25q512aFlashDriverRead(uint32_t address, uint8_t *data, size_t length)
{
   uint8_t status;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = N25Q512A_ADDR + N25Q512A_SIZE;

   // Check address validity
   if(address >= topAddress)
      return ERROR_INVALID_PARAMETER;

   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;

   // Perform read operation
   status = BSP_QSPI_Read(data, address, length);
   if(status != QSPI_OK)
   {
      TRACE_ERROR("Failed to read from flash memory!\r\n");
      return ERROR_FAILURE;
   }

   // Successfull process
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

error_t n25q512aFlashDriverErase(uint32_t address, size_t length)
{
   uint8_t status;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = N25Q512A_ADDR + N25Q512A_SIZE;

   // Check address validity
   if(address >= topAddress)
      return ERROR_INVALID_PARAMETER;

   // Check parameters validity (is data in flash)
   if(address + length > topAddress)
      return ERROR_INVALID_PARAMETER;

   // Be sure address match a memory flash subsector start address
   if(address % N25Q512A_SUBSECTORS_SIZE != 0)
   {
      length += address % N25Q512A_SUBSECTORS_SIZE;
      address -= address % N25Q512A_SUBSECTORS_SIZE;
   }

   // Perform erase operation
   while(length > 0)
   {
      // Erases the specified block
      status = BSP_QSPI_Erase_Block(address);
      if(status != QSPI_OK)
      {
         TRACE_ERROR("Failed to erase flash memory block!\r\n");
         return ERROR_FAILURE;
      }

      // Increment word address
      address += N25Q512A_SUBSECTORS_SIZE;
      // Remaining bytes to be erased
      length -= MIN(length, N25Q512A_SUBSECTORS_SIZE);
   }

   // Successful process
   return NO_ERROR;
}

bool_t n25q512aFlashDriverSectorAddr(uint32_t address) {
   return TRUE;
}
