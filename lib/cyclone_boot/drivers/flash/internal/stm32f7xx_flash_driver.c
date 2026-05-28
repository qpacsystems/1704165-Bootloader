/**
 * @file stm32f7xx_flash_driver.c
 * @brief STM32F7xx CycloneBOOT flash driver
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
#include "stm32f7xx_flash_driver.h"

#include "core/flash.h"
#include "debug.h"
#include "stm32f7xx.h"
#include "stm32f7xx_hal.h"

#if !defined(FLASH_DB_MODE)
#define SECTORS_LIST_LEN 3
#else
#define SECTORS_LIST_LEN 6
#endif

/**
 * @brief Sector group description
 **/

typedef struct
{
   uint32_t addr;  ///< Flash sector group address
   uint32_t size;  ///< Flash sector group cumulated size
   uint32_t nb;    ///< Number of flash sector in the Flash sector group
} SectorsGroup;

// Internal test memory sectors list
static const SectorsGroup sectorsList[SECTORS_LIST_LEN] = {
#if !defined(FLASH_DB_MODE)
   {STM32F7xx_SECTOR_0_ADDR, STM32F7xx_SECTOR_0_SIZE, 4},
   {STM32F7xx_SECTOR_4_ADDR, STM32F7xx_SECTOR_4_SIZE, 1},
#if defined(FLASH_1MB)
   {STM32F7xx_SECTOR_5_ADDR, STM32F7xx_SECTOR_5_SIZE, 3}
#else
   {STM32F7xx_SECTOR_5_ADDR, STM32F7xx_SECTOR_5_SIZE, 7}
#endif
#else
   {STM32F7xx_SECTOR_0_ADDR, STM32F7xx_SECTOR_0_SIZE, 4},
   {STM32F7xx_SECTOR_4_ADDR, STM32F7xx_SECTOR_4_SIZE, 1},
#if defined(FLASH_1MB)
   {STM32F7xx_SECTOR_5_ADDR, STM32F7xx_SECTOR_5_SIZE, 3},
#else
   {STM32F7xx_SECTOR_5_ADDR, STM32F7xx_SECTOR_5_SIZE, 7},
#endif
   {STM32F7xx_SECTOR_12_ADDR, STM32F7xx_SECTOR_12_SIZE, 4},
   {STM32F7xx_SECTOR_16_ADDR, STM32F7xx_SECTOR_16_SIZE, 1},
#if defined(FLASH_1MB)
   {STM32F7xx_SECTOR_17_ADDR, STM32F7xx_SECTOR_17_SIZE, 3}
#else
   {STM32F7xx_SECTOR_17_ADDR, STM32F7xx_SECTOR_17_SIZE, 7}
#endif
#endif
};

// Flash driver private related functions
error_t stm32f7xxFlashDriverInit(void);
error_t stm32f7xxFlashDriverDeInit(void);
error_t stm32f7xxFlashDriverGetInfo(const FlashInfo **info);
error_t stm32f7xxFlashDriverGetStatus(FlashStatus *status);
error_t stm32f7xxFlashDriverWrite(uint32_t address, uint8_t *data, size_t length);
error_t stm32f7xxFlashDriverRead(uint32_t address, uint8_t *data, size_t length);
error_t stm32f7xxFlashDriverErase(uint32_t address, size_t length);
error_t stm32f7xxFlashDriverSwapBanks(void) __attribute__((section(".code_in_ram")));
error_t stm32f7xxFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr);
bool_t stm32f7xxFlashDriverIsSectorAddr(uint32_t address);
error_t stm32f7xxFlashDriverWriteWord(uint32_t address, uint32_t word);
int_t stm32f7xxFlashGetSector(uint32_t address);
error_t flashGetCurrentBank(uint8_t *fBankID);
error_t stm32f7xxFlashDriverEraseSector(uint32_t firstSector, size_t nbSector);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Memory Information
 **/

const FlashInfo stm32f7xxFlashDriverInfo = {
   FLASH_DRIVER_VERSION, STM32F7xx_NAME, FLASH_TYPE_INTERNAL,
   // MEM_CLASS_FLASH,
   STM32F7xx_ADDR, STM32F7xx_SIZE, STM32F7xx_WRITE_SIZE, STM32F7xx_READ_SIZE,
#ifndef FLASH_DB_MODE
   0, 0, 0, 0, 0
#else
   1, STM32F7xx_BANK_1_SIZE, STM32F7xx_BANK_1_ADDR, STM32F7xx_BANK_2_ADDR, FLASH_FLAGS_LATER_SWAP
#endif
};

/**
 * @brief STM32F7xx Flash driver
 **/

const FlashDriver stm32f7xxFlashDriver = {stm32f7xxFlashDriverInit,
                                          stm32f7xxFlashDriverDeInit,
                                          stm32f7xxFlashDriverGetInfo,
                                          stm32f7xxFlashDriverGetStatus,
                                          stm32f7xxFlashDriverWrite,
                                          stm32f7xxFlashDriverRead,
                                          stm32f7xxFlashDriverErase,
#ifndef FLASH_DB_MODE
                                          NULL,
#else
                                          stm32f7xxFlashDriverSwapBanks,
#endif
                                          stm32f7xxFlashDriverGetNextSector,
                                          stm32f7xxFlashDriverIsSectorAddr};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Initialize Flash Memory.
 * @return Error code
 **/

error_t stm32f7xxFlashDriverInit(void)
{
   // Debug message
   TRACE_INFO("Initializing %s memory...\r\n", STM32F7xx_NAME);

   // Initialize FLASH flags
   //(Patch to fix stm32 hal library wrong initial flash flags issue)
   FLASH_WaitForLastOperation((uint32_t)50000U);

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief De-Initialize Flash Memory.
 * @return Error code
 **/

error_t stm32f7xxFlashDriverDeInit(void)
{
   // Debug message
   TRACE_INFO("De-Initializing %s memory...\r\n", STM32F7xx_NAME);

   // Clear FLASH flags
   __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS | FLASH_FLAG_EOP);

   //(Patch to fix stm32 hal library wrong initial flash flags issue)
   FLASH_WaitForLastOperation((uint32_t)50000U);

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Get Flash Memory information.
 * @param[in,out] info Pointeur to the Memory information structure to be
 * returned
 * @return Error code
 **/

error_t stm32f7xxFlashDriverGetInfo(const FlashInfo **info)
{
   // Set Memory information pointeur
   *info = (const FlashInfo *)&stm32f7xxFlashDriverInfo;

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Get Flash Memory status.
 * @param[in,out] status Pointeur to the Memory status to be returned
 * @return Error code
 **/

error_t stm32f7xxFlashDriverGetStatus(FlashStatus *status)
{
   uint32_t flag;

   // Check parameter vailidity
   if(status == NULL)
      return ERROR_INVALID_PARAMETER;

   do
   {
      // Get Flash Memory error flags status
      flag = __HAL_FLASH_GET_FLAG(FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
         FLASH_FLAG_PGPERR | FLASH_FLAG_ERSERR);
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

error_t stm32f7xxFlashDriverWrite(uint32_t address, uint8_t *data, size_t length)
{
   error_t error;
   uint32_t topAddress;
   const uint8_t *p;
   uint8_t word[4];
   size_t n;

   // Precompute the top address
   topAddress = STM32F7xx_ADDR + STM32F7xx_SIZE;

   // Check address validity
   if((address < STM32F7xx_ADDR || address >= topAddress) || (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(data == NULL ||
      !((address >= STM32F7xx_BANK_1_ADDR && address + length <= STM32F7xx_BANK_2_ADDR) ||
      (address >= STM32F7xx_BANK_2_ADDR && address + length <= topAddress)))
      return ERROR_INVALID_PARAMETER;
#endif

   // Debug message
   TRACE_DEBUG("Writing data (%d bytes) at 0x%08X\r\n", length, address);

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

      // Program 32-bit word in flash memory
      error = stm32f7xxFlashDriverWriteWord(address, *((uint32_t *)word));
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

error_t stm32f7xxFlashDriverRead(uint32_t address, uint8_t *data, size_t length)
{
   uint_t i;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = STM32F7xx_ADDR + STM32F7xx_SIZE;

   // Check address validity
   if(address < STM32F7xx_ADDR || address >= topAddress)
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(data == NULL ||
      !((address >= STM32F7xx_BANK_1_ADDR && address + length <= STM32F7xx_BANK_2_ADDR) ||
      (address >= STM32F7xx_BANK_2_ADDR && address + length <= topAddress)))
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
error_t stm32f7xxFlashDriverErase(uint32_t address, size_t length)
{
   error_t error;
   uint32_t topAddress;
   int_t firstSectorNumber;
   uint32_t lastSectorAddr;
   int_t lastSectorNumber;

   error = NO_ERROR;

   // Precompute the top address
   topAddress = STM32F7xx_ADDR + STM32F7xx_SIZE;

   // Check address validity
   if((address < STM32F7xx_ADDR || address >= topAddress) || (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if((length == 0) || (address + length > topAddress))
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is address in flash bank 1 or 2 only)
   if((length == 0) ||
      !((address >= STM32F7xx_BANK_1_ADDR && address + length <= STM32F7xx_BANK_2_ADDR) ||
      (address >= STM32F7xx_BANK_2_ADDR && address + length <= topAddress)))
      return ERROR_INVALID_PARAMETER;
#endif

   // Get the number of the first sector to erase
   firstSectorNumber = stm32f7xxFlashGetSector(address);

   // Check first sector number is valid (means address must match a sector start
   // address)
   if(firstSectorNumber == -1)
      return ERROR_INVALID_PARAMETER;

   // Get the address of the boundary sector (not to be erased)
   error = stm32f7xxFlashDriverGetNextSector(address + length, &lastSectorAddr);
   // Is any error?
   if(error)
      return error;

   // Get the number of the boundary sector (not to be erased)
   lastSectorNumber = stm32f7xxFlashGetSector(lastSectorAddr);

   // Get the number of the first sector to erase
   firstSectorNumber = stm32f7xxFlashGetSector(address);

   // Erase the required number of sectors
   error =
      stm32f7xxFlashDriverEraseSector(firstSectorNumber, lastSectorNumber - firstSectorNumber);
   // Is any error?
   if(error)
      return error;

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

error_t stm32f7xxFlashDriverSwapBanks(void)
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
         if(fCurrentBankID == STM32F7xx_BANK_1_ID)
         {
            // Debug message
            TRACE_DEBUG("Swaping from flask bank 1 to flash bank 2...\r\n");

            // Configure option bytes to swap on flash bank 2
            OBInit.OptionType = OPTIONBYTE_BOOTADDR_0 | OPTIONBYTE_BOOTADDR_1;
            OBInit.BootAddr0 = __HAL_FLASH_CALC_BOOT_BASE_ADR(STM32F7xx_BANK_2_ADDR);
            OBInit.BootAddr1 = __HAL_FLASH_CALC_BOOT_BASE_ADR(STM32F7xx_BANK_1_ADDR);
         }
         // Swap in flash bank 1
         else
         {
            // Debug message
            TRACE_DEBUG("Swaping from flask bank 2 to flash bank 1...\r\n");

            // Configure option bytes to swap on flash bank 1
            OBInit.OptionType = OPTIONBYTE_BOOTADDR_0 | OPTIONBYTE_BOOTADDR_1;
            OBInit.BootAddr0 = __HAL_FLASH_CALC_BOOT_BASE_ADR(STM32F7xx_BANK_1_ADDR);
            OBInit.BootAddr1 = __HAL_FLASH_CALC_BOOT_BASE_ADR(STM32F7xx_BANK_2_ADDR);
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
   uint32_t bootAddress0;

   // Initialize status code
   error = NO_ERROR;

   // Start handling exception
   do
   {
      // Unlock Flash
      if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != RESET)
      {
         /* Authorize the FLASH Registers access */
         WRITE_REG(FLASH->KEYR, FLASH_KEY1);
         WRITE_REG(FLASH->KEYR, FLASH_KEY2);

         /* Verify Flash is unlocked */
         if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != RESET)
         {
            error = ERROR_ABORTED;
            break;
         }
      }

      // Start handling exception
      do
      {
         // Unlock Option Bytes
         if((FLASH->OPTCR & FLASH_OPTCR_OPTLOCK) != RESET)
         {
            /* Authorizes the Option Byte register programming */
            FLASH->OPTKEYR = FLASH_OPT_KEY1;
            FLASH->OPTKEYR = FLASH_OPT_KEY2;
         }
         else
         {
            error = ERROR_ABORTED;
            break;
         }

         // Read Option Byte BOOT_ADD0
         bootAddress0 = FLASH->OPTCR1 & FLASH_OPTCR1_BOOT_ADD0;

         // Program option bytes
         if(((READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_MEM_BOOT) == SYSCFG_MEM_BOOT_ADD0) &&
            bootAddress0 < 0x2040))
         {
            MODIFY_REG(FLASH->OPTCR1, FLASH_OPTCR1_BOOT_ADD0, ((0x08100000 >> 14) & 0xFFFF));
         }
         else
         {
            MODIFY_REG(FLASH->OPTCR1, FLASH_OPTCR1_BOOT_ADD0, ((0x08000000 >> 14) & 0xFFFF));
         }

         // Launch
         FLASH->OPTCR |= FLASH_OPTCR_OPTSTRT;

         // Lock Option Bytes
         FLASH->OPTCR |= FLASH_OPTCR_OPTLOCK;
      } while(0);

      // Lock Fash
      FLASH->CR |= FLASH_CR_LOCK;
   } while(0);

   // Return status code
   return error;
}

/**
 * @brief Get address of the neighbouring sector
 * @return Error code
 **/

error_t stm32f7xxFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr)
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
   if(address < STM32F7xx_ADDR || address > lastSectorAddr || sectorAddr == NULL)
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
 * @brief Determine if a given address matches the start address of a sector
 * @return boolean
 **/

bool_t stm32f7xxFlashDriverIsSectorAddr(uint32_t address)
{
   int_t sector;

   // Get Flash memory sector number
   sector = stm32f7xxFlashGetSector(address);

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

error_t stm32f7xxFlashDriverEraseSector(uint32_t firstSector, size_t nbSectors)
{
   HAL_StatusTypeDef status;
   FLASH_EraseInitTypeDef EraseInitStruct;
   uint32_t sectorError;

   // Check parameter validity
   if((firstSector >= STM32F7xx_SECTORS_NUMBER) || (nbSectors == 0) ||
      ((firstSector + nbSectors - 1) >= STM32F7xx_SECTORS_NUMBER))
      return ERROR_INVALID_PARAMETER;

   // Initialize FLASH flags
   //(Patch to fix STM32 HAL library wrong initial flash flags issue)
   FLASH_WaitForLastOperation((uint32_t)50000U);

   // Debug message
   TRACE_DEBUG("Erasing Flash sector(s) %" PRIu32 "through %" PRIu32 "...\r\n", firstSector,
      firstSector + nbSectors - 1);

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

      // Initialize FLASH flags
      status = FLASH_WaitForLastOperation((uint32_t)50000U);
      // Is any error?
      if(status != HAL_OK)
      {
         // Debug message
         TRACE_ERROR("Initialize FLASH flags...\r\n");
      }

      // Set flash erase parameters
      EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
      EraseInitStruct.Sector = firstSector;
      EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;   // Erase multiple sectors
      EraseInitStruct.NbSectors = nbSectors;

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

error_t stm32f7xxFlashDriverWriteWord(uint32_t address, uint32_t word)
{
#ifdef FLASH_DB_MODE
   error_t error;
   uint8_t fCurrentBankID;
#endif
   int_t sector;
   uint32_t sectorError;
   uint32_t topAddress;
   HAL_StatusTypeDef status;
   FLASH_EraseInitTypeDef EraseInitStruct;

   // Precompute the top address
   topAddress = STM32F7xx_ADDR + STM32F7xx_SIZE;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if((address + sizeof(uint32_t) >= topAddress) || (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(!((address >= STM32F7xx_BANK_1_ADDR &&
      address + sizeof(uint32_t) <= STM32F7xx_BANK_2_ADDR) ||
      (address >= STM32F7xx_BANK_2_ADDR && address + sizeof(uint32_t) <= topAddress)) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#endif

   // Initialize FLASH flags
   //(Patch to fix stm32 hal library wrong initial flash flags issue)
   FLASH_WaitForLastOperation((uint32_t)50000U);

   // Get Flash memory sector number
   sector = stm32f7xxFlashGetSector(address);

   // Check whether the address match the beginning of a Flash sector.
   // If this is the case then the flash sector must be erased before any write
   // operation
   if(sector >= 0)
   {
#ifdef FLASH_DB_MODE
      // Get current used flash bank
      error = flashGetCurrentBank(&fCurrentBankID);
      // Is any error?
      if(error)
         return error;

      // Running in flash bank2?
      if(fCurrentBankID == STM32F7xx_BANK_2_ID)
      {
         if(sector < FLASH_SECTOR_12)
         {
            sector += 12;
         }
         else
         {
            sector -= 12;
         }
      }
#endif

      // Debug message
      TRACE_DEBUG("Erasing Flash sector %" PRIu32 "...\r\n", sector);

      // Start of exception handling block
      do
      {
         // Initialize FLASH flags
         status = FLASH_WaitForLastOperation((uint32_t)50000U);
         // Is any error?
         if(status != HAL_OK)
         {
            // Debug message
            TRACE_ERROR("Initialize FLASH flags...\r\n");
         }

         // Allow access to Flash control registers and user False
         status = HAL_FLASH_Unlock();
         // Is any error?
         if(status != HAL_OK)
         {
            // Debug message
            TRACE_ERROR("Flash Control Register unlock failed!\r\n");
            break;
         }

         // Set flash erase parameters
         EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
         EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
         EraseInitStruct.Sector = sector;
         EraseInitStruct.NbSectors = 1;

         // Erase the specified Flash sector
         status = HAL_FLASHEx_Erase(&EraseInitStruct, &sectorError);
         // Is any error?
         if(status != HAL_OK)
         {
            // Debug message
            TRACE_ERROR("Failed to erase flash sector %d, error = 0x%08lX!\r\n", sector,
               sectorError);
         }

         // Disable the Flash option control register access (recommended to
         // protect the option Bytes against possible unwanted operations)
         if(HAL_FLASH_Lock() != HAL_OK)
         {
            // Debug message
            TRACE_ERROR("Flash Control Register lock failed!\r\n");
         }
      } while(0);
   }
   else
   {
      // Erase is not needed
      status = HAL_OK;
   }

   // Check status code
   if(status == HAL_OK)
   {
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

         // Program the 32-bit word to Flash memory
         status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, word);
         // Is any error?
         if(status != HAL_OK)
         {
            // Debug message
            TRACE_ERROR("Failed to write word \"0x%08lX\" at 0x%08lX!\r\n", word, address);
         }

         // Disable the Flash option control register access (recommended to
         // protect the option Bytes against possible unwanted operations)
         if(HAL_FLASH_Lock() != HAL_OK)
         {
            // Debug message
            TRACE_ERROR("Flash Control Register lock failed!\r\n");
         }
      } while(0);
   }

   // Return status code
   return (status == HAL_OK) ? NO_ERROR : ERROR_WRITE_FAILED;
}

/**
 * @brief Get the Memory Flash sector number according to the given address.
 * @param[in] address Given Flash Memory address
 * @return Sector number or -1
 **/

int_t stm32f7xxFlashGetSector(uint32_t address)
{
   int_t sector;

#ifndef FLASH_DB_MODE
   if(address == STM32F7xx_SECTOR_0_ADDR)
   {
      sector = 0;
   }
   else if(address == STM32F7xx_SECTOR_1_ADDR)
   {
      sector = 1;
   }
   else if(address == STM32F7xx_SECTOR_2_ADDR)
   {
      sector = 2;
   }
   else if(address == STM32F7xx_SECTOR_3_ADDR)
   {
      sector = 3;
   }
   else if(address == STM32F7xx_SECTOR_4_ADDR)
   {
      sector = 4;
   }
   else if(address == STM32F7xx_SECTOR_5_ADDR)
   {
      sector = 5;
   }
   else if(address == STM32F7xx_SECTOR_6_ADDR)
   {
      sector = 6;
   }
   else if(address == STM32F7xx_SECTOR_7_ADDR)
   {
      sector = 7;
   }
#ifndef FLASH_1MB
   else if(address == STM32F7xx_SECTOR_8_ADDR)
   {
      sector = 8;
   }
   else if(address == STM32F7xx_SECTOR_9_ADDR)
   {
      sector = 9;
   }
   else if(address == STM32F7xx_SECTOR_10_ADDR)
   {
      sector = 10;
   }
   else if(address == STM32F7xx_SECTOR_11_ADDR)
   {
      sector = 11;
   }
#endif
#else
   if(address == STM32F7xx_SECTOR_0_ADDR)
   {
      sector = 0;
   }
   else if(address == STM32F7xx_SECTOR_1_ADDR)
   {
      sector = 1;
   }
   else if(address == STM32F7xx_SECTOR_2_ADDR)
   {
      sector = 2;
   }
   else if(address == STM32F7xx_SECTOR_3_ADDR)
   {
      sector = 3;
   }
   else if(address == STM32F7xx_SECTOR_4_ADDR)
   {
      sector = 4;
   }
   else if(address == STM32F7xx_SECTOR_5_ADDR)
   {
      sector = 5;
   }
   else if(address == STM32F7xx_SECTOR_6_ADDR)
   {
      sector = 6;
   }
   else if(address == STM32F7xx_SECTOR_7_ADDR)
   {
      sector = 7;
   }
#ifdef FLASH_1MB
   else if(address == STM32F7xx_SECTOR_12_ADDR)
   {
      sector = 8;
   }
   else if(address == STM32F7xx_SECTOR_13_ADDR)
   {
      sector = 9;
   }
   else if(address == STM32F7xx_SECTOR_14_ADDR)
   {
      sector = 10;
   }
   else if(address == STM32F7xx_SECTOR_15_ADDR)
   {
      sector = 11;
   }
   else if(address == STM32F7xx_SECTOR_16_ADDR)
   {
      sector = 12;
   }
   else if(address == STM32F7xx_SECTOR_17_ADDR)
   {
      sector = 13;
   }
   else if(address == STM32F7xx_SECTOR_18_ADDR)
   {
      sector = 14;
   }
   else if(address == STM32F7xx_SECTOR_19_ADDR)
   {
      sector = 15;
   }
#else
   else if(address == STM32F7xx_SECTOR_8_ADDR)
   {
      sector = 8;
   }
   else if(address == STM32F7xx_SECTOR_9_ADDR)
   {
      sector = 9;
   }
   else if(address == STM32F7xx_SECTOR_10_ADDR)
   {
      sector = 10;
   }
   else if(address == STM32F7xx_SECTOR_11_ADDR)
   {
      sector = 11;
   }
   else if(address == STM32F7xx_SECTOR_12_ADDR)
   {
      sector = 12;
   }
   else if(address == STM32F7xx_SECTOR_13_ADDR)
   {
      sector = 13;
   }
   else if(address == STM32F7xx_SECTOR_14_ADDR)
   {
      sector = 14;
   }
   else if(address == STM32F7xx_SECTOR_15_ADDR)
   {
      sector = 15;
   }
   else if(address == STM32F7xx_SECTOR_16_ADDR)
   {
      sector = 16;
   }
   else if(address == STM32F7xx_SECTOR_17_ADDR)
   {
      sector = 17;
   }
   else if(address == STM32F7xx_SECTOR_18_ADDR)
   {
      sector = 18;
   }
   else if(address == STM32F7xx_SECTOR_19_ADDR)
   {
      sector = 19;
   }
   else if(address == STM32F7xx_SECTOR_20_ADDR)
   {
      sector = 20;
   }
   else if(address == STM32F7xx_SECTOR_21_ADDR)
   {
      sector = 21;
   }
   else if(address == STM32F7xx_SECTOR_22_ADDR)
   {
      sector = 22;
   }
   else if(address == STM32F7xx_SECTOR_23_ADDR)
   {
      sector = 23;
   }
#endif
#endif
   else
   {
      sector = -1;
   }

   return sector;
}

#ifdef FLASH_DB_MODE

/**
 * @brief Get current used flash bank (STM32F7xx_BANK_1_ID or
 * STM32F7xx_BANK_2_ID).
 * @param[out] fBankID Pointer to the flash bank ID to be retrieved
 * @return Status code
 **/

error_t flashGetCurrentBank(uint8_t *fBankID)
{
   if(READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_SWP_FB))
   {
      *fBankID = STM32F7xx_BANK_2_ID;
   }
   else
   {
      *fBankID = STM32F7xx_BANK_1_ID;
   }

   // Successful process
   return NO_ERROR;
}

#endif
