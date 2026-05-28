/**
 * @file stm32f4xx_flash_driver.c
 * @brief STM32F4xx CycloneBOOT flash driver
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
#include "stm32f4xx_flash_driver.h"

#include "core/flash.h"
#include "debug.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

#if (defined(FLASH_DB_MODE) || (STM32F4xx_SECTORS_NUMBER == 24))
#define SECTORS_LIST_LEN 6
#else
#define SECTORS_LIST_LEN 3
#endif

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
// Dual bank organization
#if defined(FLASH_DB_BANK)
   {STM32F4xx_SECTOR_0_ADDR, STM32F4xx_SECTOR_0_SIZE, 4},
   {STM32F4xx_SECTOR_4_ADDR, STM32F4xx_SECTOR_4_SIZE, 1},
#if (STM32F4xx_SECTORS_NUMBER == 16)
   {STM32F4xx_SECTOR_5_ADDR, STM32F4xx_SECTOR_5_SIZE, 3},
#elif (STM32F4xx_SECTORS_NUMBER == 24)
   {STM32F4xx_SECTOR_5_ADDR, STM32F4xx_SECTOR_5_SIZE, 7},
#endif
   {STM32F4xx_SECTOR_12_ADDR, STM32F4xx_SECTOR_12_SIZE, 4},
   {STM32F4xx_SECTOR_16_ADDR, STM32F4xx_SECTOR_16_SIZE, 1},
#if (STM32F4xx_SECTORS_NUMBER == 16)
   {STM32F4xx_SECTOR_17_ADDR, STM32F4xx_SECTOR_17_SIZE, 3}
#elif (STM32F4xx_SECTORS_NUMBER == 24)
   {STM32F4xx_SECTOR_17_ADDR, STM32F4xx_SECTOR_17_SIZE, 7}
#endif
// Single bank organization
#else
   {STM32F4xx_SECTOR_0_ADDR, STM32F4xx_SECTOR_0_SIZE, 4},
   {STM32F4xx_SECTOR_4_ADDR, STM32F4xx_SECTOR_4_SIZE, 1},
#if (STM32F4xx_SECTORS_NUMBER == 6)
   {STM32F4xx_SECTOR_5_ADDR, STM32F4xx_SECTOR_5_SIZE, 1}
#elif (STM32F4xx_SECTORS_NUMBER == 8)
   {STM32F4xx_SECTOR_5_ADDR, STM32F4xx_SECTOR_5_SIZE, 3}
#elif (STM32F4xx_SECTORS_NUMBER == 12)
   {STM32F4xx_SECTOR_5_ADDR, STM32F4xx_SECTOR_5_SIZE, 7}
#elif (STM32F4xx_SECTORS_NUMBER == 16)
   {STM32F4xx_SECTOR_5_ADDR, STM32F4xx_SECTOR_5_SIZE, 11}
#elif (STM32F4xx_SECTORS_NUMBER == 24)
   {STM32F4xx_SECTOR_5_ADDR, STM32F4xx_SECTOR_5_SIZE, 7},
   {STM32F4xx_SECTOR_12_ADDR, STM32F4xx_SECTOR_12_SIZE, 4},
   {STM32F4xx_SECTOR_16_ADDR, STM32F4xx_SECTOR_16_SIZE, 1},
   {STM32F4xx_SECTOR_17_ADDR, STM32F4xx_SECTOR_17_SIZE, 7}
#endif
#endif
};

// Memory driver private related functions
error_t stm32f4xxFlashDriverInit(void);
error_t stm32f4xxFlashDriverDeInit(void);
error_t stm32f4xxFlashDriverGetInfo(const FlashInfo **info);
error_t stm32f4xxFlashDriverGetStatus(FlashStatus *status);
error_t stm32f4xxFlashDriverWrite(uint32_t address, uint8_t *data, size_t length);
error_t stm32f4xxFlashDriverRead(uint32_t address, uint8_t *data, size_t length);
error_t stm32f4xxFlashDriverErase(uint32_t address, size_t length);
error_t stm32f4xxFlashDriverSwapBanks(void) __attribute__((section(".code_in_ram")));
error_t stm32f4xxFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr);
bool_t stm32f4xxFlashDriverIsSectorAddr(uint32_t address);
error_t stm32f4xxFlashDriverWriteWord(uint32_t address, uint32_t word);
int_t stm32f4xxFlashGetSector(uint32_t address);
error_t flashGetCurrentBank(uint8_t *fBankID);
error_t stm32f4xxFlashDriverEraseSector(uint32_t firstSector, size_t nbSector);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Memory Information
 **/

const FlashInfo stm32f4xxFlashDriverInfo = {
   FLASH_DRIVER_VERSION, STM32F4xx_NAME, FLASH_TYPE_INTERNAL,
   // MEM_CLASS_FLASH,
   STM32F4xx_ADDR, STM32F4xx_SIZE, STM32F4xx_WRITE_SIZE, STM32F4xx_READ_SIZE,
#ifndef FLASH_DB_MODE
   0, 0, 0, 0, 0
#else
   1, STM32F4xx_BANK_1_SIZE, STM32F4xx_BANK_1_ADDR, STM32F4xx_BANK_2_ADDR, FLASH_FLAGS_LATER_SWAP
#endif
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief STM32F4xx Flash driver
 **/

const FlashDriver stm32f4xxFlashDriver = {stm32f4xxFlashDriverInit,
                                          stm32f4xxFlashDriverDeInit,
                                          stm32f4xxFlashDriverGetInfo,
                                          stm32f4xxFlashDriverGetStatus,
                                          stm32f4xxFlashDriverWrite,
                                          stm32f4xxFlashDriverRead,
                                          stm32f4xxFlashDriverErase,
#ifndef FLASH_DB_MODE
                                          NULL,
#else
                                          stm32f4xxFlashDriverSwapBanks,
#endif
                                          stm32f4xxFlashDriverGetNextSector,
                                          stm32f4xxFlashDriverIsSectorAddr};

/**
 * @brief Initialize Flash Memory.
 * @return Error code
 **/

error_t stm32f4xxFlashDriverInit(void)
{
   // Debug message
   TRACE_INFO("Initializing %s memory...\r\n", STM32F4xx_NAME);

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

error_t stm32f4xxFlashDriverDeInit(void) {
   return ERROR_NOT_IMPLEMENTED;
}

/**
 * @brief Get Flash Memory information.
 * @param[in,out] info Pointeur to the Memory information structure to be
 * returned
 * @return Error code
 **/

error_t stm32f4xxFlashDriverGetInfo(const FlashInfo **info)
{
   // Set Memory information pointeur
   *info = (const FlashInfo *)&stm32f4xxFlashDriverInfo;

   // Successfull process
   return NO_ERROR;
}

/**
 * @brief Get Flash Memory status.
 * @param[in,out] status Pointeur to the Memory status to be returned
 * @return Error code
 **/

error_t stm32f4xxFlashDriverGetStatus(FlashStatus *status)
{
   uint32_t flag;

   // Check parameter vailidity
   if(status == NULL)
      return ERROR_INVALID_PARAMETER;

   do
   {
      // Get Flash Memory error flags status
      flag = __HAL_FLASH_GET_FLAG(FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
         FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

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

error_t stm32f4xxFlashDriverWrite(uint32_t address, uint8_t *data, size_t length)
{
   error_t error;
   uint32_t topAddress;
   const uint8_t *p;
   uint8_t word[4];
   size_t n;

   // Precompute the top address
   topAddress = STM32F4xx_ADDR + STM32F4xx_SIZE;

   // Check address validity
   if((address < STM32F4xx_ADDR || address >= topAddress) || (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(data == NULL ||
      !((address >= STM32F4xx_BANK_1_ADDR && address + length <= STM32F4xx_BANK_2_ADDR) ||
      (address >= STM32F4xx_BANK_2_ADDR && address + length <= topAddress)))
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
      // Prevent to write more than 4 bytes at a time
      n = MIN(sizeof(word), length);

      // Check if remaining bytes is less than 4 (32bits word)
      if(n < sizeof(uint32_t))
         memset(word, 0, sizeof(word));

      // Copy n bytes
      memcpy(word, p, n);

      // Program 32-bit word in flash memory
      error = stm32f4xxFlashDriverWriteWord(address, *((uint32_t *)word));
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

error_t stm32f4xxFlashDriverRead(uint32_t address, uint8_t *data, size_t length)
{
   uint_t i;
   uint32_t topAddress;

   // Precompute the top address
   topAddress = STM32F4xx_ADDR + STM32F4xx_SIZE;

   // Check address validity
   if(address < STM32F4xx_ADDR || address >= topAddress)
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if(data == NULL || address + length > topAddress)
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(data == NULL ||
      !((address >= STM32F4xx_BANK_1_ADDR && address + length <= STM32F4xx_BANK_2_ADDR) ||
      (address >= STM32F4xx_BANK_2_ADDR && address + length <= topAddress)))
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
error_t stm32f4xxFlashDriverErase(uint32_t address, size_t length)
{
   error_t error;
   uint32_t topAddress;
   int_t firstSectorNumber;
   uint32_t lastSectorAddr;
   int_t lastSectorNumber;

   error = NO_ERROR;

   // Precompute the top address
   topAddress = STM32F4xx_ADDR + STM32F4xx_SIZE;

   // Check address validity
   if((address < STM32F4xx_ADDR || address >= topAddress) || (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if((length == 0) || (address + length > topAddress))
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is address in flash bank 1 or 2 only)
   if((length == 0) ||
      !((address >= STM32F4xx_BANK_1_ADDR && address + length <= STM32F4xx_BANK_2_ADDR) ||
      (address >= STM32F4xx_BANK_2_ADDR && address + length <= topAddress)))
      return ERROR_INVALID_PARAMETER;
#endif

   // Get the number of the first sector to erase
   firstSectorNumber = stm32f4xxFlashGetSector(address);

   // Check first sector number is valid (means address must match a sector start
   // address)
   if(firstSectorNumber == -1)
      return ERROR_INVALID_PARAMETER;

   if(address + length == STM32F4xx_ADDR + STM32F4xx_SIZE)
   {
      // Get the number of the boundary sector (not to be erased)
      lastSectorNumber = STM32F4xx_SECTORS_NUMBER;
   }
   else
   {
      // Get the address of the boundary sector (not to be erased)
      error = stm32f4xxFlashDriverGetNextSector(address + length, &lastSectorAddr);
      // Is any error?
      if(error)
         return error;

      // Get the number of the boundary sector (not to be erased)
      lastSectorNumber = stm32f4xxFlashGetSector(lastSectorAddr);
   }

   // Get the number of the first sector to erase
   firstSectorNumber = stm32f4xxFlashGetSector(address);

   // Erase the required number of sectors
   error =
      stm32f4xxFlashDriverEraseSector(firstSectorNumber, lastSectorNumber - firstSectorNumber);
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

error_t stm32f4xxFlashDriverSwapBanks(void)
{
#if defined(FLASH_DB_MODE)
   error_t error;
   HAL_StatusTypeDef status;
   uint8_t fCurrentBankID;
   FLASH_AdvOBProgramInitTypeDef AdvOBInit;

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
         AdvOBInit.OptionType = OPTIONBYTE_BOOTCONFIG;
         HAL_FLASHEx_AdvOBGetConfig(&AdvOBInit);

         // Swap in flash bank 2
         if(fCurrentBankID == STM32F4xx_BANK_1_ID)
         {
            // Debug message
            TRACE_DEBUG("Swaping from flask bank 1 to flash bank 2...\r\n");

            // Configure option bytes to swap on flash bank 2
            AdvOBInit.BootConfig = OB_DUAL_BOOT_ENABLE;
         }
         // Swap in flash bank 1
         else
         {
            // Debug message
            TRACE_DEBUG("Swaping from flask bank 2 to flash bank 1...\r\n");

            // Configure option bytes to swap on flash bank 1
            AdvOBInit.BootConfig = OB_DUAL_BOOT_DISABLE;
         }

         // Start of exception handling block
         do
         {
            // Start Advanced Option Bytes Programming
            status = HAL_FLASHEx_AdvOBProgram(&AdvOBInit);
            // Is any error?
            if(status != HAL_OK)
            {
               // Debug message
               TRACE_ERROR("Advanced Option byte programming failed!\r\n");
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

         // Program option bytes
         if(READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_UFB_MODE))
         {
            CLEAR_BIT(FLASH->OPTCR, FLASH_OPTCR_BFB2);
         }
         else
         {
            SET_BIT(FLASH->OPTCR, FLASH_OPTCR_BFB2);
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Get address of the neighbouring sector
 * @return Error code
 **/

error_t stm32f4xxFlashDriverGetNextSector(uint32_t address, uint32_t *sectorAddr)
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
   if(address < STM32F4xx_ADDR || address > lastSectorAddr || sectorAddr == NULL)
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

bool_t stm32f4xxFlashDriverIsSectorAddr(uint32_t address)
{
   int_t sector;

   // Get Flash memory sector number
   sector = stm32f4xxFlashGetSector(address);

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

error_t stm32f4xxFlashDriverEraseSector(uint32_t firstSector, size_t nbSectors)
{
   HAL_StatusTypeDef status;
   FLASH_EraseInitTypeDef EraseInitStruct;
   uint32_t sectorError;

   // Check parameter validity
   if((firstSector >= STM32F4xx_SECTORS_NUMBER) || (nbSectors == 0) ||
      ((firstSector + nbSectors - 1) >= STM32F4xx_SECTORS_NUMBER))
      return ERROR_INVALID_PARAMETER;

   // Initialize FLASH flags
   //(Patch to fix STM32 HAL library wrong initial flash flags issue)
   FLASH_WaitForLastOperation((uint32_t)50000U);

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

error_t stm32f4xxFlashDriverWriteWord(uint32_t address, uint32_t word)
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
   topAddress = STM32F4xx_ADDR + STM32F4xx_SIZE;

#ifndef FLASH_DB_MODE
   // Check parameters validity (is data in flash)
   if((address + sizeof(uint32_t) >= topAddress) || (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#else
   // Check parameters validity (is data in flash bank 1 or 2 only)
   if(!((address >= STM32F4xx_BANK_1_ADDR &&
      address + sizeof(uint32_t) <= STM32F4xx_BANK_2_ADDR) ||
      (address >= STM32F4xx_BANK_2_ADDR && address + sizeof(uint32_t) <= topAddress)) ||
      (address % sizeof(uint32_t) != 0))
      return ERROR_INVALID_PARAMETER;
#endif

   // Initialize FLASH flags
   //(Patch to fix stm32 hal library wrong initial flash flags issue)
   FLASH_WaitForLastOperation((uint32_t)50000U);

   // Get Flash memory sector number
   sector = stm32f4xxFlashGetSector(address);

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
      if(fCurrentBankID == STM32F4xx_BANK_2_ID)
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

int_t stm32f4xxFlashGetSector(uint32_t address)
{
   int_t sector;
   uint_t i;
   uint_t j;
   SectorsGroup *sGroup;
   int_t tempSector;

   // Initialize sector number
   sector = -1;
   tempSector = 0;

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
            sector = tempSector;
         }
         else
         {
            tempSector++;
         }
      }
   }

   return sector;
}

#ifdef FLASH_DB_MODE

/**
 * @brief Get current used flash bank (STM32F4xx_BANK_1_ID or
 * STM32F4xx_BANK_2_ID).
 * @param[out] fBankID Pointer to the flash bank ID to be retrieved
 * @return Status code
 **/

error_t flashGetCurrentBank(uint8_t *fBankID)
{
   if(READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_UFB_MODE))
   {
      *fBankID = STM32F4xx_BANK_2_ID;
   }
   else
   {
      *fBankID = STM32F4xx_BANK_1_ID;
   }

   // Successful process
   return NO_ERROR;
}

#endif
