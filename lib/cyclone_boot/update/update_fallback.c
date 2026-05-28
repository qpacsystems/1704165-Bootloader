/**
 * @file update_fallback.c
 * @brief CycloneBOOT IAP Fallback Functions API
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
#include "update/update_fallback.h"

#include "error.h"

// Extern Flash driver Swap bank (No-Init) routine
extern error_t flashDriverSwapBanksNoInit(void);
// Extern device MCU related function
extern void mcuSystemReset(void);

/**
 * @brief Start CycloneBOOT Dual Bank mode fallback procedure.
 * It will switch flash bank and then restart the MCU.
 * @return Status code
 **/

cboot_error_t updateFallbackStart(void)
{
   error_t error;

   // Start fallback procedure
   error = flashDriverSwapBanksNoInit();
   // Is any error?
   if(error)
   {
      // Return Status code
      return CBOOT_ERROR_FAILURE;
   }
   else
   {
      // Reset MCU
      mcuSystemReset();

      // To avoid compilation warning
      return CBOOT_NO_ERROR;
   }
}
