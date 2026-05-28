/**
 * @file stm32u5xx_flash_driver.c
 * @brief CycloneBOOT STM32U5xx Flash Driver
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
#include "stm32u5xx_flash_driver.h"

#include "core/flash.h"
#include "debug.h"
#include "stm32u5xx.h"
#include "stm32u5xx_hal.h"

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
#ifndef FLASH_DB_MODE
   {STM32U5xx_FLASH_ADDR, STM32U5xx_FLASH_SECTOR_SIZE, STM32U5xx_FLASH_SECTOR_NUMBER}
#else
   {STM32U5xx_FLASH_ADDR, STM32U5xx_FLASH_SECTOR_SIZE, STM32U5xx_FLASH_SECTOR_NUMBER}
#endif
};

// Memory driver private related functions
error_t stm32u5xx_flash_driver_init(void);
error_t stm32u5xx_flash_driver_deinit(void);
error_t stm32u5xx_flash_driver_get_info(const FlashInfo **info);
error_t stm32u5xx_flash_driver_get_status(FlashStatus *status);
error_t stm32u5xx_flash_driver_write(uint32_t address, uint8_t *data, size_t length);
error_t stm32u5xx_flash_driver_read(uint32_t address, uint8_t *data, size_t length);
error_t stm32u5xx_flash_driver_erase(uint32_t address, size_t length);
error_t stm32u5xx_flash_driver_swap_banks(void);
error_t stm32u5xx_flash_driver_get_next_sector_address(uint32_t address, uint32_t *sectorAddr);
bool_t stm32u5xx_flash_driver_is_sector_address(uint32_t address);

error_t stm32u5xxFlashDriverWriteQuadWord(uint32_t address, uint32_t quadWord);
int_t stm32u5xxFlashGetSector(uint32_t address);
error_t stm32u5xxFlashDriverEraseSector(uint32_t bankID, uint32_t firstSector, size_t nbSectors);
error_t flashGetCurrentBank(uint8_t *fBankID);
error_t stm32u5xxFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Memory Information
 **/

const FlashInfo stm32u5xx_flash_driver_info = {.version = FLASH_DRIVER_VERSION,
                                               .flashName = STM32U5xx_FLASH_NAME,
                                               .flashType = FLASH_TYPE_INTERNAL,
                                               .flashAddr = STM32U5xx_FLASH_ADDR,
                                               .flashSize = STM32U5xx_FLASH_SIZE,
                                               .writeSize = STM32U5xx_FLASH_WRITE_SIZE,
                                               .readSize = STM32U5xx_FLASH_READ_SIZE,
#ifndef FLASH_DB_MODE
                                               .dualBank = 0,
                                               .bankSize = 0,
                                               .bank1Addr = 0,
                                               .bank2Addr = 0,
                                               .flags = 0
#else
                                               .dualBank = 1,
                                               .bankSize = STM32U5xx_FLASH_BANK_SIZE,
                                               .bank1Addr = STM32U5xx_FLASH_ADDR,
                                               .bank2Addr =
                                                  STM32U5xx_FLASH_ADDR + STM32U5xx_FLASH_BANK_SIZE,
                                               .flags = 0
#endif
};

/**
 * @brief stm32u5xx Flash driver
 **/

const FlashDriver stm32u5xxFlashDriver = {
   .init = stm32u5xx_flash_driver_init,
   .deInit = stm32u5xx_flash_driver_deinit,
   .getInfo = stm32u5xx_flash_driver_get_info,
   .getStatus = stm32u5xx_flash_driver_get_status,
   .write = stm32u5xx_flash_driver_write,
   .read = stm32u5xx_flash_driver_read,
   .erase = stm32u5xx_flash_driver_erase,
#ifdef FLASH_DB_MODE
   .swapBanks = stm32u5xx_flash_driver_swap_banks,
#endif
   .getNextSectorAddr = stm32u5xx_flash_driver_get_next_sector_address,
   .isSectorAddr = stm32u5xx_flash_driver_is_sector_address,
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Initialize Flash Memory.
 * @return Error code
 **/

error_t stm32u5xx_flash_driver_init(void)
{
   // Debug message
   TRACE_INFO("Initializing %s memory...\r\n", STM32U5xx_FLASH_NAME);

   // Wait for last flash operation on flash bank 1
   FLASH_WaitForLastOperation(50);
   // Clear all flash bank 1 flags
   __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

#if defined(FLASH_DB_MODE)
   // Wait for last flash operation on flash bank 2
   FLASH_WaitForLastOperation(50);
   // Clear all flash bank 2 flags
   __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
#endif

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief De-Initialize Flash Memory.
 * @return Error code
 **/

error_t stm32u5xx_flash_driver_deinit(void)
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

error_t stm32u5xx_flash_driver_get_info(const FlashInfo **info)
{
   // Set Memory information pointer
   *info = &stm32u5xx_flash_driver_info;

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Get Flash Memory status.
 * @param[in,out] status Pointeur to the Memory status to be returned
 * @return Error code
 **/

error_t stm32u5xx_flash_driver_get_status(FlashStatus *status)
{
   uint32_t flag;

   // Check parameter validity
   if(status == NULL)
      return ERROR_INVALID_PARAMETER;

   do
   {
      // Get Flash Memory error flags status
#if defined(FLASH_DB_MODE)
      flag = __HAL_FLASH_GET_FLAG(FLASH_FLAG_ALL_ERRORS);
#else
      flag = __HAL_FLASH_GET_FLAG(FLASH_FLAG_ALL_ERRORS);
#endif
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

   // Successful process
   return NO_ERROR;
}

/**
 * @brief Write data in Flash Memory at the given address.
 * @param[in] address Address in Flash Memory to write to
 * @param[in] data Pointeur to the data to write
 * @param[in] length Number of data bytes to write in
 * @return Error code
 **/

error_t stm32u5xx_flash_driver_write(uint32_t address, uint8_t *data, size_t length)
{
   error_t error;
   uint32_t topAddress;
   const uint8_t *p;
   uint8_t quadWord[STM32U5xx_FLASH_WRITE_SIZE];
   size_t n;

   // Pre-compute the top address
   topAddress = STM32U5xx_FLASH_ADDR + STM32U5xx_FLASH_SIZE;

   // Check address validity
   if((address < STM32U5xx_FLASH_ADDR || address >= topAddress) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(data == NULL ||
      !((address >= STM32U5xx_FLASH_BANK1_ADDR &&
      address + length <= STM32U5xx_FLASH_BANK2_ADDR) ||
      (address >= STM32U5xx_FLASH_BANK2_ADDR && address + length <= topAddress)))
      return ERROR_INVALID_PARAMETER;
#endif

   // Debug message
   TRACE_DEBUG("Writing data (%d bytes) at 0x%08lx\r\n", length, address);
   TRACE_DEBUG_ARRAY("WRITE DATA: ", data, length);

   // Cast data pointer
   p = (const uint8_t *)data;

   // Perform write operation
   while(length > 0)
   {
      // Prevent to write more than allowed flash write bytes at a time
      n = MIN(sizeof(quadWord), length);

      // Check if remaining bytes is less than required flash write size
      if(n < sizeof(quadWord))
         memset(quadWord, 0, sizeof(quadWord));

      // Copy n bytes
      memcpy(quadWord, p, n);

      // Program 32-bit word x 4 in flash memory
      error = stm32u5xxFlashDriverWriteQuadWord(address, (uint32_t)quadWord);
      if(error)
      {
         // Debug message
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

error_t stm32u5xx_flash_driver_read(uint32_t address, uint8_t *data, size_t length)
{
   uint_t i;
   uint32_t topAddress;

   // Pre-compute the top address
   topAddress = STM32U5xx_FLASH_ADDR + STM32U5xx_FLASH_SIZE;

   // Check address validity
   if(address < STM32U5xx_FLASH_ADDR || address >= topAddress)
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(data == NULL ||
      !((address >= STM32U5xx_FLASH_BANK1_ADDR &&
      address + length <= STM32U5xx_FLASH_BANK2_ADDR) ||
      (address >= STM32U5xx_FLASH_BANK2_ADDR && address + length <= topAddress)))
      return ERROR_INVALID_PARAMETER;
#endif

   // Perform read operation
   for(i = 0; i < length; i++)
   {
      *((uint8_t *)data + i) = *(uint8_t *)address;
      address++;
   }

   // Successful process
   return NO_ERROR;
}

/**
 * @brief Erase data from Memory at the given address.
 * @param[in] address Address in Memory to start erasing from
 * @param[in] length Number of data bytes to be erased
 * @return Error code
 **/

error_t stm32u5xx_flash_driver_erase(uint32_t address, size_t length)
{
   error_t error;
   uint32_t topAddress;
   int_t firstSectorNumber;
   uint32_t lastSectorAddr;
   int_t lastSectorNumber;

   // Initialize error code
   error = NO_ERROR;

   // Pre-compute the top address
   topAddress = STM32U5xx_FLASH_ADDR + STM32U5xx_FLASH_SIZE;

   // Check address validity
   if((address < STM32U5xx_FLASH_ADDR || address >= topAddress) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if((length == 0) || (address + length > topAddress))
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is address in flash bank 1 or 2 only)
   if((length == 0) ||
      !((address >= STM32U5xx_FLASH_BANK1_ADDR &&
      address + length <= STM32U5xx_FLASH_BANK2_ADDR) ||
      (address >= STM32U5xx_FLASH_BANK2_ADDR && address + length <= topAddress)))
      return ERROR_INVALID_PARAMETER;
#endif

#if !defined(FLASH_DB_MODE)

   // Get the number of the first sector to erase
   firstSectorNumber = stm32u5xxFlashGetSector(address);

   if(address + length == topAddress)
   {
      // Set last sector number as the flash sector total number
      lastSectorNumber = STM32U5xx_FLASH_SECTOR_NUMBER;
   }
   else
   {
      // Get the address of the boundary sector (not to be erased)
      error = stm32u5xx_flash_driver_get_next_sector_address(address + length, &lastSectorAddr);
      // Is any error?
      if(error)
         return error;

      // The last sector to erase is the last flash bank 1 sector
      lastSectorNumber = stm32u5xxFlashGetSector(lastSectorAddr);
   }

   // Erase sectors of the flash bank 1 (the only one flash bank)
   error = stm32u5xxFlashDriverEraseSector(STM32U5xx_FLASH_BANK1_ID, firstSectorNumber,
      lastSectorNumber - firstSectorNumber);
   // Is any error?
   if(error)
      return error;

#else

   // Is sector to erase in flash bank 1?
   if(address < STM32U5xx_FLASH_BANK2_ADDR)
   {
      // Get the number of the first sector to erase
      firstSectorNumber = stm32u5xxFlashGetSector(address);

      // Is there data to erase in second flash bank?
      if(address + length > STM32U5xx_FLASH_BANK2_ADDR)
      {
         // Set last sector number as the first flash bank 2 sector number
         lastSectorNumber = STM32U5xx_FLASH_SECTOR_NUMBER / 2;

         // No more to erase
         length = 0;
         address = lastSectorAddr;
      }
      else
      {
         // Get the address of the boundary sector (not to be erased)
         error =
            stm32u5xx_flash_driver_get_next_sector_address(address + length, &lastSectorAddr);
         // Is any error?
         if(error)
            return error;

         // The last sector to erase is the last flash bank 1 sector
         lastSectorNumber = stm32u5xxFlashGetSector(lastSectorAddr);

         // No more to erase
         length = 0;
         address = lastSectorAddr;
      }

      // Erase sectors of the flash bank 1
      error = stm32u5xxFlashDriverEraseSector(STM32U5xx_FLASH_BANK1_ID, firstSectorNumber,
         lastSectorNumber - firstSectorNumber);
      // Is any error?
      if(error)
         return error;
   }

   // Is sector to erase in flash bank 2?
   if(length && address >= STM32U5xx_FLASH_BANK2_ADDR)
   {
      // Get the number of the first sector to erase
      firstSectorNumber = stm32u5xxFlashGetSector(address);

      // Is final erase address match end of flash?
      if(address + length == topAddress)
      {
         // Set last sector number as the flash sector total number
         lastSectorNumber = STM32U5xx_FLASH_SECTOR_NUMBER;
      }
      else
      {
         // Get the address of the boundary sector (not to be erased)
         error = stm32u5xxFlashDriverGetNextSector(address + length, &lastSectorAddr);
         // Is any error?
         if(error)
            return error;

         // The last sector to erase is the last flash bank 1 sector
         lastSectorNumber = stm32u5xxFlashGetSector(lastSectorAddr);
      }

      // Erase sectors of the flash bank 2
      error = stm32u5xxFlashDriverEraseSector(STM32U5xx_FLASH_BANK2_ID, firstSectorNumber,
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

error_t stm32u5xx_flash_driver_swap_banks(void)
{
#if defined(FLASH_DB_MODE)
   error_t error;
   FLASH_OBProgramInitTypeDef OBInit;
   HAL_StatusTypeDef status;
   uint8_t fCurrentBankID;

   // Debug message
   TRACE_INFO("Swapping device flash bank...\r\n");

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
         if(fCurrentBankID == STM32U5xx_FLASH_BANK1_ID)
         {
            // Debug message
            TRACE_DEBUG("Swapping from flask bank 1 to flash bank 2...\r\n");

            // Configure option bytes to swap on flash bank 2
            OBInit.OptionType = OPTIONBYTE_USER;
            OBInit.USERType = OB_USER_SWAP_BANK;
            OBInit.USERConfig = OB_SWAP_BANK_ENABLE;
         }
         // Swap in flash bank 1
         else
         {
            // Debug message
            TRACE_DEBUG("Swapping from flask bank 2 to flash bank 1...\r\n");

            // Configure option bytes to swap on flash bank 1
            OBInit.OptionType = OPTIONBYTE_USER;
            OBInit.USERType = OB_USER_SWAP_BANK;
            OBInit.USERConfig = OB_SWAP_BANK_DISABLE;
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
   // Successfull process
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

   // Start handling exception
   do
   {
      // Unlock flash
      if(READ_BIT(FLASH->NSCR, FLASH_NSCR_LOCK) != 0U)
      {
         /* Authorize the FLASH Registers access */
         WRITE_REG(FLASH->NSKEYR, FLASH_KEY1);
         WRITE_REG(FLASH->NSKEYR, FLASH_KEY2);

         /* verify Flash is unlocked */
         if(READ_BIT(FLASH->NSCR, FLASH_NSCR_LOCK) != 0U)
         {
            error = ERROR_ABORTED;
            break;
         }
      }

      // Start handling exception
      do
      {
         // Unlock Option Bytes
         if(READ_BIT(FLASH->NSCR, FLASH_NSCR_OPTLOCK) != 0U)
         {
            /* Authorizes the Option Byte register programming */
            WRITE_REG(FLASH->OPTKEYR, FLASH_OPTKEY1);
            WRITE_REG(FLASH->OPTKEYR, FLASH_OPTKEY2);

            /* Verify that the Option Bytes are unlocked */
            if(READ_BIT(FLASH->NSCR, FLASH_NSCR_OPTLOCK) != 0U)
            {
               error = ERROR_ABORTED;
               break;
            }
         }

         // Program option bytes
         if(READ_BIT(FLASH->OPTR, FLASH_OPTR_SWAP_BANK))
         {
            MODIFY_REG(FLASH->OPTR, FLASH_OPTR_SWAP_BANK,
               FLASH_OPTR_SWAP_BANK & OB_SWAP_BANK_DISABLE);
         }
         else
         {
            MODIFY_REG(FLASH->OPTR, FLASH_OPTR_SWAP_BANK,
               FLASH_OPTR_SWAP_BANK & OB_SWAP_BANK_ENABLE);
         }

         /* Set OPTSTRT Bit */
         SET_BIT(FLASH->NSCR, FLASH_NSCR_OPTSTRT);

         // Launch option byte programming
         SET_BIT(FLASH->NSCR, FLASH_NSCR_OBL_LAUNCH);

         // Lock Option Bytes
         SET_BIT(FLASH->NSCR, FLASH_NSCR_OPTLOCK);

      } while(0);

      // Lock flash
      SET_BIT(FLASH->NSCR, FLASH_NSCR_LOCK);

   } while(0);

   // Return status code
   return error;
}

/**
 * @brief Get address of the neighbouring sector
 * @return Error code
 **/

error_t stm32u5xx_flash_driver_get_next_sector_address(uint32_t address, uint32_t *sectorAddr)
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
   if(address < STM32U5xx_FLASH_ADDR || address > lastSectorAddr || sectorAddr == NULL)
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

   // Save next sector address
   *sectorAddr = sAddr;

   // Successful process
   return NO_ERROR;
}

/**
 * @brief Determine if a given address is contained within a sector
 * @return boolean
 **/

bool_t stm32u5xx_flash_driver_is_sector_address(uint32_t address)
{
   int_t sector;

   // Get Flash memory sector number
   sector = stm32u5xxFlashGetSector(address);

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

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Write 64-bits word in Flash Memory at the given address.
 * @param[in] address Address in Flash Memory to write to
 * @param[in] quadWord 64-bit word to write in Flash memory
 * @return Error code
 **/

error_t stm32u5xxFlashDriverWriteQuadWord(uint32_t address, uint32_t quadWord)
{
   uint8_t bankId;
   int_t flashSector;
   uint32_t sectorError;
   uint32_t topAddress;
   HAL_StatusTypeDef status;
   FLASH_EraseInitTypeDef EraseInitStruct;

   // Pre-compute the top address
   topAddress = STM32U5xx_FLASH_ADDR + STM32U5xx_FLASH_SIZE;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if((address + sizeof(uint32_t) >= topAddress) || (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(!((address >= STM32U5xx_FLASH_BANK1_ADDR &&
      address + sizeof(uint32_t) <= STM32U5xx_FLASH_BANK2_ADDR) ||
      (address >= STM32U5xx_FLASH_BANK2_ADDR && address + sizeof(uint32_t) <= topAddress)) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#endif

   // Unlock FLASH
   HAL_FLASH_Unlock();

   do
   {
      // Get flash sector number according to the given wirte address
      //  (-1 if it doesn't match a flash sector start address)
      flashSector = stm32u5xxFlashGetSector(address);

      // Is write address match a flash sector start address?
      if(flashSector >= 0)
      {
#if defined(FLASH_DB_MODE)
         // Get current bank ID
         flashGetCurrentBank(&bankId);

         // Sector number MUST be within flash bank sector boundaries (each bank
         // has half of the total number of sectors)
         flashSector = flashSector % (STM32U5xx_FLASH_SECTOR_NUMBER / 2);

         // Get flash bank ID according to the given write address
         // bankId = (address >= STM32U5xx_FLASH_BANK2_ADDR)? FLASH_BANK_2 :
         // FLASH_BANK_1;
         if(bankId == 1)
         {
            bankId = 2;
         }
         else
         {
            bankId = 1;
         }
#else
         bankId = FLASH_BANK_1;
#endif

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

      // Write word (256bits) into flash
      status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, address, (uint32_t)quadWord);
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

int_t stm32u5xxFlashGetSector(uint32_t address)
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

   // Return sector number
   return sector;
}

/**
 * @brief Erase given number of sector from Flash memory starting from the given
 * index sector.
 * @param[in] firstSector Index of the first Flash memory sector to be erased
 * @param[in] nbSectors Number of Flash memory sector to be erased
 * @return Error code
 **/

error_t stm32u5xxFlashDriverEraseSector(uint32_t bankID, uint32_t firstSector, size_t nbSectors)
{
   HAL_StatusTypeDef status;
   FLASH_EraseInitTypeDef EraseInitStruct;
   uint32_t sectorError;

   // Check parameter validity
   if((firstSector >= STM32U5xx_FLASH_SECTOR_NUMBER) || (nbSectors == 0) ||
      ((firstSector + nbSectors - 1) >= STM32U5xx_FLASH_SECTOR_NUMBER) ||
#if !defined(FLASH_DB_MODE)
      (bankID != STM32U5xx_FLASH_BANK1_ID))
#else
      !((bankID == STM32U5xx_FLASH_BANK1_ID) || (bankID == STM32U5xx_FLASH_BANK2_ID)))
#endif
      return ERROR_INVALID_PARAMETER;

   // Debug message
   TRACE_DEBUG("Erasing Flash sector(s) %" PRIu32 "through %" PRIu32 "...\r\n", firstSectorNumber,
      lastSectorNumber);

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
         TRACE_ERROR("Failed to erase flash sector(s) %" PRIu32 ", error = 0x%08" PRIx32 "!\r\n",
            firstSector, sectorError);
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

#ifdef FLASH_DB_MODE

/**
 * @brief Get current used flash bank (STM32U5xx_FLASH_BANK1_ID or
 * STM32U5xx_BANK_2_ID).
 * @param[out] fBankID Pointer to the flash bank ID to be retrieved
 * @return Status code
 **/

error_t flashGetCurrentBank(uint8_t *fBankID)
{
   if(READ_BIT(FLASH->OPTR, FLASH_OPTR_SWAP_BANK))
   {
      *fBankID = STM32U5xx_FLASH_BANK2_ID;
   }
   else
   {
      *fBankID = STM32U5xx_FLASH_BANK1_ID;
   }

   // Successful process
   return NO_ERROR;
}

#endif

/**
 * @brief Get address of the neighbouring sector
 * @return Error code
 **/

error_t stm32u5xxFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr)
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
   if(address < STM32U5xx_FLASH_ADDR || address > lastSectorAddr || sectorAddr == NULL)
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
