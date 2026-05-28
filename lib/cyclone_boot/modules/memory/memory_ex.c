/**
 * @file memory_ex.c
 * @brief CycloneBOOT Memory Layer Abstraction (Extended)
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
#define TRACE_LEVEL CBOOT_TRACE_LEVEL

// Dependencies
#include "memory_ex.h"

/**
 * @brief Swap memory banks if and only if the memory is a flash
 * with dual bank capabilities.
 * @param[in] memory Memory pointer
 * @return Status code
 **/

cboot_error_t memoryExSwapBanks(Memory *memory)
{
   error_t error;
   const FlashDriver *fDrv;
   const FlashInfo *fInfo;

   // Initialize status code
   error = NO_ERROR;

   // Check parameters
   if(memory == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Check memory type
   if(memory->memoryType == MEMORY_TYPE_FLASH)
   {
      // Point the memory flash driver
      fDrv = (const FlashDriver *)memory->driver;

      // Get flash memory informations
      error = fDrv->getInfo(&fInfo);
      // Check there is no error
      if(!error)
      {
         // Check flash memory has dualbank capability
         if(fInfo->dualBank == 1)
         {
            // Swap memory banks
            error = fDrv->swapBanks();
         }
      }
   }
   else
   {
      error = ERROR_FAILURE;
   }

   // Return status code
   if(error)
   {
      return CBOOT_ERROR_FAILURE;
   }
   else
   {
      return CBOOT_NO_ERROR;
   }
}
