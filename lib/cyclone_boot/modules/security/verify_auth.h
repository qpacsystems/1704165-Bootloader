/**
 * @file verify_auth.h
 * @brief CycloneBOOT Image data authentication module
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

#ifndef _VERIFY_AUTH_H
#define _VERIFY_AUTH_H

// Dependencies
#include "security/verify.h"

// C++ guard
#ifdef __cplusplus
extern "C"
{
#endif

// CycloneBOOT authentication verification related functions
cboot_error_t authInit(VerifyContext *verifyContext);
cboot_error_t authUpdateTag(VerifyContext *verifyContext, const uint8_t *data, size_t length);
cboot_error_t authFinalizeTag(VerifyContext *verifyContext);

// C++ guard
#ifdef __cplusplus
}
#endif
#endif // !_VERIFY_AUTH_H
