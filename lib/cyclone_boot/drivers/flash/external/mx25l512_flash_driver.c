/**
 * @file mx25l512_flash_driver.c
 * @brief CycloneBOOT MX25L512 Flash Driver
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
#include "mx25l512_flash_driver.h"

#include "core/flash.h"
#include "debug.h"
#include "stm32f769i_discovery_qspi.h"

#define SECTORS_LIST_LEN 1

/**
 * @brief Sector group description
 **/

typedef struct
{
   uint32_t addr;  ///< Flash sector group address
   uint32_t size;  ///< Flash sector group cumulated size
   uint32_t nb;    ///< Number of flash sector in the Flash sector group
} SectorsGroup;

// External test memory sectors list
static const SectorsGroup sectorsList[SECTORS_LIST_LEN] = {
   {MX25L512_ADDR, MX25L512_SUBSECTORS_SIZE, MX25L512_SUBSECTORS_NUMBER}
};

// External Flash driver private related functions
error_t mx25l512FlashDriverInit(void);
error_t mx25l512FlashDriverDeInit(void);
error_t mx25l512FlashDriverGetInfo(const FlashInfo **info);
error_t mx25l512FlashDriverGetStatus(FlashStatus *status);
error_t mx25l512FlashDriverWrite(uint32_t address, uint8_t *data, size_t length);
error_t mx25l512FlashDriverRead(uint32_t address, uint8_t *data, size_t length);
error_t mx25l512FlashDriverErase(uint32_t address, size_t length);
error_t mx25l512FlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr);
bool_t mx25l512FlashDriverIsSectorAddr(uint32_t address);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Memory Information
 **/

const FlashInfo mx25l512FlashDriverInfo = {
   FLASH_DRIVER_VERSION, MX25L512_NAME, FLASH_TYPE_EXTERNAL_QSPI,
   // MEM_CLASS_FLASH,
   MX25L512_ADDR, MX25L512_SIZE, MX25L512_WRITE_SIZE, MX25L512_READ_SIZE, 0, 0, 0, 0, 0
};

/**
 * @brief Memory Driver
 **/

const FlashDriver mx25l512FlashDriver = {
   mx25l512FlashDriverInit, mx25l512FlashDriverDeInit,
   mx25l512FlashDriverGetInfo, mx25l512FlashDriverGetStatus,
   mx25l512FlashDriverWrite, mx25l512FlashDriverRead,
   mx25l512FlashDriverErase, NULL,
   mx25l512FlashDriverGetNextSector, mx25l512FlashDriverIsSectorAddr
};

/**
 * @brief Initialize Flash Memory.
 * @return Error code
 **/

error_t mx25l512FlashDriverInit(void)
{
   uint8_t status;

   // Debug message
   TRACE_INFO("Initializing %s memory...\r\n", MX25L512_NAME);

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
 * @brief De-Initialize Flash Memory.
 * @return Error code
 **/

error_t mx25l512FlashDriverDeInit(void)
{
   uint8_t status;

   // Debug message
   TRACE_INFO("Deinitializing %s memory...\r\n", MX25L512_NAME);

   status = BSP_QSPI_DeInit();
   if(status != QSPI_OK)
   {
      TRACE_ERROR("Failed to deinitialize QSPI NOR Flash!\r\n");
      return ERROR_FAILURE;
   }

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Get Flash Memory information.
 * @param[in,out] info Pointeur to the Memory information structure to be
 * returned
 * @return Error code
 **/

error_t mx25l512FlashDriverGetInfo(const FlashInfo **info)
{
   // Set Memory information pointeur
   *info = (const FlashInfo *)&mx25l512FlashDriverInfo;

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Get Flash Memory status.
 * @param[in,out] status Pointeur to the Memory status to be returned
 * @return Error code
 **/

error_t mx25l512FlashDriverGetStatus(FlashStatus *status)
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

error_t mx25l512FlashDriverWrite(uint32_t address, uint8_t *data, size_t length)
{
   uint8_t status;
   uint32_t topAddress;
   const uint8_t *p;
   uint8_t word[4];
   size_t n;

   // Precompute the top address
   topAddress = MX25L512_ADDR + MX25L512_SIZE;

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
      if(address % MX25L512_SUBSECTORS_SIZE == 0)
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
         return ERROR_FAILURE;
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

error_t mx25l512FlashDriverRead(uint32_t address, uint8_t *data, size_t length)
{
   uint8_t status;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = MX25L512_ADDR + MX25L512_SIZE;

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

error_t mx25l512FlashDriverErase(uint32_t address, size_t length)
{
   uint8_t status;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = MX25L512_ADDR + MX25L512_SIZE;

   // Check address validity
   if(address >= topAddress)
      return ERROR_INVALID_PARAMETER;

   // Check parameters validity (is data in flash)
   if((length == 0) || (address + length > topAddress))
      return ERROR_INVALID_PARAMETER;

   // Be sure address match a memory flash subsector start address
   if(address % MX25L512_SUBSECTORS_SIZE != 0)
   {
      return ERROR_INVALID_PARAMETER;
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
      address += MX25L512_SUBSECTORS_SIZE;
      // Remaining bytes to be erased
      length -= MIN(length, MX25L512_SUBSECTORS_SIZE);
   }

   // Successful process
   return NO_ERROR;
}

/**
 * @brief Get address of the neighbouring sector
 * @return Error code
 **/

error_t mx25l512FlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr)
{
   uint_t i;
   uint_t j;
   SectorsGroup *sg;
   uint32_t sAddr = 0xFFFFFFFF;
   uint32_t lastSectorAddr;

   // Calculate last sector address
   lastSectorAddr = MX25L512_ADDR + MX25L512_SUBSECTORS_SIZE * (MX25L512_SUBSECTORS_NUMBER - 1);

   // Check parameters validity
   if(address > lastSectorAddr || sectorAddr == NULL)
      return ERROR_INVALID_PARAMETER;

   // Loop through sectors list
   for(i = 0; i < SECTORS_LIST_LEN && sAddr == 0xFFFFFFFF; i++)
   {
      // Point to the current sectors group
      sg = (SectorsGroup *)&sectorsList[i];

      // Is address in current sector group
      if(address <= sg->addr + sg->size * sg->nb)
      {
         // Loop through sectors group list
         for(j = 0; j < sg->nb; j++)
         {
            // Is address located in current sector?
            if(address <= sg->addr + j * sg->size)
            {
               // Set next sector address
               sAddr = sg->addr + (j + 1) * sg->size;
               break;
            }
         }
      }
   }

   // Save next sector addr
   *sectorAddr = sAddr;

   // Succesfull process
   return NO_ERROR;
}

/**
 * @brief Determine if a given address is contained within a sector
 * @return boolean
 **/

bool_t mx25l512FlashDriverIsSectorAddr(uint32_t address)
{
   // Is given address match a sector start address?
   if(address % MX25L512_SUBSECTORS_SIZE == 0)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}
