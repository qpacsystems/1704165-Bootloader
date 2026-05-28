/**
 * @file mx25l512_flash_driver.h
 * @brief CycloneBOOT MX25L512 Flash Driver
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

#ifndef _MX25L512_FLASH_DRIVER_H
#define _MX25L512_FLASH_DRIVER_H

// Dependencies
#include "core/flash.h"
#include "error.h"

#include <stdint.h>
#include <stdlib.h>

// MX25L512 name
#define MX25L512_NAME "MX25L512 External QPSI Nor Flash"
// MX25L512 start addr
#define MX25L512_ADDR 0x00000000
// MX25L512 write size
#define MX25L512_WRITE_SIZE 0x04 // 4-bytes word
// MX25L512 read size
#define MX25L512_READ_SIZE 0x04 // 4-bytes word

// MX25L512 size
#define MX25L512_SIZE 0x4000000
// MX25L512 sectors number
#define MX25L512_SECTORS_NUMBER 1024
// MX25L512 Sectors size
#define MX25L512_SECTORS_SIZE 0x10000
// MX25L512 Subsectors 4KB number
#define MX25L512_SUBSECTORS_NUMBER 16384
// MX25L512 Subsectors 4KB size
#define MX25L512_SUBSECTORS_SIZE 0x1000

// MX25L512 Internal Memory Flash driver
extern const FlashDriver mx25l512FlashDriver;

#endif //!_MX25L512_FLASH_DRIVER_H
