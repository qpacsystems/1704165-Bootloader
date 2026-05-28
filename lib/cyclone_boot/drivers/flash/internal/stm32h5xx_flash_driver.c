/**
 * @file stm32h5xx_flash_driver.c
 * @brief CycloneBOOT STM32H5xx Flash Driver
 *
 * @section License
 *
 * Copyright (C) 2021-2025 Oryx Embedded SARL. All rights reserved.
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
 * @version 2.5.4-revb
 **/

// Switch to the appropriate trace level
#define TRACE_LEVEL CBOOT_DRIVER_TRACE_LEVEL

// Dependencies
#include "stm32h5xx_flash_driver.h"

#include "core/flash.h"
#include "debug.h"
#include "stm32h5xx.h"
#include "stm32h5xx_hal.h"

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
   {STM32H5xx_FLASH_ADDR, STM32H5xx_FLASH_SECTOR_SIZE, STM32H5xx_FLASH_SECTOR_NUMBER}
};

// private functions
error_t stm32h5xxFlashDriverInit(void);
error_t stm32h5xxFlashDriverDeInit(void);
error_t stm32h5xxFlashDriverGetInfo(const FlashInfo **info);
error_t stm32h5xxFlashDriverGetStatus(FlashStatus *status);
error_t stm32h5xxFlashDriverWrite(uint32_t address, uint8_t *data, size_t length);
error_t stm32h5xxFlashDriverRead(uint32_t address, uint8_t *data, size_t length);
error_t stm32h5xxFlashDriverErase(uint32_t address, size_t length);
error_t stm32h5xxFlashDriverSwapBanks(void);
error_t stm32h5xxFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr);
bool_t stm32h5xxFlashDriverIsSectorAddress(uint32_t address);

error_t stm32h5xxFlashDriverWriteQuadWord(uint32_t address, uint32_t quadWord);
int_t stm32h5xxFlashGetSector(uint32_t address);
error_t stm32h5xxFlashDriverEraseSector(uint32_t bankID, uint32_t firstSector, size_t nbSectors);
error_t flashGetCurrentBank(uint8_t *fBankID);

uint32_t GetBank(uint32_t Addr);
uint32_t GetSector(uint32_t Address);
uint32_t CountSectorsToDelete(uint32_t StartAddress, uint32_t EndAddress);
uint32_t GetSectorSingleBank(uint32_t Address);
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Memory Information
 **/

const FlashInfo stm32h5xxFlashDriverInfo = {.version = FLASH_DRIVER_VERSION,
                                            .flashName = STM32H5xx_FLASH_NAME,
                                            .flashType = FLASH_TYPE_INTERNAL,
                                            .flashAddr = STM32H5xx_FLASH_ADDR,
                                            .flashSize = STM32H5xx_FLASH_SIZE,
                                            .writeSize = STM32H5xx_FLASH_WRITE_SIZE,
                                            .readSize = STM32H5xx_FLASH_READ_SIZE,
#ifndef FLASH_DB_MODE
                                            .dualBank = 0,
                                            .bankSize = 0,
                                            .bank1Addr = 0,
                                            .bank2Addr = 0,
                                            .flags = 0
#else
                                            .dualBank = 1,
                                            .bankSize = STM32H5xx_FLASH_BANK_SIZE,
                                            .bank1Addr = STM32H5xx_FLASH_BANK1_ADDR,
                                            .bank2Addr = STM32H5xx_FLASH_BANK2_ADDR,
                                            .flags = FLASH_FLAGS_LATER_SWAP
#endif
};

/**
 * @brief STM32h7xx Flash driver
 **/

const FlashDriver stm32h5xxFlashDriver = {
   .init = stm32h5xxFlashDriverInit,
   .deInit = stm32h5xxFlashDriverDeInit,
   .getInfo = stm32h5xxFlashDriverGetInfo,
   .getStatus = stm32h5xxFlashDriverGetStatus,
   .write = stm32h5xxFlashDriverWrite,
   .read = stm32h5xxFlashDriverRead,
   .erase = stm32h5xxFlashDriverErase,
#ifndef FLASH_DB_MODE
   .swapBanks = NULL,
#else
   .swapBanks = stm32h5xxFlashDriverSwapBanks,
#endif
   .getNextSectorAddr = stm32h5xxFlashDriverGetNextSector,
   .isSectorAddr = stm32h5xxFlashDriverIsSectorAddress,
};

/**
 * @brief Initialize Flash Memory.
 * @return Error code
 **/

error_t stm32h5xxFlashDriverInit(void)
{

   TRACE_INFO("Initializing %s memory...\r\n", STM32H5xx_FLASH_NAME);

   // Wait for last flash operation on flash bank 1
   FLASH_WaitForLastOperation(50);
   // Clear all flash bank 1 flags
   __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief De-Initialize Flash Memory.
 * @return Error code
 **/

error_t stm32h5xxFlashDriverDeInit(void)
{
   return ERROR_NOT_IMPLEMENTED;
   ;
}

/**
 * @brief Get Flash Memory information.
 * @param[in,out] info Pointeur to the Memory information structure to be
 * returned
 * @return Error code
 **/

error_t stm32h5xxFlashDriverGetInfo(const FlashInfo **info)
{
   // Set Memory information pointer
   *info = (const FlashInfo *)&stm32h5xxFlashDriverInfo;

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Get Flash Memory status.
 * @param[in,out] status Pointeur to the Memory status to be returned
 * @return Error code
 **/

error_t stm32h5xxFlashDriverGetStatus(FlashStatus *status)
{
   uint32_t flag;

   // Check parameter validity
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

error_t stm32h5xxFlashDriverWrite(uint32_t address, uint8_t *data, size_t length)
{
   error_t error;
   uint32_t topAddress;
   const uint8_t *p;
   uint8_t quadWord[STM32H5xx_FLASH_WRITE_SIZE];
   size_t n;

   // Pre-compute the top address
   topAddress = STM32H5xx_FLASH_ADDR + STM32H5xx_FLASH_SIZE;

   // Check address validity
   if((address < STM32H5xx_FLASH_ADDR || address >= topAddress) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(data == NULL ||
      !((address >= STM32H5xx_FLASH_BANK1_ADDR &&
      address + length <= STM32H5xx_FLASH_BANK2_ADDR) ||
      (address >= STM32H5xx_FLASH_BANK2_ADDR && address + length <= topAddress)))
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

      // Program 32-bit word in flash memory
      error = stm32h5xxFlashDriverWriteQuadWord(address, (uint32_t)quadWord);
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

error_t stm32h5xxFlashDriverRead(uint32_t address, uint8_t *data, size_t length)
{
   uint_t i;
   uint32_t topAddress;

   // Pre-compute the top address
   topAddress = STM32H5xx_FLASH_ADDR + STM32H5xx_FLASH_SIZE;

   // Check address validity
   if(address < STM32H5xx_FLASH_ADDR || address >= topAddress)
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(data == NULL ||
      !((address >= STM32H5xx_FLASH_BANK1_ADDR &&
      address + length <= STM32H5xx_FLASH_BANK2_ADDR) ||
      (address >= STM32H5xx_FLASH_BANK2_ADDR && address + length <= topAddress)))
      return ERROR_INVALID_PARAMETER;
#endif

   // Perform read operation
   for(i = 0; i < length; i++)
   {
      *((uint8_t *)data + i) = *(uint8_t *)address;
      address++;
   }

   return NO_ERROR;
}

/**
 * @brief Erase data from Memory at the given address.
 * @param[in] address Address in Memory to start erasing from
 * @param[in] length Number of data bytes to be erased
 * @return Error code
 **/

error_t stm32h5xxFlashDriverErase(uint32_t address, size_t length)
{
   error_t error;
   uint32_t topAddress;
   int_t firstSectorNumber;
   uint8_t bankId;

   uint32_t sectorAddr;
   uint32_t sectorEndAddr;

   error = NO_ERROR;
   // Pre-compute the top address
   topAddress = STM32H5xx_FLASH_ADDR + STM32H5xx_FLASH_SIZE;

   // Check address validity
   if((address < STM32H5xx_FLASH_ADDR || address >= topAddress) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if((length == 0) || (address + length > topAddress))
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is address in flash bank 1 or 2 only)
   if((length == 0) ||
      !((address >= STM32H5xx_FLASH_BANK1_ADDR &&
      address + length <= STM32H5xx_FLASH_BANK2_ADDR) ||
      (address >= STM32H5xx_FLASH_BANK2_ADDR && address + length <= topAddress)))
      return ERROR_INVALID_PARAMETER;
#endif

#if !defined(FLASH_DB_MODE)
   sectorEndAddr = address + length;

   do
   {

      sectorAddr = address;
      firstSectorNumber = GetSector(sectorAddr);
      bankId = GetBank(sectorAddr);

      error = stm32h5xxFlashDriverEraseSector(bankId, firstSectorNumber, 1);
      // Is any error?
      if(error)
      {
         TRACE_ERROR("Failed to erase sector.\r\n");
         break;
      }

      address += FLASH_SECTOR_SIZE;

   } while(address < sectorEndAddr);
#else
   error = ERROR_NOT_IMPLEMENTED;
#endif

   // Successful process
   return error;
}

/**
 * @brief Performs a Memory bank swap according to the current bank ID.
 * If current Memory bank ID match the 1st bank then it will swap on the 2nd
 * Memory bank. If current Memory bank ID match the 2nd bank then it will swap on
 * the 1st Memory bank.
 * @return Error code
 **/

error_t stm32h5xxFlashDriverSwapBanks(void)
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
         if(fCurrentBankID == STM32H5xx_FLASH_BANK1_ID)
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
   // Return status code
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
#if 0
   error_t error;

   //Initialize status code
   error = NO_ERROR;

   //Start handling exception
   do
   {
      // Unlock FLASH
      if(READ_BIT(FLASH->NSCR, FLASH_CR_LOCK) != 0U)
      {
         /* Authorize the FLASH Registers access */
         WRITE_REG(FLASH->NSKEYR, FLASH_KEY1);
         WRITE_REG(FLASH->NSKEYR, FLASH_KEY2);

         /* verify Flash is unlocked */
         if(READ_BIT(FLASH->NSCR, FLASH_CR_LOCK) != 0U)
         {
            error = ERROR_FAILURE;
         }
      }
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
      if(error == NO_ERROR)
      {
         if(READ_BIT(FLASH->SECCR, FLASH_SECCR_LOCK) != 0U)
         {
            /* Authorize the FLASH Registers access */
            WRITE_REG(FLASH->SECKEYR, FLASH_KEY1);
            WRITE_REG(FLASH->SECKEYR, FLASH_KEY2);

            /* verify Flash is unlocked */
            if(READ_BIT(FLASH->SECCR, FLASH_SECCR_LOCK) != 0U)
            {
               error = ERROR_FAILURE;
            }
         }
      }
#endif /* __ARM_FEATURE_CMSE */

      // Unlock Option Bytes
      if(READ_BIT(FLASH->NSCR, FLASH_CR_OPTLOCK) != 0U)
      {
         /* Authorizes the Option Byte register programming */
         WRITE_REG(FLASH->OPTKEYR, FLASH_OPTKEY1);
         WRITE_REG(FLASH->OPTKEYR, FLASH_OPTKEY2);

         /* Verify that the Option Bytes are unlocked */
         if(READ_BIT(FLASH->NSCR, FLASH_NSCR_OPTLOCK) != 0U)
         {
            error = ERROR_FAILURE;
         }
      }

      // Swap Bank
      // TODO: IMPLEMENT ME!

      return error;

   } while(0);

   //Return status code
   return error;
#else
   return ERROR_NOT_IMPLEMENTED;
#endif
}

/**
 * @brief Get address of the neighbouring sector
 * @return Error code
 **/

error_t stm32h5xxFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr)
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
   if(address < STM32H5xx_FLASH_ADDR || address > lastSectorAddr || sectorAddr == NULL)
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

bool_t stm32h5xxFlashDriverIsSectorAddress(uint32_t address)
{

   int_t sector;

   // Get Flash memory sector number
   sector = stm32h5xxFlashGetSector(address);

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

error_t stm32h5xxFlashDriverWriteQuadWord(uint32_t address, uint32_t quadWord)
{
   uint8_t bankId;
   int_t flashSector;
   uint32_t sectorError;
   uint32_t topAddress;
   HAL_StatusTypeDef status;
   FLASH_EraseInitTypeDef EraseInitStruct;

   // Pre-compute the top address
   topAddress = STM32H5xx_FLASH_ADDR + STM32H5xx_FLASH_SIZE;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if((address + sizeof(uint32_t) >= topAddress) || (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(!((address >= STM32H5xx_FLASH_BANK1_ADDR &&
      address + sizeof(uint32_t) <= STM32H5xx_FLASH_BANK2_ADDR) ||
      (address >= STM32H5xx_FLASH_BANK2_ADDR && address + sizeof(uint32_t) <= topAddress)) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#endif

   // Unlock FLASH
   HAL_FLASH_Unlock();

   do
   {
      // Get flash sector number according to the given wirte address
      //  (-1 if it doesn't match a flash sector start address)
      flashSector = stm32h5xxFlashGetSector(address);

      // Is write address match a flash sector start address?
      if(flashSector >= 0)
      {
#if defined(FLASH_DB_MODE)
         // Sector number MUST be within flash bank sector boundaries (each bank
         // has half of the total number of sectors)
         flashSector = flashSector % (STM32H5xx_FLASH_SECTOR_NUMBER / 2);

         // Determine the nno-executing flash bank to be erased
         flashGetCurrentBank(&bankId);
         if(bankId == STM32H5xx_FLASH_BANK1_ID)
         {
            bankId = FLASH_BANK_2;
         }
         else
         {
            bankId = FLASH_BANK_1;
         }
#else
         // Sector number MUST be within flash bank sector boundaries (each bank
         // has half of the total number of sectors)
         flashSector = GetSector(address);
         bankId = GetBank(address);
#endif

         // Set flash erase settings
         EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
         EraseInitStruct.Banks = bankId;
         EraseInitStruct.Sector = flashSector;
         EraseInitStruct.NbSectors = 1;

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

int_t stm32h5xxFlashGetSector(uint32_t address)
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
 * @brief Erase given number of sector from Flash memory starting from the given
 * index sector.
 * @param[in] firstSector Index of the first Flash memory sector to be erased
 * @param[in] nbSectors Number of Flash memory sector to be erased
 * @return Error code
 **/

error_t stm32h5xxFlashDriverEraseSector(uint32_t bankID, uint32_t firstSector, size_t nbSectors)
{
   HAL_StatusTypeDef status;
   FLASH_EraseInitTypeDef EraseInitStruct;
   uint32_t sectorError;
   uint32_t flashError;

   // Check parameter validity
   if((firstSector >= STM32H5xx_FLASH_SECTOR_NUMBER) || (nbSectors == 0) ||
      ((firstSector + nbSectors - 1) >= STM32H5xx_FLASH_SECTOR_NUMBER) ||
#if defined(FLASH_SINGLE_BANK)
      (bankID != STM32H5xx_FLASH_BANK1_ID))
#else
      !((bankID == STM32H5xx_FLASH_BANK1_ID) || (bankID == STM32H5xx_FLASH_BANK2_ID)))
#endif
      return ERROR_INVALID_PARAMETER;

   // Debug message
   // TRACE_DEBUG("Erasing Flash sector(s) %" PRIu32 "through %" PRIu32
   // "...\r\n", firstSectorNumber, lastSectorNumber);
   HAL_ICACHE_Disable();

   // Allow access to Flash control registers
   status = HAL_FLASH_Unlock();
   // Is any error?
   if(status != HAL_OK)
   {
      // Debug message
      TRACE_ERROR("Flash Control Register unlock failed!\r\n");
      return CBOOT_ERROR_FAILURE;
   }

   // Start of exception handling block
   do
   {
      // Set flash erase settings
      EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
      EraseInitStruct.Banks = bankID;
      EraseInitStruct.Sector = firstSector;
      EraseInitStruct.NbSectors = nbSectors;

      // Wait for the last flash operation
      // FLASH_WaitForLastOperation((uint32_t)5000U);

      // Erase the specified Flash sector(s)
      status = HAL_FLASHEx_Erase(&EraseInitStruct, &sectorError);

      // Is any error?
      if(status != HAL_OK)
      {
         // Debug message
         flashError = HAL_FLASH_GetError();
         TRACE_ERROR("Failed to erase flash sector(s) %lu, sector = 0x%08lx, "
            "error = %d\r\n",
            firstSector, sectorError, flashError);
         break;
      }
   } while(0);

   // Disable the Flash option control register access (recommended to protect
   // the option Bytes against possible unwanted operations)
   if(HAL_FLASH_Lock() != HAL_OK)
   {
      // Debug message
      TRACE_ERROR("Flash Control Register lock failed!\r\n");
      return CBOOT_ERROR_FAILURE;
   }

   HAL_ICACHE_Enable();
   // Return status code
   return (status == HAL_OK) ? NO_ERROR : ERROR_FAILURE;
}

#ifdef FLASH_DB_MODE

/**
 * @brief Get current used flash bank (STM32H5xx_FLASH_BANK1_ID or
 * STM32H5xx_BANK_2_ID).
 * @param[out] fBankID Pointer to the flash bank ID to be retrieved
 * @return Status code
 **/

error_t flashGetCurrentBank(uint8_t *fBankID)
{
   FLASH_OBProgramInitTypeDef OBInit;
   /* Get the boot configuration status */
   HAL_FLASHEx_OBGetConfig(&OBInit);

   /* Check Swap Flash banks  status */
   if((OBInit.USERConfig & OB_SWAP_BANK_ENABLE) == OB_SWAP_BANK_DISABLE)
   {
      /*Active Bank is bank 1 */
      *fBankID = STM32H5xx_FLASH_BANK1_ID;
   }
   else
   {
      /*Active Bank is bank 2 */
      *fBankID = STM32H5xx_FLASH_BANK2_ID;
   }
   // Successful process
   return NO_ERROR;
}

#else

static inline uint32_t GetFlashSize(void)
{
   volatile uint16_t *sizeReg = (uint16_t *)FLASHSIZE_BASE;

   if(sizeReg == NULL)
   {
      return FLASH_SIZE_DEFAULT;   // Prevent hardfault
   }

   uint16_t size = *sizeReg;

   if(size == 0xFFFFU || size == 0x0000U)
   {
      return FLASH_SIZE_DEFAULT;
   }

   return ((uint32_t)size << 10U);
}
#if 0
#define MY_FLASH_SIZE GetFlashSize()
#define MY_FLASH_BANK_SIZE (MY_FLASH_SIZE >> 1U)

uint32_t GetSector(uint32_t Address)
{
   uint32_t sector = 0;

   if((Address >= FLASH_BASE) && (Address < FLASH_BASE + MY_FLASH_BANK_SIZE))
   {
      sector = (Address & ~FLASH_BASE) / FLASH_SECTOR_SIZE;
   }
   else if((Address >= FLASH_BASE + MY_FLASH_BANK_SIZE) && (Address < FLASH_BASE + FLASH_SIZE))
   {
      sector = ((Address & ~FLASH_BASE) - MY_FLASH_BANK_SIZE) / FLASH_SECTOR_SIZE;
   }
   else
   {
      sector = 0xFFFFFFFF; /* Address out of range */
   }

   return sector;
}
#else
/**
 * @brief  Gets the sector of a given address
 * @param  Addr: Address of the FLASH Memory
 * @retval The sector of a given address
 */
uint32_t GetSector(uint32_t Address)
{
   uint32_t sector = 0;

   if((Address >= FLASH_BASE) && (Address < FLASH_BASE + FLASH_BANK_SIZE))
   {
      sector = (Address & ~FLASH_BASE) / FLASH_SECTOR_SIZE;
   }
   else if((Address >= FLASH_BASE + FLASH_BANK_SIZE) && (Address < FLASH_BASE + FLASH_SIZE))
   {
      sector = ((Address & ~FLASH_BASE) - FLASH_BANK_SIZE) / FLASH_SECTOR_SIZE;
   }
   else
   {
      sector = 0xFFFFFFFF;   /* Address out of range */
   }

   return sector;
}

uint32_t CountSectorsToDelete(uint32_t StartAddress, uint32_t EndAddress)
{
   if(StartAddress > EndAddress)
   {
      // Swap addresses if in reverse order
      uint32_t temp = StartAddress;
      StartAddress = EndAddress;
      EndAddress = temp;
   }

   uint32_t startSector = GetSectorSingleBank(StartAddress);
   uint32_t endSector = GetSectorSingleBank(EndAddress);

   if(startSector == 0xFFFFFFFF || endSector == 0xFFFFFFFF)
   {
      return 0xFFFFFFFF;   // Invalid addresses
   }

   return (startSector > endSector) ? (startSector - endSector + 1)
          : (endSector - startSector + 1);
}

uint32_t GetSectorSingleBank(uint32_t Address)
{
   if(Address < FLASH_BASE || Address >= FLASH_BASE + FLASH_SIZE_DEFAULT)
   {
      return 0xFFFF;   // Invalid address
   }
   return (Address - FLASH_BASE) / FLASH_SECTOR_SIZE;
}
#endif

#if 0
/**
 * @brief  Gets the bank of a given address
 * @param  Addr: Address of the FLASH Memory
 * @retval The bank of a given address
 */
uint32_t GetBank(uint32_t Addr)
{
   uint32_t bank = 0;

   if(READ_BIT(FLASH->OPTSR_CUR, FLASH_OPTSR_SWAP_BANK) == 0)
   {
      /* No Bank swap */
      if(Addr < (FLASH_BASE + MY_FLASH_BANK_SIZE))
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
      if(Addr < (FLASH_BASE + MY_FLASH_BANK_SIZE))
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
#else
/**
 * @brief  Gets the bank of a given address
 * @param  Addr: Address of the FLASH Memory
 * @retval The bank of a given address
 */
uint32_t GetBank(uint32_t Addr)
{
   uint32_t bank = 0;

   if(READ_BIT(FLASH->OPTSR_CUR, FLASH_OPTSR_SWAP_BANK) == 0)
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
#endif
#endif
