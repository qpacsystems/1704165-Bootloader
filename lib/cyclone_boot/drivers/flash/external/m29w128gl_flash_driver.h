/**
 * @file m29w128gl_flash_driver.h
 * @brief CycloneBOOT M29W128GL Flash Driver
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

#ifndef _M29W128GL_FLASH_DRIVER_H
#define _M29W128GL_FLASH_DRIVER_H

// Dependencies
#include "core/flash.h"
#include "error.h"

#include <stdint.h>
#include <stdlib.h>

// M29W128GL name
#define M29W128GL_NAME "M29W128GL External Parallel Nor Flash"
// M29W128GL start addr
#define M29W128GL_ADDR 0x00000000
// M29W128GL write size
#define M29W128GL_WRITE_SIZE 0x02 // 2-bytes word
// M29W128GL read size
#define M29W128GL_READ_SIZE 0x02 // 2-bytes word

// M29W128GL size
#define M29W128GL_SIZE 0x8000000
// M29W128GL sectors number
#define M29W128GL_SECTORS_NUMBER 1024
// M29W128GL Sectors size
#define M29W128GL_SECTORS_SIZE 0x20000
// M29W128GL Subsectors 4KB number
#define M29W128GL_SUBSECTORS_NUMBER 1024
// M29W128GL Subsectors 4KB size
#define M29W128GL_SUBSECTORS_SIZE 0x20000

// M29W128GL Internal Memory Flash driver
extern const FlashDriver m29w128glFlashDriver;

#endif //!_M29W128GL_FLASH_DRIVER_H
