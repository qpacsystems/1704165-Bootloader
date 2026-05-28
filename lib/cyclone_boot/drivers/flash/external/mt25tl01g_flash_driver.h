/**
 * @file mt25tl01g_flash_driver.h
 * @brief CycloneBOOT MT25TL01G Flash Driver
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

#ifndef _N25Q512A_FLASH_DRIVER_H
#define _N25Q512A_FLASH_DRIVER_H

// Dependencies
#include "core/flash.h"
#include "error.h"

#include <stdint.h>
#include <stdlib.h>

// MT25TL01G name
#define MT25TL01G_NAME "MT25TL01G External QPSI Nor Flash"
// MT25TL01G start addr
#define MT25TL01G_ADDR 0x00000000
// MT25TL01G write size
#define MT25TL01G_WRITE_SIZE 0x04 // 4-bytes word
// MT25TL01G read size
#define MT25TL01G_READ_SIZE 0x04 // 4-bytes word

// MT25TL01G size
#define MT25TL01G_SIZE 0x8000000
// MT25TL01G sectors number
#define MT25TL01G_SECTORS_NUMBER 2048
// MT25TL01G Sectors size
#define MT25TL01G_SECTORS_SIZE 0x10000
// MT25TL01G Subsectors 4KB number
#define MT25TL01G_SUBSECTORS_NUMBER 32768
// MT25TL01G Subsectors 4KB size
#define MT25TL01G_SUBSECTORS_SIZE (2 * 0x1000)

// Memory mapped mode
#define MEMORY_XIP_ADDR(addr) (addr + 0x90000000)
#define MEMORY_NO_XIP_ADDR(addr) (addr - 0x90000000)

// MT25TL01G Internal Memory Flash driver
extern const FlashDriver mt25tl01gFlashDriver;

#endif //!_N25Q512A_FLASH_DRIVER_H
