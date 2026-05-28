/**
 * @file stm32l4xx_flash_driver.c
 * @brief CycloneBOOT STM32L4xx Flash Driver
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
#include "stm32l4xx_flash_driver.h"

#include "core/flash.h"
#include "debug.h"
#include "stm32l4xx.h"
#include "stm32l4xx_hal.h"

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

// Internal flash memory sectors list
static const SectorsGroup sectorsList[SECTORS_LIST_LEN] = {
#if !defined(FLASH_DB_MODE)
   {STM32L4xx_FLASH_ADDR, STM32L4xx_FLASH_SECTOR_SIZE, STM32L4xx_FLASH_SECTOR_NUMBER}
#else
   {STM32L4xx_FLASH_ADDR, STM32L4xx_FLASH_SECTOR_SIZE, STM32L4xx_FLASH_SECTOR_NUMBER}
#endif
};

// Memory driver private related functions
error_t stm32l4xxFlashDriverInit(void);
error_t stm32l4xxFlashDriverDeInit(void);
error_t stm32l4xxFlashDriverGetInfo(const FlashInfo **info);
error_t stm32l4xxFlashDriverGetStatus(FlashStatus *status);
error_t stm32l4xxFlashDriverWrite(uint32_t address, uint8_t *data, size_t length);
error_t stm32l4xxFlashDriverRead(uint32_t address, uint8_t *data, size_t length);
error_t stm32l4xxFlashDriverErase(uint32_t address, size_t length);
error_t stm32l4xxFlashDriverSwapBanks(void) __attribute__((section(".code_in_ram")));
error_t stm32l4xxFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr);
bool_t stm32l4xxFlashDriverIsSectorAddr(uint32_t address);
error_t stm32l4xxFlashDriverWriteWord(uint32_t address, uint32_t word);
int_t stm32l4xxFlashGetSector(uint32_t address);
error_t flashGetCurrentBank(uint8_t *fBankID);

// error_t stm32l4xxFlashDriverEraseSector(uint32_t firstSector, size_t
// nbSector);
error_t stm32l4xxFlashDriverEraseSector(uint32_t bankID, uint32_t firstSector, size_t nbSectors);

static uint32_t GetBank(uint32_t Addr);
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Memory Information
 **/

const FlashInfo stm32l4xxFlashDriverInfo = {FLASH_DRIVER_VERSION, STM32L4xx_FLASH_NAME,
                                            FLASH_TYPE_INTERNAL,
                                            // MEM_CLASS_FLASH,
                                            STM32L4xx_FLASH_ADDR, STM32L4xx_FLASH_SIZE,
                                            STM32L4xx_FLASH_WRITE_SIZE, STM32L4xx_FLASH_READ_SIZE,
#ifndef FLASH_DB_MODE
                                            0, 0, 0, 0, 0
#else
                                            1, STM32L4xx_FLASH_BANK_SIZE,
                                            STM32L4xx_FLASH_BANK1_ADDR, STM32L4xx_FLASH_BANK2_ADDR,
                                            FLASH_FLAGS_LATER_SWAP
#endif
};

/**
 * @brief stm32l4xx Flash driver
 **/

const FlashDriver stm32l4xxFlashDriver = {stm32l4xxFlashDriverInit,
                                          stm32l4xxFlashDriverDeInit,
                                          stm32l4xxFlashDriverGetInfo,
                                          stm32l4xxFlashDriverGetStatus,
                                          stm32l4xxFlashDriverWrite,
                                          stm32l4xxFlashDriverRead,
                                          stm32l4xxFlashDriverErase,
#ifndef FLASH_DB_MODE
                                          NULL,
#else
                                          stm32l4xxFlashDriverSwapBanks,
#endif
                                          stm32l4xxFlashDriverGetNextSector,
                                          stm32l4xxFlashDriverIsSectorAddr};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Initialize Flash Memory.
 * @return Error code
 **/

error_t stm32l4xxFlashDriverInit(void)
{
   // Debug message
   TRACE_INFO("Initializing %s memory...\r\n", STM32L4xx_FLASH_NAME);

   // Wait for last flash operation on flash
   FLASH_WaitForLastOperation(50);
   // Clear all flash error flags
   __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief DeInitialize Flash Memory.
 * @return Error code
 **/

error_t stm32l4xxFlashDriverDeInit(void)
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

error_t stm32l4xxFlashDriverGetInfo(const FlashInfo **info)
{
   // Set Memory information pointeur
   *info = (const FlashInfo *)&stm32l4xxFlashDriverInfo;

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Get Flash Memory status.
 * @param[in,out] status Pointeur to the Memory status to be returned
 * @return Error code
 **/

error_t stm32l4xxFlashDriverGetStatus(FlashStatus *status)
{
   uint32_t flag;

   // Check parameter vailidity
   if(status == NULL)
      return ERROR_INVALID_PARAMETER;

   do
   {
      // Get Flash Memory error flags status
      flag = __HAL_FLASH_GET_FLAG(FLASH_FLAG_ALL_ERRORS);
      // Is any error flag set?
      if(flag != RESET)
      {
         // Set Flash memory status
         *status = FLASH_STATUS_ERR;
         break;
      }

      // Get Flash Memory busy flags
      flag = __HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY);
      // Is busy flag set?
      if(flag != RESET)
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

error_t stm32l4xxFlashDriverWrite(uint32_t address, uint8_t *data, size_t length)
{
   error_t error;
   uint32_t topAddress;
   const uint8_t *p;
   uint8_t word[STM32L4xx_FLASH_WRITE_SIZE];
   size_t n;

   // Precompute the top address
   topAddress = STM32L4xx_FLASH_ADDR + STM32L4xx_FLASH_SIZE;

   // Check address validity
   if((address < STM32L4xx_FLASH_ADDR || address >= topAddress) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(data == NULL ||
      !((address >= STM32L4xx_FLASH_BANK1_ADDR &&
      address + length <= STM32L4xx_FLASH_BANK2_ADDR) ||
      (address >= STM32L4xx_FLASH_BANK2_ADDR && address + length <= topAddress)))
      return ERROR_INVALID_PARAMETER;
#endif

   // Debug message
   TRACE_DEBUG("Writing data (%d bytes) at 0x%08X\r\n", length, address);
   TRACE_DEBUG_ARRAY("WRITE DATA: ", data, length);

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

      // Program 32-bit word in flash memory
      error = stm32l4xxFlashDriverWriteWord(address, (uint32_t)word);
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

error_t stm32l4xxFlashDriverRead(uint32_t address, uint8_t *data, size_t length)
{
   uint_t i;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = STM32L4xx_FLASH_ADDR + STM32L4xx_FLASH_SIZE;

   // Check address validity
   if(address < STM32L4xx_FLASH_ADDR || address >= topAddress)
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(data == NULL ||
      !((address >= STM32L4xx_FLASH_BANK1_ADDR &&
      address + length <= STM32L4xx_FLASH_BANK2_ADDR) ||
      (address >= STM32L4xx_FLASH_BANK2_ADDR && address + length <= topAddress)))
      return ERROR_INVALID_PARAMETER;
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
error_t stm32l4xxFlashDriverErase(uint32_t address, size_t length)
{
   error_t error;
   uint32_t topAddress;
   int_t firstSectorNumber;
   uint32_t lastSectorAddr;
   int_t lastSectorNumber;
   uint8_t bankId;

   error = NO_ERROR;

   // Precompute the top address
   topAddress = STM32L4xx_FLASH_ADDR + STM32L4xx_FLASH_SIZE;

   // Check address validity
   if((address < STM32L4xx_FLASH_ADDR || address >= topAddress) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if((length == 0) || (address + length > topAddress))
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is address in flash bank 1 or 2 only)
   if((length == 0) ||
      !((address >= STM32L4xx_FLASH_BANK1_ADDR &&
      address + length <= STM32L4xx_FLASH_BANK2_ADDR) ||
      (address >= STM32L4xx_FLASH_BANK2_ADDR && address + length <= topAddress)))
      return ERROR_INVALID_PARAMETER;
#endif

#if !defined(FLASH_DB_MODE)

   // Get the number of the first sector to erase
   firstSectorNumber = stm32l4xxFlashGetSector(address);

   if(address + length == topAddress)
   {
      // Set last sector number as the flash sector total number
      lastSectorNumber = STM32L4xx_FLASH_SECTOR_NUMBER;
   }
   else
   {
      // Get the address of the boundary sector (not to be erased)
      error = stm32l4xxFlashDriverGetNextSector(address + length, &lastSectorAddr);
      // Is any error?
      if(error)
         return error;

      // The last sector to erase is the last flash bank 1 sector
      lastSectorNumber = stm32l4xxFlashGetSector(lastSectorAddr);
   }

   // Erase sectors of the flash bank 1 (the only one flash bank)
   error = stm32l4xxFlashDriverEraseSector(STM32L4xx_FLASH_BANK1_ID, firstSectorNumber,
      lastSectorNumber - firstSectorNumber);
   // Is any error?
   if(error)
      return error;

#else

   // Get current flash bank ID
   error = flashGetCurrentBank(&bankId);
   // Is any error?
   if(error)
      return error;

   // Is sector to erase in flash bank 1?
   if(address < STM32L4xx_FLASH_BANK2_ADDR)
   {
      // Get the number of the first sector to erase
      firstSectorNumber = stm32l4xxFlashGetSector(address);

      // Is there data to erase in second flash bank?
      if(address + length > STM32L4xx_FLASH_BANK2_ADDR)
      {
         // Set last sector number as the first flash bank 2 sector number
         lastSectorNumber = STM32L4xx_FLASH_SECTOR_NUMBER / 2;

         // No more to erase
         length = 0;
         address = STM32L4xx_FLASH_BANK2_ADDR;
      }
      else
      {
         // Get the address of the boundary sector (not to be erased)
         error = stm32l4xxFlashDriverGetNextSector(address + length, &lastSectorAddr);
         // Is any error?
         if(error)
            return error;

         // The last sector to erase is the last flash bank 1 sector
         lastSectorNumber = stm32l4xxFlashGetSector(lastSectorAddr);

         // No more to erase
         length = 0;
         address = lastSectorAddr;
      }

      // Erase sectors of the flash bank 1
      error = stm32l4xxFlashDriverEraseSector(bankId, firstSectorNumber,
         lastSectorNumber - firstSectorNumber);
      // Is any error?
      if(error)
         return error;
   }

   // Is sector to erase in flash bank 2?
   if(length && address >= STM32L4xx_FLASH_BANK2_ADDR)
   {
      // Get the number of the first sector to erase
      firstSectorNumber = stm32l4xxFlashGetSector(address) % (STM32L4xx_FLASH_SECTOR_NUMBER / 2);

      // Is final erase address match end of flash?
      if(address + length >= topAddress)
      {
         // Set last sector number as the flash sector total number
         lastSectorNumber = STM32L4xx_FLASH_SECTOR_NUMBER / 2;
      }
      else
      {
         // Get the address of the boundary sector (not to be erased)
         error = stm32l4xxFlashDriverGetNextSector(address + length, &lastSectorAddr);
         // Is any error?
         if(error)
            return error;

         // The last sector to erase is the last flash bank 1 sector
         lastSectorNumber =
            stm32l4xxFlashGetSector(lastSectorAddr) % (STM32L4xx_FLASH_SECTOR_NUMBER / 2);
      }

      // Select the right flash bank for erase
      if(bankId == STM32L4xx_FLASH_BANK2_ID)
      {
         bankId = STM32L4xx_FLASH_BANK1_ID;
      }
      else
      {
         bankId = STM32L4xx_FLASH_BANK2_ID;
      }

      // Erase sectors of the flash bank 2
      error = stm32l4xxFlashDriverEraseSector(bankId, firstSectorNumber,
         lastSectorNumber - firstSectorNumber);
      // Is any error?
      if(error)
         return error;
   }
#endif

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

error_t stm32l4xxFlashDriverSwapBanks(void)
{
#if defined(FLASH_DB_MODE)
   error_t error;
   FLASH_OBProgramInitTypeDef OBInit;
   HAL_StatusTypeDef status;
   uint8_t fCurrentBankID;

   // Debug message
   TRACE_INFO("Swaping device flash bank...\r\n");

   // Get current flash bank ID
   error = flashGetCurrentBank(&fCurrentBankID);
   // Is any error?
   if(error)
      return error;

   // Start of exception handling block
   do
   {
      // Allow access to Flash control registers and user False
      status = HAL_FLASH_Unlock();
      // Is any error?
      if(status != HAL_OK)
      {
         // Debug message
         TRACE_ERROR("Flash Control Register unlock failed!\r\n");
         break;
      }

      // Start of exception handling block
      do
      {
         // Allow Access to option bytes sector
         status = HAL_FLASH_OB_Unlock();
         // Is any error?
         if(status != HAL_OK)
         {
            // Debug message
            TRACE_ERROR("Flash Option Control Registers unlock failed!\r\n");
            break;
         }

         // Get the Dual boot configuration status
         HAL_FLASHEx_OBGetConfig(&OBInit);

         // Swap in flash bank 2
         if(fCurrentBankID == STM32L4xx_FLASH_BANK1_ID)
         {
            // Debug message
            TRACE_DEBUG("Swaping from flask bank 1 to flash bank 2...\r\n");

            // Configure option bytes to swap on flash bank 2
            OBInit.OptionType = OPTIONBYTE_USER;
            OBInit.USERType = OB_USER_BFB2;
            OBInit.USERConfig = OB_BFB2_ENABLE;
         }
         // Swap in flash bank 1
         else
         {
            // Debug message
            TRACE_DEBUG("Swaping from flask bank 2 to flash bank 1...\r\n");

            // Configure option bytes to swap on flash bank 1
            OBInit.OptionType = OPTIONBYTE_USER;
            OBInit.USERType = OB_USER_BFB2;
            OBInit.USERConfig = OB_BFB2_DISABLE;
         }

         // Start of exception handling block
         do
         {
            // Start the Option Bytes programming process
            status = HAL_FLASHEx_OBProgram(&OBInit);
            // Is any error?
            if(status != HAL_OK)
            {
               // Debug message
               TRACE_ERROR("Option Bytes programming process failed!\r\n");
               break;
            }

            // Launch the option byte loading
            status = HAL_FLASH_OB_Launch();
            // Is any error?
            if(status != HAL_OK)
            {
               // Debug message
               TRACE_ERROR("Option byte loading failed!\r\n");
            }
         } while(0);

         // Prevent Access to option bytes sector
         if(HAL_FLASH_OB_Lock() != HAL_OK)
         {
            // Debug message
            TRACE_ERROR("Flash Option Control Register lock failed!\r\n");
         }
      } while(0);

      // Disable the Flash option control register access (recommended to protect
      // the option Bytes against possible unwanted operations)
      if(HAL_FLASH_Lock() != HAL_OK)
      {
         // Debug message
         TRACE_ERROR("Flash Control Register lock failed!\r\n");
      }
   } while(0);

   // Return status code
   return (status == HAL_OK) ? NO_ERROR : ERROR_FAILURE;
#else
   return NO_ERROR;
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

   // Start handling exception
   do
   {
      // Unlock Flash
      if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0U)
      {
         /* Authorize the FLASH Registers access */
         WRITE_REG(FLASH->KEYR, FLASH_KEY1);
         WRITE_REG(FLASH->KEYR, FLASH_KEY2);

         /* Verify Flash is unlocked */
         if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0U)
         {
            error = ERROR_ABORTED;
            break;
         }
      }

      // Start handling exception
      do
      {
         // Unlock Option Bytes
         if(READ_BIT(FLASH->CR, FLASH_CR_OPTLOCK) != 0U)
         {
            /* Authorizes the Option Byte register programming */
            WRITE_REG(FLASH->OPTKEYR, FLASH_OPTKEY1);
            WRITE_REG(FLASH->OPTKEYR, FLASH_OPTKEY2);

            /* Verify that the Option Bytes are unlocked */
            if(READ_BIT(FLASH->CR, FLASH_CR_OPTLOCK) != 0U)
            {
               error = ERROR_ABORTED;
               break;
            }
         }

         // Program option bytes
         if(READ_BIT(FLASH->OPTR, FLASH_OPTR_BFB2))
         {
            MODIFY_REG(FLASH->OPTR, FLASH_OPTR_BFB2, FLASH_OPTR_BFB2 & OB_BFB2_DISABLE);
         }
         else
         {
            MODIFY_REG(FLASH->OPTR, FLASH_OPTR_BFB2, FLASH_OPTR_BFB2 & OB_BFB2_ENABLE);
         }

         // Launch option byte programming
         SET_BIT(FLASH->CR, FLASH_CR_OPTSTRT);

         // Launch option byte loading
         SET_BIT(FLASH->CR, FLASH_CR_OBL_LAUNCH);

         // Lock Option Bytes
         SET_BIT(FLASH->CR, FLASH_CR_OPTLOCK);
      } while(0);

      // Lock flash bank
      SET_BIT(FLASH->CR, FLASH_CR_LOCK);

   } while(0);

   // Return status code
   return error;
}

/**
 * @brief Get address of the neighbouring sector
 * @return Error code
 **/

error_t stm32l4xxFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr)
{
   uint_t i;
   uint_t j;
   SectorsGroup *sg;
   uint32_t sAddr = 0xFFFFFFFF;
   uint32_t lastSectorAddr;

   lastSectorAddr =
      sectorsList[SECTORS_LIST_LEN - 1].addr +
      (sectorsList[SECTORS_LIST_LEN - 1].size * (sectorsList[SECTORS_LIST_LEN - 1].nb - 1));

   // Check parameters validity
   if(address < STM32L4xx_FLASH_ADDR || address > lastSectorAddr || sectorAddr == NULL)
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

bool_t stm32l4xxFlashDriverIsSectorAddr(uint32_t address)
{
   int_t sector;

   // Get Flash memory sector number
   sector = stm32l4xxFlashGetSector(address);

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
 * @brief Erase given number of sector from Flash memory starting from the given
 * index sector.
 * @param[in] firstSector Index of the first Flash memory sector to be erased
 * @param[in] nbSectors Number of Flash memory sector to be erased
 * @return Error code
 **/

error_t stm32l4xxFlashDriverEraseSector(uint32_t bankID, uint32_t firstSector, size_t nbSectors)
{
   HAL_StatusTypeDef status;
   FLASH_EraseInitTypeDef EraseInitStruct;
   uint32_t sectorError;

   // Check parameter validity
   if((firstSector >= STM32L4xx_FLASH_SECTOR_NUMBER) || (nbSectors == 0) ||
      ((firstSector + nbSectors - 1) >= STM32L4xx_FLASH_SECTOR_NUMBER) ||
#if !defined(FLASH_DB_MODE)
      (bankID != STM32L4xx_FLASH_BANK1_ID))
#else
      !((bankID == STM32L4xx_FLASH_BANK1_ID) || (bankID == STM32L4xx_FLASH_BANK2_ID)))
#endif
      return ERROR_INVALID_PARAMETER;

   // Debug message
   TRACE_DEBUG("Erasing Flash sector(s) %" PRIu32 "through %" PRIu32 "...\r\n", firstSector,
      firstSector + nbSectors);

   // Start of exception handling block
   do
   {
      // Allow access to Flash control registers and user False
      status = HAL_FLASH_Unlock();
      // Is any error?
      if(status != HAL_OK)
      {
         // Debug message
         TRACE_ERROR("Flash Control Register unlock failed!\r\n");
         break;
      }

      __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

      // Set flash erase settings
      EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
      EraseInitStruct.Banks = bankID;
      EraseInitStruct.Page = firstSector;
      EraseInitStruct.NbPages = nbSectors;

      // Wait for the last flash operation
      FLASH_WaitForLastOperation((uint32_t)5000U);

      // Erase the specified Flash sector(s)
      status = HAL_FLASHEx_Erase(&EraseInitStruct, &sectorError);

      // Is any error?
      if(status != HAL_OK)
      {
         // Debug message
         TRACE_ERROR("Failed to erase flash sector(s) %ld, error = 0x%08lX!\r\n", firstSector,
            sectorError);
      }

      // Disable the Flash option control register access (recommended to protect
      // the option Bytes against possible unwanted operations)
      if(HAL_FLASH_Lock() != HAL_OK)
      {
         // Debug message
         TRACE_ERROR("Flash Control Register lock failed!\r\n");
         break;
      }
   } while(0);

   // Return status code
   return (status == HAL_OK) ? NO_ERROR : ERROR_WRITE_FAILED;
}

/**
 * @brief Write 32-bits word in Flash Memory at the given address.
 * @param[in] address Address in Flash Memory to write to
 * @param[in] word 32-bit word to write in Flash memory
 * @return Error code
 **/

error_t stm32l4xxFlashDriverWriteWord(uint32_t address, uint32_t word)
{
   error_t error;
   uint8_t bankId;
   int_t flashSector;
   uint32_t sectorError;
   uint32_t topAddress;
   HAL_StatusTypeDef status;
   FLASH_EraseInitTypeDef EraseInitStruct;

   // Precompute the top address
   topAddress = STM32L4xx_FLASH_ADDR + STM32L4xx_FLASH_SIZE;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if((address + sizeof(uint32_t) >= topAddress) || (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(!((address >= STM32L4xx_FLASH_BANK1_ADDR &&
      address + sizeof(uint32_t) <= STM32L4xx_FLASH_BANK2_ADDR) ||
      (address >= STM32L4xx_FLASH_BANK2_ADDR && address + sizeof(uint32_t) <= topAddress)) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#endif

   // Unlock FLASH
   HAL_FLASH_Unlock();

   do
   {
      // Get flash sector number according to the given wirte address
      //  (-1 if it doesn't match a flash sector start address)
      flashSector = stm32l4xxFlashGetSector(address);

      // Is write address match a flash sector start address?
      if(flashSector >= 0)
      {
         // Sector number MUST be within flash bank sector boundaries (each bank
         // has half of the total number of sectors)
         flashSector = flashSector % (STM32L4xx_FLASH_SECTOR_NUMBER / 2);

         // Get current flash bank
         error = flashGetCurrentBank(&bankId);
         if(error)
            break;

         bankId = GetBank(address);
#if 0
         // Is bank2 the current flash bank?
         if(bankId == STM32L4xx_FLASH_BANK2_ID)
         {
            // Select right bank for erase operation in regards of the address
            bankId = (address >= STM32L4xx_FLASH_BANK2_ADDR) ? FLASH_BANK_1 : FLASH_BANK_2;
         }
         else
         {
            // Select right bank for erase operation in regards of the address
            bankId = (address >= STM32L4xx_FLASH_BANK2_ADDR) ? FLASH_BANK_2 : FLASH_BANK_1;
         }
#endif
         __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

         // Set flash erase settings
         EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
         EraseInitStruct.Banks = bankId;
         EraseInitStruct.Page = flashSector;
         EraseInitStruct.NbPages = 1;

         // Wait for the last flash operation
         FLASH_WaitForLastOperation((uint32_t)5000U);

         // Erase the sector
         status = HAL_FLASHEx_Erase(&EraseInitStruct, &sectorError);
         if(status != HAL_OK)
         {
            // Debug message
            TRACE_ERROR("Flash erase failed!\r\n");
            break;
         }
      }

      TRACE_DEBUG("Write data in flash at 0x%08X:\r\n", address);
      TRACE_DEBUG_ARRAY("data: ", (uint8_t *)word, 8);
      TRACE_DEBUG("\r\n");

      // Write word (64bits) into flash
      //  Becareful !!!
      //  - If FLASH_TYPEPROGRAM_DOUBLEWORD type program -> Data parameter MUST be
      //  the 64bits data value
      //  - If FLASH_TYPEPROGRAM_FAST type program -> Data parameter MUST be the
      //  the address of data
      status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, *((uint64_t *)word));
      if(status != HAL_OK)
      {
         // Debug message
         TRACE_ERROR("Flash program failed!\r\n");
         break;
      }
   } while(0);

   // Lock FLASH
   HAL_FLASH_Lock();

   // Return error code
   return (status != HAL_OK) ? ERROR_FAILURE : NO_ERROR;
}

/**
 * @brief Get the Memory Flash sector number according to the given address.
 * @param[in] address Given Flash Memory address
 * @return Sector number or -1
 **/

int_t stm32l4xxFlashGetSector(uint32_t address)
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
 * @brief Get current used flash bank (STM32L4xx_FLASH_BANK1_ID or
 * STM32L4xx_BANK_2_ID).
 * @param[out] fBankID Pointer to the flash bank ID to be retrieved
 * @return Status code
 **/

error_t flashGetCurrentBank(uint8_t *fBankID)
{
   if(READ_BIT(FLASH->OPTR, FLASH_OPTR_BFB2))
   {
      *fBankID = STM32L4xx_FLASH_BANK2_ID;
   }
   else
   {
      *fBankID = STM32L4xx_FLASH_BANK1_ID;
   }

   // Successful process
   return NO_ERROR;
}

/**
 * @brief  Gets the bank of a given address
 * @param  Addr: Address of the FLASH Memory
 * @retval The bank of a given address
 */
static uint32_t GetBank(uint32_t Addr)
{
   uint32_t bank = 0;

   if(READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE) == 0)
   {
      /* No Bank swap */
      if(Addr < (FLASH_BASE + FLASH_BANK_SIZE))
      {
         bank = FLASH_BANK_1;
      }
      else
      {
         bank = FLASH_BANK_2;
      }
   }
   else
   {
      /* Bank swap */
      if(Addr < (FLASH_BASE + FLASH_BANK_SIZE))
      {
         bank = FLASH_BANK_2;
      }
      else
      {
         bank = FLASH_BANK_1;
      }
   }

   return bank;
}
