/**
 * @file at25sf128a_flash_driver.h
 * @brief CycloneBOOT AT25SF128A Flash Driver
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

#ifndef AT25SF128A_FLASH_DRIVER_H
#define AT25SF128A_FLASH_DRIVER_H

// Dependencies
#include "core/flash.h"
#include "error.h"
#include "spi_flash_driver.h"

#include <stdint.h>
#include <stdlib.h>

// MEMORY name
#define AT25SF128A_NAME "AT25SF128A SPI Flash"
// MEMORY start addr
#define AT25SF128A_ADDR 0x00000000
// MEMORY size
#define AT25SF128A_SIZE AT25SF128A_TOTAL_SIZE
// MEMORY write size
#define AT25SF128A_WRITE_SIZE 0x04 // 4-bytes word
// MEMORY read size
#define AT25SF128A_READ_SIZE 0x04 // 4-bytes word

// AT25SF128A Memory Flash driver
extern const FlashDriver at25sf128aFlashDriver;

#endif //! AT25SF128A_FLASH_DRIVER_H
