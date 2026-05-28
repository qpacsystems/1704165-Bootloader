/**
 * @file update_fallback.h
 * @brief CycloneBOOT IAP Fallback Functions API
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

#ifndef _UPDATE_FALLBACK_H
#define _UPDATE_FALLBACK_H

// Dependencies
#include "core/cboot_error.h"

// C++ guard
#ifdef __cplusplus
extern "C"
{
#endif

// CycloneBOOT Dual Bank IAP fallback routines
cboot_error_t updateFallbackStart(void);

// C++ guard
#ifdef __cplusplus
}
#endif

#endif //!_UPDATE_FALLBACK_H
