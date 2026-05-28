/**
 * @file at25sf128a_flash_driver.c
 * @brief CycloneBOOT AT25SF128A Flash Driver
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
#include "at25sf128a_flash_driver.h"

#include "core/flash.h"
#include "debug.h"
#include "spi_flash_driver.h"

// Memory driver private related functions
error_t at25sf128aFlashDriverInit(void);
error_t at25sf128aFlashDriverGetInfo(const FlashInfo **info);
error_t at25sf128aFlashDriverGetStatus(FlashStatus *status);
error_t at25sf128aFlashDriverWrite(uint32_t address, uint8_t *data, size_t length);
error_t at25sf128aFlashDriverRead(uint32_t address, uint8_t *data, size_t length);
error_t at25sf128aFlashDriverSwapBanks(void);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Memory Information
 **/

const FlashInfo at25sf128aFlashDriverInfo = {
   FLASH_DRIVER_VERSION, AT25SF128A_NAME, FLASH_TYPE_EXTERNAL_SPI,
   // MEM_CLASS_FLASH,
   AT25SF128A_ADDR, AT25SF128A_SIZE, AT25SF128A_WRITE_SIZE, AT25SF128A_READ_SIZE, 0, 0, 0, 0, 0
};

/**
 * @brief Memory Driver
 **/

const FlashDriver at25sf128aFlashDriver = {
   at25sf128aFlashDriverInit, at25sf128aFlashDriverGetInfo, at25sf128aFlashDriverGetStatus,
   at25sf128aFlashDriverWrite, at25sf128aFlashDriverRead, NULL
};

/**
 * @brief Initialize at25sf128a Flash Memory.
 * @return Error code
 **/

error_t at25sf128aFlashDriverInit(void)
{
   // Initialize AT25SF128A Flash Memory
   return spiFlashInit();
}

/**
 * @brief Get Memory information.
 * @param[in,out] info Pointeur to the Memory information structure to be
 * returned
 * @return Error code
 **/

error_t at25sf128aFlashDriverGetInfo(const FlashInfo **info)
{
   // Set Memory information pointeur
   *info = (const FlashInfo *)&at25sf128aFlashDriverInfo;

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Get Memory status.
 * @param[in,out] status Pointeur to the Memory status to be returned
 * @return Error code
 **/

error_t at25sf128aFlashDriverGetStatus(FlashStatus *status)
{
   error_t error;
   uint8_t tempStatus;

   // Check parameter vailidity
   if(status == NULL)
      return ERROR_INVALID_PARAMETER;

   // Get at25sf128a status
   error = spiFlashReadStatus(&tempStatus);
   // Is any error?
   if(error)
      return error;

   if(tempStatus)
   {
      *status = FLASH_STATUS_BUSY;
   }
   else
   {
      *status = FLASH_STATUS_OK;
   }

   // Successful process
   return NO_ERROR;
}

/**
 * @brief Write data in Memory at the given address.
 * @param[in] address Address in Memory to write to
 * @param[in] data Pointeur to the data to write
 * @param[in] length Number of data bytes to write in
 * @return Error code
 **/

error_t at25sf128aFlashDriverWrite(uint32_t address, uint8_t *data, size_t length)
{
   error_t error;
   uint32_t topAddress;
   const uint8_t *p;
   uint8_t word[4];
   size_t n;

   // Precompute the top address
   topAddress = AT25SF128A_ADDR + AT25SF128A_SIZE;

   // Check address validity
   if(address >= topAddress || address % sizeof(uint32_t) != 0)
      return ERROR_INVALID_PARAMETER;

   // Check data length validity
   if(data == NULL || address + length >= topAddress)
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

      if(address % AT25SF128A_SECTOR_SIZE == 0)
      {
         TRACE_INFO("Erasing SPI NOR Flash sector...\r\n");

         // Erase the Flash sector
         error = spiFlashErase(address, AT25SF128A_SECTOR_SIZE);
         // Is any error?
         if(error)
         {
            TRACE_ERROR("Failed to erase SPI NOR Flash sector!\r\n");
            return error;
         }
      }

      // Program 32-bit word in SPI Flash memory
      error = spiFlashWrite(address, word, sizeof(uint32_t));
      if(error)
      {
         TRACE_ERROR("Failed to write in flash memory!\r\n");
         return error;
      }

      // Advance data pointer
      p += n;
      // Increment word address
      address += n;
      // Remaining bytes to be written
      length -= n;
   }

   // Succcessful process
   return NO_ERROR;
}

/**
 * @brief Read data from Memory at the given address.
 * @param[in] address Address in Memory to read from
 * @param[in] data Buffer to store read data
 * @param[in] length Number of data bytes to read out
 * @return Error code
 **/

error_t at25sf128aFlashDriverRead(uint32_t address, uint8_t *data, size_t length)
{
   error_t error;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = AT25SF128A_ADDR + AT25SF128A_SIZE;

   // Check address validity
   if(address >= topAddress)
      return ERROR_INVALID_PARAMETER;

   // Check data length validity
   if(data == NULL || address + length >= topAddress)
      return ERROR_INVALID_PARAMETER;

   // Read data bytes from SPI Flash memory
   error = spiFlashRead(address, data, length);
   // Is any error?
   if(error)
   {
      TRACE_ERROR("Failed to read SPI NOR Flash!\r\n");
   }

   // Return status code
   return error;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
