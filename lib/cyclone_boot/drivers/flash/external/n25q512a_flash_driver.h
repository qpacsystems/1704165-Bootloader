/**
 * @file n25q512a_flash_driver.h
 * @brief CycloneBOOT N25Q512A Flash Driver
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

// N25Q512A name
#define N25Q512A_NAME "N25Q512A External QPSI Nor Flash"
// N25Q512A start addr
#define N25Q512A_ADDR 0x00000000
// N25Q512A write size
#define N25Q512A_WRITE_SIZE 0x04 // 4-bytes word
// N25Q512A read size
#define N25Q512A_READ_SIZE 0x04 // 4-bytes word

// N25Q512A size
#define N25Q512A_SIZE 0x4000000
// N25Q512A sectors number
#define N25Q512A_SECTORS_NUMBER 1024
// N25Q512A Sectors size
#define N25Q512A_SECTORS_SIZE 0x10000
// N25Q512A Subsectors 4KB number
#define N25Q512A_SUBSECTORS_NUMBER 16384
// N25Q512A Subsectors 4KB size
#define N25Q512A_SUBSECTORS_SIZE 0x1000

// N25Q512A Internal Memory Flash driver
extern const FlashDriver n25q512aFlashDriver;

#endif //!_N25Q512A_FLASH_DRIVER_H
