/**
 * @file sam_ed_5x_flash_driver.c
 * @brief SAM(E|D)5x CycloneBOOT flash driver
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
#include "sam_ed_5x_flash_driver.h"

#include "core/flash.h"
#include "debug.h"
#include "sam.h"

// Macro to send command to NVMCTRL_REGS
#define NVMCTRL_SEND_CMD(cmd)                                                                      \
        (NVMCTRL_REGS->NVMCTRL_CTRLB = (uint32_t)(cmd | NVMCTRL_CTRLB_CMDEX_KEY))

// Sector list size
#define SECTORS_LIST_LEN 1

/**
 * @brief Sector group description
 **/

typedef struct
{
   uint32_t addr;
   uint32_t size;
   uint32_t nb;
} SectorsGroup;

// Internal test memory sectors list
static const SectorsGroup sectorsList[SECTORS_LIST_LEN] = {
   {SAM_ED_5x_FLASH_ADDR, SAM_ED_5x_FLASH_SECTOR_SIZE, SAM_ED_5x_FLASH_SECTOR_NUMBER}
};

// Memory driver private related functions
error_t sam_ed_5xFlashDriverInit(void);
error_t sam_ed_5xFlashDriverDeInit(void);
error_t sam_ed_5xFlashDriverGetInfo(const FlashInfo **info);
error_t sam_ed_5xFlashDriverGetStatus(FlashStatus *status);
error_t sam_ed_5xFlashDriverWrite(uint32_t address, uint8_t *data, size_t length);
error_t sam_ed_5xFlashDriverRead(uint32_t address, uint8_t *data, size_t length);
error_t sam_ed_5xFlashDriverErase(uint32_t address, size_t length);
error_t sam_ed_5xFlashDriverSwapBanks(void) __attribute__((section(".code_in_ram")));
error_t sam_ed_5xFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr);
bool_t sam_ed_5xFlashDriverIsSectorAddr(uint32_t address);
error_t sam_ed_5xFlashDriverWriteQuadWord(uint32_t address, uint32_t *word);
int_t sam_ed_5xFlashGetSector(uint32_t address);
error_t flashGetCurrentBank(uint8_t *fBankID);
error_t sam_ed_5xFlashDriverEraseSector(uint32_t firstSector, size_t nbSector);

// Simple NVM controller HAL api
void nvmInit(void);
void nvmUnlockRegion(uint32_t address);
void nvmLockRegion(uint32_t address);
void nvmEraseBlock(uint32_t address);
void nvmWriteQuadWord(const uint32_t *data, const uint32_t address);
void nvmSwapBanks(void);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Memory Information
 **/

const FlashInfo sam_ed_5x_FlashDriverInfo = {FLASH_DRIVER_VERSION, SAM_ED_5x_FLASH_NAME,
                                             FLASH_TYPE_INTERNAL,
                                             // MEM_CLASS_FLASH,
                                             SAM_ED_5x_FLASH_ADDR, SAM_ED_5x_FLASH_SIZE,
                                             SAM_ED_5x_FLASH_WRITE_SIZE, SAM_ED_5x_FLASH_READ_SIZE,
#ifndef FLASH_DB_MODE
                                             0, 0, 0, 0, 0
#else
                                             1, SAM_ED_5x_FLASH_BANK_SIZE,
                                             SAM_ED_5x_FLASH_BANK1_ADDR, SAM_ED_5x_FLASH_BANK2_ADDR,
                                             0
#endif
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief STM32F4xx Flash driver
 **/

const FlashDriver sam_ed_5x_FlashDriver = {sam_ed_5xFlashDriverInit,
                                           sam_ed_5xFlashDriverDeInit,
                                           sam_ed_5xFlashDriverGetInfo,
                                           sam_ed_5xFlashDriverGetStatus,
                                           sam_ed_5xFlashDriverWrite,
                                           sam_ed_5xFlashDriverRead,
                                           sam_ed_5xFlashDriverErase,
#ifndef FLASH_DB_MODE
                                           NULL,
#else
                                           sam_ed_5xFlashDriverSwapBanks,
#endif
                                           sam_ed_5xFlashDriverGetNextSector,
                                           sam_ed_5xFlashDriverIsSectorAddr};

/**
 * @brief Initialize Flash Memory.
 * @return Error code
 **/

error_t sam_ed_5xFlashDriverInit(void)
{
   // Debug message
   TRACE_INFO("Initializing %s memory...\r\n", SAM_ED_5x_FLASH_NAME);

   // Intialize flash NVM controller
   nvmInit();

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Initialize Flash Memory.
 * @return Error code
 **/

error_t sam_ed_5xFlashDriverDeInit(void)
{
   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Get Flash Memory information.
 * @param[in,out] info Pointeur to the Memory information structure to be
 * returned
 * @return Error code
 **/

error_t sam_ed_5xFlashDriverGetInfo(const FlashInfo **info)
{
   // Set Memory information pointeur
   *info = (const FlashInfo *)&sam_ed_5x_FlashDriverInfo;

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Get Flash Memory status.
 * @param[in,out] status Pointeur to the Memory status to be returned
 * @return Error code
 **/

error_t sam_ed_5xFlashDriverGetStatus(FlashStatus *status)
{
   uint32_t flag;

   // Check parameter vailidity
   if(status == NULL)
      return ERROR_INVALID_PARAMETER;

   // Get Flash Memory error flags
   flag = (NVMCTRL_REGS->NVMCTRL_INTFLAG &
      (NVMCTRL_INTFLAG_ADDRE_Msk | NVMCTRL_INTFLAG_ECCDE_Msk | NVMCTRL_INTFLAG_ECCSE_Msk |
      NVMCTRL_INTFLAG_LOCKE_Msk | NVMCTRL_INTFLAG_NVME_Msk | NVMCTRL_INTFLAG_PROGE_Msk));

   do
   {
      // Is any error flag set?
      if(flag != 0)
      {
         // Set Flash memory status
         *status = FLASH_STATUS_ERR;
         break;
      }

      // Get Flash memory busy flag
      flag = NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_READY_Msk;

      // Is busy flag set?
      if(flag == 0)
      {
         // Set Flash memory status
         *status = FLASH_STATUS_BUSY;
         break;
      }

      // Set Flash memory status
      *status = FLASH_STATUS_OK;
   } while(0);

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

error_t sam_ed_5xFlashDriverWrite(uint32_t address, uint8_t *data, size_t length)
{
   error_t error;
   uint32_t topAddress;
   const uint8_t *p;
   uint8_t word[SAM_ED_5x_FLASH_WRITE_SIZE];
   size_t n;

   // Precompute the top address
   topAddress = SAM_ED_5x_FLASH_ADDR + SAM_ED_5x_FLASH_SIZE;

#if defined(__CC_ARM)
#pragma diag_suppress 186
#endif
   // Check address validity
   if((address < SAM_ED_5x_FLASH_ADDR || address >= topAddress) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#if defined(__CC_ARM)
#pragma diag_default 186
#endif

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#else
#if defined(__CC_ARM)
#pragma diag_suppress 186
#endif
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(data == NULL ||
      !((address >= SAM_ED_5x_FLASH_BANK1_ADDR &&
      address + length <= SAM_ED_5x_FLASH_BANK2_ADDR) ||
      (address >= SAM_ED_5x_FLASH_BANK2_ADDR && address + length <= topAddress)))
      return ERROR_INVALID_PARAMETER;
#if defined(__CC_ARM)
#pragma diag_default 186
#endif
#endif

   // Debug message
   TRACE_DEBUG("Writing data (%d bytes) at 0x%08X\r\n", length, address);
   TRACE_DEBUG_ARRAY("WRITE DATA: ", data, length);

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
      error = sam_ed_5xFlashDriverWriteQuadWord(address, (uint32_t *)word);
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

error_t sam_ed_5xFlashDriverRead(uint32_t address, uint8_t *data, size_t length)
{
   uint_t i;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = SAM_ED_5x_FLASH_ADDR + SAM_ED_5x_FLASH_SIZE;

#if defined(__CC_ARM)
#pragma diag_suppress 186
#endif
   // Check address validity
   if(address < SAM_ED_5x_FLASH_ADDR || address >= topAddress)
      return ERROR_INVALID_PARAMETER;
#if defined(__CC_ARM)
#pragma diag_default 186
#endif

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#else
#if defined(__CC_ARM)
#pragma diag_suppress 186
#endif
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(data == NULL ||
      !((address >= SAM_ED_5x_FLASH_BANK1_ADDR &&
      address + length <= SAM_ED_5x_FLASH_BANK2_ADDR) ||
      (address >= SAM_ED_5x_FLASH_BANK2_ADDR && address + length <= topAddress)))
      return ERROR_INVALID_PARAMETER;
#if defined(__CC_ARM)
#pragma diag_default 186
#endif
#endif

   // Perform read operation
   for(i = 0; i < length; i++)
   {
      *((uint8_t *)data + i) = *(uint8_t *)address;
      address++;
   }

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Erase data from Memory at the given address.
 * @param[in] address Address in Memory to start erasing from
 * @param[in] length Number of data bytes to be erased
 * @return Error code
 **/
error_t sam_ed_5xFlashDriverErase(uint32_t address, size_t length)
{
   uint_t i;
   uint_t j;
   SectorsGroup *sGp;
   uint32_t sectorGroupAddr;
   uint32_t topAddress;
   uint32_t tempSectorAddr;
   size_t tempSectorSize;

   // Precompute the top address
   topAddress = SAM_ED_5x_FLASH_ADDR + SAM_ED_5x_FLASH_SIZE;

#if defined(__CC_ARM)
#pragma diag_suppress 186
#endif
   // Check address validity
   if((address < SAM_ED_5x_FLASH_ADDR || address >= topAddress) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#if defined(__CC_ARM)
#pragma diag_default 186
#endif

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if((length == 0) || (address + length > topAddress))
      return ERROR_INVALID_PARAMETER;
#else
#if defined(__CC_ARM)
#pragma diag_suppress 186
#endif
   // Check parameters validity (is address in flash bank 1 or 2 only)
   if((length == 0) ||
      !((address >= SAM_ED_5x_FLASH_BANK1_ADDR &&
      address + length <= SAM_ED_5x_FLASH_BANK2_ADDR) ||
      (address >= SAM_ED_5x_FLASH_BANK2_ADDR && address + length <= topAddress)))
      return ERROR_INVALID_PARAMETER;
#if defined(__CC_ARM)
#pragma diag_default 186
#endif
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
            // Update lenght of the erase block
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
      // Unlock region
      nvmUnlockRegion(tempSectorAddr);

      // Erase sector
      nvmEraseBlock(tempSectorAddr);

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

      // Lock region
      nvmLockRegion(tempSectorAddr);

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

error_t sam_ed_5xFlashDriverSwapBanks(void)
{
#if defined(FLASH_DB_MODE)
   // Debug message
   TRACE_INFO("Swaping device flash bank...\r\n");

   // Swap in flash bank 2
   if(NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_AFIRST_Msk)
   {
      // Debug message
      TRACE_DEBUG("Swaping from flask bank 1 to flash bank 2...\r\n");
   }
   // Swap in flash bank 1
   else
   {
      // Debug message
      TRACE_DEBUG("Swaping from flask bank 2 to flash bank 1...\r\n");
   }

   // Swap flash banks
   nvmSwapBanks();

   // Return status code
   return NO_ERROR;
#else
   // Return status code
   return ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief Performs a Memory bank swap according to the current bank ID without
 * HAL initialization. It uses CMSIS low layer instead of HAL. If current Memory
 * bank ID match the 1st bank then it will swap on the 2nd Memory bank. If
 * current Memory bank ID match the 2nd bank then it will swap on the 1st Memory
 * bank.
 * @return Error code
 **/

error_t flashDriverSwapBanksNoInit(void)
{
   error_t error;

   // Initialize status code
   error = NO_ERROR;

   // Swap flash banks
   nvmSwapBanks();

   // Return status code
   return error;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Get address of the neighbouring sector
 * @return Error code
 **/

error_t sam_ed_5xFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr)
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
   if(address < SAM_ED_5x_FLASH_ADDR || address > lastSectorAddr || sectorAddr == NULL)
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

   // Succesfull process
   return NO_ERROR;
}

/**
 * @brief Determine if a given address is contained within a sector
 * @return boolean
 **/

bool_t sam_ed_5xFlashDriverIsSectorAddr(uint32_t address)
{
   int_t sector;

   // Get Flash memory sector number
   sector = sam_ed_5xFlashGetSector(address);

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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Write 32-bits word in Flash Memory at the given address.
 * @param[in] address Address in Flash Memory to write to
 * @param[in] word 32-bit word to write in Flash memory
 * @return Error code
 **/

error_t sam_ed_5xFlashDriverWriteQuadWord(uint32_t address, uint32_t *word)
{
   int_t sector;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = SAM_ED_5x_FLASH_ADDR + SAM_ED_5x_FLASH_SIZE;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if((address + sizeof(uint32_t) >= topAddress) || (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#else
#if defined(__CC_ARM)
#pragma diag_suppress 186
#endif
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(!((address >= SAM_ED_5x_FLASH_BANK1_ADDR &&
      address + sizeof(uint32_t) <= SAM_ED_5x_FLASH_BANK2_ADDR) ||
      (address >= SAM_ED_5x_FLASH_BANK2_ADDR && address + sizeof(uint32_t) <= topAddress)) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#if defined(__CC_ARM)
#pragma diag_default 186
#endif
#endif

   // Get Flash memory sector number
   sector = sam_ed_5xFlashGetSector(address);

   // Check whether the address match the beginning of a Flash sector.
   // If this is the case then the flash sector must be erased before any write
   // operation
   if(sector >= 0)
   {
      // Debug message
      TRACE_DEBUG("Erasing Flash sector %" PRIu32 "...\r\n", sector);

      // Unlock flash region
      nvmUnlockRegion(address);

      // Erase block (sector)
      nvmEraseBlock(address);

      // Lock flash region
      nvmLockRegion(address);
   }

   // Start of exception handling block
   do
   {
      // Unlock flash region
      nvmUnlockRegion(address);

      // Write quad-word data
      nvmWriteQuadWord(word, address);

      // Unlock flash region
      nvmLockRegion(address);

   } while(0);

   // Return status code
   return NO_ERROR;
}

/**
 * @brief Get the Memory Flash sector number according to the given address.
 * @param[in] address Given Flash Memory address
 * @return Sector number or -1
 **/

int_t sam_ed_5xFlashGetSector(uint32_t address)
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

#ifdef FLASH_DB_MODE

/**
 * @brief Get current used flash bank (SAM_ED_5x_FLASH_BANK_1_ID or
 * SAM_ED_5x_FLASH_BANK_2_ID).
 * @param[out] fBankID Pointer to the flash bank ID to be retrieved
 * @return Status code
 **/

error_t flashGetCurrentBank(uint8_t *fBankID)
{
   if(NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_AFIRST_Msk)
   {
      *fBankID = SAM_ED_5x_FLASH_BANK1_ID;
   }
   else
   {
      *fBankID = SAM_ED_5x_FLASH_BANK2_ID;
   }

   // Successful process
   return NO_ERROR;
}

#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Initialize NVM controller
 **/

void nvmInit(void)
{
   // Enable NVM automatic wait state generation
   NVMCTRL_REGS->NVMCTRL_CTRLA |= NVMCTRL_CTRLA_AUTOWS_Msk;

   // Set read wait state to 6 (120MHz CPU max clock)
   NVMCTRL_REGS->NVMCTRL_CTRLA |= 6 << NVMCTRL_CTRLA_RWS_Pos;

   // Disable NVM suspend operation
   NVMCTRL_REGS->NVMCTRL_CTRLA |= (0 << NVMCTRL_CTRLA_SUSPEN_Pos);

   // Configure NVM in manual write mode
   NVMCTRL_REGS->NVMCTRL_CTRLA |= NVMCTRL_CTRLA_WMODE_MAN;
}

/**
 * @brief Unlock Flash region according the given address.
 * @param[in] address Address of the region to be unlocked.
 **/

void nvmUnlockRegion(uint32_t address)
{
   // Wait NVMCTRL_REGS to be ready
   while(!(NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_READY_Msk))
      ;

   // Set address location of the region to be unlocked
   NVMCTRL_REGS->NVMCTRL_ADDR = address;

   // Send NVMCTRL_REGS unlock region command
   NVMCTRL_SEND_CMD(NVMCTRL_CTRLB_CMD_UR);

   // Wait NVMCTRL_REGS to be ready
   while(!(NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_READY_Msk))
      ;

   // Clear DONE INTFLAG
   NVMCTRL_REGS->NVMCTRL_INTFLAG |= NVMCTRL_INTFLAG_DONE_Msk;
}

/**
 * @brief Lock Flash region according the given address.
 * @param[in] address Address of the region to be locked.
 **/

void nvmLockRegion(uint32_t address)
{
   // Wait NVMCTRL_REGS to be ready
   while(!(NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_READY_Msk))
      ;

   // Set address location of the region to be unlocked
   NVMCTRL_REGS->NVMCTRL_ADDR = address;

   // Send NVMCTRL_REGS lock region command
   NVMCTRL_SEND_CMD(NVMCTRL_CTRLB_CMD_LR);

   // Wait NVMCTRL_REGS to be ready
   while(!(NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_READY_Msk))
      ;

   // Clear DONE INTFLAG
   NVMCTRL_REGS->NVMCTRL_INTFLAG |= NVMCTRL_INTFLAG_DONE_Msk;
}

/**
 * @brief Erase flash block (sector) corresponding to the given address.
 * @param[in] address Address of the block to be erased.
 **/

void nvmEraseBlock(uint32_t address)
{
   // Wait NVMCTRL_REGS to be ready
   while(!(NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_READY_Msk))
      ;

   // Set address location of the block to be erased
   NVMCTRL_REGS->NVMCTRL_ADDR = address;

   // Send NVMCTRL_REGS erase block command
   NVMCTRL_SEND_CMD(NVMCTRL_CTRLB_CMD_EB);

   // Wait NVMCTRL_REGS to be ready
   while(!(NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_READY_Msk))
      ;

   // Clear DONE INTFLAG
   NVMCTRL_REGS->NVMCTRL_INTFLAG |= NVMCTRL_INTFLAG_DONE_Msk;
}

/**
 * @brief Write Quad-Word (16Bytes) data in the flash memory at the given
 * address.
 * @param[in] data Pointer to the quad-word data to be written.
 * @param[in] address Address where to write the data.
 **/

void nvmWriteQuadWord(const uint32_t *data, const uint32_t address)
{
   uint_t i;
   uint32_t *pbWriteAddr;

   // Initialize page buffer address pointer
   pbWriteAddr = (uint32_t *)address;

   // Wait NVMCTRL_REGS to be ready
   while(!(NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_READY_Msk))
      ;

   // Clear page buffer
   NVMCTRL_SEND_CMD(NVMCTRL_CTRLB_CMD_PBC);

   // Wait NVMCTRL_REGS to be ready
   while(!(NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_READY_Msk))
      ;

   // Clear DONE INTFLAG
   NVMCTRL_REGS->NVMCTRL_INTFLAG |= NVMCTRL_INTFLAG_DONE_Msk;

   // Loop to write one word at a time.
   for(i = 0; i < 4; i++)
   {
      *pbWriteAddr++ = data[i];
   }

   // Write page buffer data in memory
   NVMCTRL_SEND_CMD(NVMCTRL_CTRLB_CMD_WQW);

   // Wait NVMCTRL_REGS to be ready
   while(!(NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_READY_Msk))
      ;

   // Clear DONE INTFLAG
   NVMCTRL_REGS->NVMCTRL_INTFLAG |= NVMCTRL_INTFLAG_DONE_Msk;
}

/**
 * @brief Swap Flash banks
 **/

void nvmSwapBanks(void)
{
   // Wait NVMCTRL_REGS to be ready
   while(!NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_READY_Msk)
      ;

   // Swap NVM flash banks
   NVMCTRL_SEND_CMD(NVMCTRL_CTRLB_CMD_BKSWRST);
}
