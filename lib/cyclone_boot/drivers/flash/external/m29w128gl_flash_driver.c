/**
 * @file m29w128gl_flash_driver.c
 * @brief CycloneBOOT M29W128GL Flash Driver
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
#include "m29w128gl_flash_driver.h"

#include "core/flash.h"
#include "debug.h"
#include "stm324x9i_eval_nor.h"

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
   {M29W128GL_ADDR, M29W128GL_SUBSECTORS_SIZE, M29W128GL_SUBSECTORS_NUMBER}
};

// External Flash driver private related functions
error_t m29w128glFlashDriverInit(void);
error_t m29w128glFlashDriverDeInit(void);
error_t m29w128glFlashDriverGetInfo(const FlashInfo **info);
error_t m29w128glFlashDriverGetStatus(FlashStatus *status);
error_t m29w128glFlashDriverWrite(uint32_t address, uint8_t *data, size_t length);
error_t m29w128glFlashDriverRead(uint32_t address, uint8_t *data, size_t length);
error_t m29w128glFlashDriverErase(uint32_t address, size_t length);
error_t m29w128glFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr);
bool_t m29w128glFlashDriverIsSectorAddr(uint32_t address);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Memory Information
 **/

const FlashInfo m29w128glFlashDriverInfo = {
   FLASH_DRIVER_VERSION, M29W128GL_NAME, FLASH_TYPE_EXTERNAL_PARALLEL,
   // MEM_CLASS_FLASH,
   M29W128GL_ADDR, M29W128GL_SIZE, M29W128GL_WRITE_SIZE, M29W128GL_READ_SIZE, 0, 0, 0, 0, 0
};

/**
 * @brief Memory Driver
 **/

const FlashDriver m29w128glFlashDriver = {
   m29w128glFlashDriverInit, m29w128glFlashDriverDeInit,
   m29w128glFlashDriverGetInfo, m29w128glFlashDriverGetStatus,
   m29w128glFlashDriverWrite, m29w128glFlashDriverRead,
   m29w128glFlashDriverErase, NULL,
   m29w128glFlashDriverGetNextSector, m29w128glFlashDriverIsSectorAddr
};

/**
 * @brief Initialize Flash Memory.
 * @return Error code
 **/

error_t m29w128glFlashDriverInit(void)
{
   uint8_t status;

   // Debug message
   TRACE_INFO("Initializing %s memory...\r\n", M29W128GL_NAME);

   status = BSP_NOR_Init();
   if(status != NOR_STATUS_OK)
   {
      TRACE_ERROR("Failed to initialize Parallel NOR Flash!\r\n");
      return ERROR_FAILURE;
   }

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief De-Initialize Flash Memory.
 * @return Error code
 **/

error_t m29w128glFlashDriverDeInit(void) {
   return ERROR_NOT_IMPLEMENTED;
}

/**
 * @brief Get Flash Memory information.
 * @param[in,out] info Pointeur to the Memory information structure to be
 * returned
 * @return Error code
 **/

error_t m29w128glFlashDriverGetInfo(const FlashInfo **info)
{
   // Set Memory information pointeur
   *info = (const FlashInfo *)&m29w128glFlashDriverInfo;

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Get Flash Memory status.
 * @param[in,out] status Pointeur to the Memory status to be returned
 * @return Error code
 **/

error_t m29w128glFlashDriverGetStatus(FlashStatus *status)
{
   // Set Flash memory status
   *status = FLASH_STATUS_OK;

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

error_t m29w128glFlashDriverWrite(uint32_t address, uint8_t *data, size_t length)
{
   uint8_t status;
   uint32_t topAddress;
   const uint8_t *p;
   uint8_t word[M29W128GL_WRITE_SIZE];
   size_t n;

   // Precompute the top address
   topAddress = M29W128GL_ADDR + M29W128GL_SIZE;

   // Check address validity
   if((address >= topAddress) || (address % M29W128GL_WRITE_SIZE != 0))
      return ERROR_INVALID_PARAMETER;

   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;

   // Cast data pointer
   p = (const uint8_t *)data;

   // Perform write operation
   while(length > 0)
   {
      // Prevent to write more than allowed flash write bytes at a time
      n = MIN(sizeof(word), length);

      // Check if remaining bytes is less than required flash write size
      if(n < sizeof(word))
         memset(word, 0, sizeof(word));

      // Copy n bytes
      memcpy(word, p, n);

      // Is address match sector start address?
      if(address % M29W128GL_SUBSECTORS_SIZE == 0)
      {
         // Erases the specified block
         status = BSP_NOR_Erase_Block(address);
         // Is any error?
         if(status != NOR_STATUS_OK)
            return ERROR_FAILURE;
      }

      // Program data in flash memory
      status = BSP_NOR_WriteData(address, (uint16_t *)p, 1);
      // Is any error?
      if(status != NOR_STATUS_OK)
      {
         // Debug message
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

error_t m29w128glFlashDriverRead(uint32_t address, uint8_t *data, size_t length)
{
   uint8_t status;
   uint32_t topAddress;
   uint16_t tempData;

   // Precompute the top address
   topAddress = M29W128GL_ADDR + M29W128GL_SIZE;

   // Check address validity
   if(address >= topAddress)
      return ERROR_INVALID_PARAMETER;

   // Check parameters validity (is data in flash)
   if((data == NULL) || ((address + length) > topAddress))
      return ERROR_INVALID_PARAMETER;

   // Perform read operation
   status = BSP_NOR_ReadData(address, (uint16_t *)data, length / M29W128GL_READ_SIZE);
   // Is any error?
   if(status != NOR_STATUS_OK)
   {
      TRACE_ERROR("Failed to read from flash memory!\r\n");
      return ERROR_FAILURE;
   }

   // Number of bytes to be read is not a multiple of the minimal flash read size
   // A last read operation is needed
   if(length % M29W128GL_READ_SIZE)
   {
      // Perform last read
      status = BSP_NOR_ReadData(address + (length - 1), (uint16_t *)&tempData, 1);
      // Is any error?
      if(status != NOR_STATUS_OK)
      {
         TRACE_ERROR("Failed to read from flash memory!\r\n");
         return ERROR_FAILURE;
      }

      // Save last read bytes in data buffer
      memcpy(data + (length - 1), (uint8_t *)&tempData, 1);
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

error_t m29w128glFlashDriverErase(uint32_t address, size_t length)
{
   uint8_t status;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = M29W128GL_ADDR + M29W128GL_SIZE;

   // Check address validity
   if(address >= topAddress)
      return ERROR_INVALID_PARAMETER;

   // Check parameters validity (is data in flash)
   if((length == 0) || (address + length > topAddress))
      return ERROR_INVALID_PARAMETER;

   // Be sure address match a memory flash subsector start address
   if(address % M29W128GL_SUBSECTORS_SIZE != 0)
   {
      return ERROR_INVALID_PARAMETER;
   }

   // Perform erase operation
   while(length > 0)
   {
      // Erases the specified block
      status = BSP_NOR_Erase_Block(address);
      if(status != NOR_STATUS_OK)
      {
         TRACE_ERROR("Failed to erase flash memory block!\r\n");
         return ERROR_FAILURE;
      }

      // Increment word address
      address += M29W128GL_SUBSECTORS_SIZE;
      // Remaining bytes to be erased
      length -= MIN(length, M29W128GL_SUBSECTORS_SIZE);
   }

   // Successful process
   return NO_ERROR;
}

/**
 * @brief Get address of the neighbouring sector
 * @return Error code
 **/

error_t m29w128glFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr)
{
   uint_t i;
   uint_t j;
   SectorsGroup *sg;
   uint32_t sAddr = 0xFFFFFFFF;
   uint32_t lastSectorAddr;

   // Calculate last sector address
   lastSectorAddr = M29W128GL_ADDR + M29W128GL_SUBSECTORS_SIZE * (M29W128GL_SUBSECTORS_NUMBER - 1);

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

bool_t m29w128glFlashDriverIsSectorAddr(uint32_t address)
{
   // Is given address match a sector start address?
   if(address % M29W128GL_SUBSECTORS_SIZE == 0)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}
