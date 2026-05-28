/**
 * @file sam_ed_7x_flash_driver.c
 * @brief SAM(E|D)7x CycloneBOOT flash driver
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
#define TRACE_LEVEL TRACE_LEVEL_DEBUG // BOOT_TRACE_LEVEL

#include "sam_ed_7x_flash_driver.h"

#include "core/flash.h"
#include "debug.h"

/**
 * @brief Sector group description
 **/

typedef struct
{
   uint32_t addr;
   uint32_t size;
   uint32_t nb;
} SectorsGroup;

// Sector list size
#define SECTORS_LIST_LEN 1

// Internal test memory sectors list
static const SectorsGroup sectorsList[SECTORS_LIST_LEN] = {
   {SAM_ED_7x_FLASH_ADDR, SAM_ED_7x_FLASH_SECTOR_SIZE, SAM_ED_7x_FLASH_SECTOR_NUMBER}
};

error_t sam_ed_7xFlashDriverInit(void);
error_t sam_ed_7xFlashDriverDeInit(void);
error_t sam_ed_7xFlashDriverGetInfo(const FlashInfo **info);
error_t sam_ed_7xFlashDriverGetStatus(FlashStatus *status);
error_t sam_ed_7xFlashDriverWrite(uint32_t address, uint8_t *data, size_t length);
error_t sam_ed_7xFlashDriverRead(uint32_t address, uint8_t *data, size_t length);
error_t sam_ed_7xFlashDriverErase(uint32_t address, size_t length);
error_t sam_ed_7xFlashDriverSwapBanks(void);
error_t sam_ed_7xFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr);
bool_t sam_ed_7xFlashDriverIsSectorAddr(uint32_t address);

error_t flashDriverSwapBanksNoInit(void);
int_t sam_ed_7xFlashGetSector(uint32_t address);

error_t sam_ed_7xFlashDriverWriteQuadWord(uint32_t address, uint32_t *word);

const FlashDriver same7x_driver = {
   .init = sam_ed_7xFlashDriverInit,
   .deInit = sam_ed_7xFlashDriverDeInit,
   .getInfo = sam_ed_7xFlashDriverGetInfo,
   .getStatus = sam_ed_7xFlashDriverGetStatus,
   .write = sam_ed_7xFlashDriverWrite,
   .read = sam_ed_7xFlashDriverRead,
   .erase = sam_ed_7xFlashDriverErase,
   .swapBanks = sam_ed_7xFlashDriverSwapBanks,
   .getNextSectorAddr = sam_ed_7xFlashDriverGetNextSector,
   .isSectorAddr = sam_ed_7xFlashDriverIsSectorAddr,
};

/**
 * @brief Memory Information
 **/

const FlashInfo sam_ed_7x_FlashDriverInfo = {FLASH_DRIVER_VERSION, SAM_ED_7x_FLASH_NAME,
                                             FLASH_TYPE_INTERNAL,
                                             // MEM_CLASS_FLASH,
                                             SAM_ED_7x_FLASH_ADDR, SAM_ED_7x_FLASH_SIZE,
                                             SAM_ED_7x_FLASH_WRITE_SIZE, SAM_ED_7x_FLASH_READ_SIZE,
#ifndef FLASH_DB_MODE
                                             0, 0, 0, 0, 0
#else
                                             1, NULL, NULL, NULL, 0
#endif
};

/**
 * @brief Initialize Flash Memory.
 * @return Error code
 **/
error_t sam_ed_7xFlashDriverInit(void)
{
   // Debug message
   TRACE_INFO("Initializing %s driver...\r\n", SAM_ED_7x_FLASH_NAME);

   EFC_Initialize();

   // Successful processing
   return NO_ERROR;
}

/**
 * @brief De-Initialize Flash Memory.
 * @return Error code
 **/
error_t sam_ed_7xFlashDriverDeInit(void) {
   return ERROR_NOT_IMPLEMENTED;
}

/**
 * @brief Get Flash Memory information.
 * @param[in,out] info pointer to the Memory information structure to be
 * returned
 * @return Error code
 **/
error_t sam_ed_7xFlashDriverGetInfo(const FlashInfo **info)
{
   *info = (const FlashInfo *)&sam_ed_7x_FlashDriverInfo;
   return NO_ERROR;
}

/**
 * @brief Get Flash Memory status.
 * @param[in,out] status pointer to the Memory status to be returned
 * @return Error code
 **/
error_t sam_ed_7xFlashDriverGetStatus(FlashStatus *status)
{

   uint32_t efc_status = 0;

   if(status == NULL)
      return ERROR_INVALID_PARAMETER;

   efc_status |= EFC_REGS->EEFC_FSR;

   do
   {
      // Is any error flag set?
      if(efc_status != 0)
      {
         // Set Flash memory status
         *status = FLASH_STATUS_ERR;
         break;
      }

      // Get Flash memory busy flag
      efc_status |= EFC_REGS->EEFC_FSR;

      // Is busy flag set?
      if((efc_status & EEFC_FSR_FRDY_Msk) == 0)
      {
         // Set Flash memory status
         *status = FLASH_STATUS_BUSY;
         break;
      }

      // Set Flash memory status
      *status = FLASH_STATUS_OK;
   } while(0);

   return NO_ERROR;
}

/**
 * @brief Write data in Flash Memory at the given address.
 * @param[in] address Address in Flash Memory to write to
 * @param[in] data pointer to the data to write
 * @param[in] length Number of data bytes to write in
 * @return Error code
 **/

error_t sam_ed_7xFlashDriverWrite(uint32_t address, uint8_t *data, size_t length)
{
   error_t error;
   uint32_t topAddress;
   const uint8_t *p;
   uint8_t word[SAM_ED_7x_FLASH_WRITE_SIZE];
   size_t n;

   // Pre-compute the top address
   topAddress = SAM_ED_7x_FLASH_ADDR + SAM_ED_7x_FLASH_SIZE;

#if defined(__CC_ARM)
#pragma diag_suppress 186
#endif
   // Check address validity
   if((address < SAM_ED_7x_FLASH_ADDR || address >= topAddress) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#if defined(__CC_ARM)
#pragma diag_default 186
#endif

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#endif

   // Debug message
   // TRACE_DEBUG("Writing data (%d bytes) at 0x%08X\r\n", length, address);

   // Cast data pointer
   p = (const uint8_t *)data;

   // Perform write operation
   while(length > 0)
   {
      // Prevent to write more than 16 bytes at a time (quad-word write)
      n = MIN(sizeof(word), length);

      // Check if remaining bytes is less than 16 (128bits word)
      if(n < sizeof(word))
         memset(word, 0, sizeof(word));

      // Copy n bytes
      memcpy(word, p, n);

      // Program 32-bit word in flash memory
      error = sam_ed_7xFlashDriverWriteQuadWord(address, (uint32_t *)word);
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
error_t sam_ed_7xFlashDriverRead(uint32_t address, uint8_t *data, size_t length)
{
   uint_t i;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = SAM_ED_7x_FLASH_ADDR + SAM_ED_7x_FLASH_SIZE;

#if defined(__CC_ARM)
#pragma diag_suppress 186
#endif
   // Check address validity
   if(address < SAM_ED_7x_FLASH_ADDR || address >= topAddress)
      return ERROR_INVALID_PARAMETER;
#if defined(__CC_ARM)
#pragma diag_default 186
#endif

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#endif

   // Perform read operation
   // for(i = 0; i < length; i++)
   //{
   //*((uint8_t *)data + i) = *(uint8_t*)address;
   // address++;
   // }

   EFC_Read((uint32_t *)data, length, address);

   // Successful process
   return NO_ERROR;
}

/**
 * @brief Erase data from Memory at the given address.
 * @param[in] address Address in Memory to start erasing from
 * @param[in] length Number of data bytes to be erased
 * @return Error code
 **/
error_t sam_ed_7xFlashDriverErase(uint32_t address, size_t length)
{
   uint_t i;
   uint_t j;
   SectorsGroup *sGp;
   uint32_t sectorGroupAddr;
   uint32_t topAddress;
   uint32_t tempSectorAddr;
   size_t tempSectorSize;

   // Precompute the top address
   topAddress = SAM_ED_7x_FLASH_ADDR + SAM_ED_7x_FLASH_SIZE;

#if defined(__CC_ARM)
#pragma diag_suppress 186
#endif
   // Check address validity
   if((address < SAM_ED_7x_FLASH_ADDR || address >= topAddress) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#if defined(__CC_ARM)
#pragma diag_default 186
#endif

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if((length == 0) || (address + length > topAddress))
      return ERROR_INVALID_PARAMETER;
#endif

   // init first erase sector address
   tempSectorAddr = sectorsList[0].addr;

   // get first erase sector address
   for(i = 0; i < SECTORS_LIST_LEN; i++)
   {
      // Point to the current sector group
      sGp = (SectorsGroup *)&sectorsList[i];

      for(j = 0; j < sGp->nb; j++)
      {
         // Get current sector address
         sectorGroupAddr = sGp->addr + (j * sGp->size);

         if(address == sectorGroupAddr)
         {
            // Update first erase sector address
            tempSectorAddr = sectorGroupAddr;
            break;
         }
         else if(address < sectorGroupAddr)
         {
            // Update first erase sector address
            tempSectorAddr = sGp->addr + ((j - 1) * sGp->size);
            // Update length of the erase block
            length += address - tempSectorAddr;
            break;
         }
         else
         {
            // For sanity
         }
      }
   }

   // Perform erase operation
   while(length > 0)
   {

      TRACE_DEBUG("Erasing Flash address 0x%08x...\r\n", tempSectorAddr);

      // Unlock region
      EFC_RegionUnlock(tempSectorAddr);

      // Erase sector
      EFC_SectorErase(tempSectorAddr);

      // Get size of the erased sector
      for(i = 0; i < SECTORS_LIST_LEN; i++)
      {
         // Point to the current sector group
         sGp = (SectorsGroup *)&sectorsList[i];

         // Is erased sector inside of the current sector group?
         if(tempSectorAddr >= sGp->addr || tempSectorAddr < (sGp->addr + (sGp->nb * sGp->size)))
         {
            // Set erased sector size
            tempSectorSize = sGp->size;
            break;
         }
      }

      TRACE_DEBUG("Erased %" PRIu32 " bytes...\r\n", tempSectorSize);

      // Lock region
      EFC_RegionLock(tempSectorAddr);

      // Update address to match the next sector to be erased
      tempSectorAddr += tempSectorSize;
      // Update length of the remaining block to be erase
      length -= MIN(length, tempSectorSize);
   }

   // Successful process
   return NO_ERROR;
}

/**
 * @brief Performs a Memory bank swap according to the current bank ID.
 * If current Memory bank ID match the 1st bank then it will swap on the 2nd
 * Memory bank. If current Memory bank ID match the 2nd bank then it will swap on
 * the 1st Memory bank.
 * @return Error code
 **/
error_t sam_ed_7xFlashDriverSwapBanks(void) {
   return ERROR_NOT_IMPLEMENTED;
}

/**
 * @brief Performs a Memory bank swap according to the current bank ID without
 * HAL initialization. It uses CMSIS low layer instead of HAL. If current Memory
 * bank ID match the 1st bank then it will swap on the 2nd Memory bank. If
 * current Memory bank ID match the 2nd bank then it will swap on the 1st Memory
 * bank.
 * @return Error code
 **/

error_t flashDriverSwapBanksNoInit(void) {
   return ERROR_NOT_IMPLEMENTED;
}

/**
 * @brief Get address of the neighbouring sector
 * @return Error code
 **/
error_t sam_ed_7xFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr)
{
   uint_t i;
   uint_t j;
   SectorsGroup *sg;
   uint32_t sAddr = 0xFFFFFFFF;
   uint32_t lastSectorAddr;

   lastSectorAddr =
      sectorsList[SECTORS_LIST_LEN - 1].addr +
      (sectorsList[SECTORS_LIST_LEN - 1].size * (sectorsList[SECTORS_LIST_LEN - 1].nb - 1));

#if defined(__CC_ARM)
#pragma diag_suppress 186
#endif
   // Check parameters validity
   if(address < SAM_ED_7x_FLASH_ADDR || address > lastSectorAddr || sectorAddr == NULL)
      return ERROR_INVALID_PARAMETER;
#if defined(__CC_ARM)
#pragma diag_default 186
#endif

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
               sAddr = sg->addr + j * sg->size;
               break;
            }
         }
      }
   }

   // Save next sector addr
   *sectorAddr = sAddr;

   // Successful process
   return NO_ERROR;
}

/**
 * @brief Determine if a given address is contained within a sector
 * @return boolean
 **/
bool_t sam_ed_7xFlashDriverIsSectorAddr(uint32_t address)
{
   if((address < MEMORY_LOWER_BOUND) || (address > MEMORY_UPPER_BOUND))
   {
      return ERROR_FAILURE;
   }

   int_t sector;

   // Get Flash memory sector number
   sector = sam_ed_7xFlashGetSector(address);

   // Is given address match a sector start address?
   if(sector >= 0)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

/**
 * @brief Get the Memory Flash sector number according to the given address.
 * @param[in] address Given Flash Memory address
 * @return Sector number or -1
 **/

int_t sam_ed_7xFlashGetSector(uint32_t address)
{
   uint_t i;
   uint_t j;
   int_t sector;
   SectorsGroup *sGroup;

   // Initialize sector number
   sector = -1;

   // Loop through flash sector group list
   for(i = 0; i < SECTORS_LIST_LEN; i++)
   {
      // Point to the current sector group
      sGroup = (SectorsGroup *)&sectorsList[i];

      // Loop through sector group sectors list
      for(j = 0; j < sGroup->nb; j++)
      {
         // Is current sector address matches given address?
         if(sGroup->addr + sGroup->size * j == address)
         {
            sector = j;
         }
      }
   }

   return sector;
}

/**
 * @brief Write 32-bits word in Flash Memory at the given address.
 * @param[in] address Address in Flash Memory to write to
 * @param[in] word 32-bit word to write in Flash memory
 * @return Error code
 **/

error_t sam_ed_7xFlashDriverWriteQuadWord(uint32_t address, uint32_t *word)
{
   int_t sector;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = SAM_ED_7x_FLASH_ADDR + SAM_ED_7x_FLASH_SIZE;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if((address + sizeof(uint32_t) >= topAddress) || (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#endif

   // Get Flash memory sector number
   sector = sam_ed_7xFlashGetSector(address);

   // Check whether the address match the beginning of a Flash sector.
   // If this is the case then the flash sector must be erased before any write
   // operation
   if(sector >= 0)
   {
      // Debug message
      TRACE_DEBUG("Erasing Flash sector %" PRIu32 "before writing...\r\n", sector);

      // Unlock flash region
      EFC_RegionUnlock(address);

      // while(EFC_IsBusy());

      // Erase block (sector)
      EFC_SectorErase(address);

      // while(EFC_IsBusy());

      // Lock flash region
      EFC_RegionLock(address);

      // while(EFC_IsBusy());
   }

   // Start of exception handling block
   do
   {
      // Unlock flash region
      EFC_RegionUnlock(address);

      // while(EFC_IsBusy());

      // Write quad-word data
      // EFC_QuadWordWrite(word, address);
      EFC_PageWrite(word, address);

      // while(EFC_IsBusy());

      // Unlock flash region
      EFC_RegionLock(address);

      // while(EFC_IsBusy());

   } while(0);

   // Return status code
   return NO_ERROR;
}
