/**
 * @file cboot_error.h
 * @brief Error codes description
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

#ifndef _CBOOT_ERROR_H
#define _CBOOT_ERROR_H

// Dependencies
#include "error.h"

// C++ guard
#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Error codes
 **/

typedef enum
{

   CBOOT_NO_ERROR,
   CBOOT_ERROR_FAILURE,
   CBOOT_ERROR_NOT_IMPLEMENTED,
   CBOOT_ERROR_ABORTED,
   CBOOT_ERROR_INVALID_PARAMETERS,
   CBOOT_ERROR_INVALID_ADDRESS,
   CBOOT_ERROR_INVALID_VALUE,
   CBOOT_ERROR_INVALID_IMAGE_HEADER,
   CBOOT_ERROR_INVALID_IMAGE_HEADER_VERSION,
   CBOOT_ERROR_INVALID_HEADER_APP_TYPE,
   CBOOT_ERROR_INVALID_IMAGE_CHECK,
   CBOOT_ERROR_MISSING_IMAGE_CHECK_METHOD,
   CBOOT_ERROR_INVALID_IMAGE_VERIFY_METHOD,
   CBOOT_ERROR_INCOMPATIBLE_IMAGE_APP_VERSION,
   CBOOT_ERROR_INCORRECT_IMAGE_APP_VERSION,
   CBOOT_ERROR_INVALID_IMAGE_INTEGRITY_TAG,
   CBOOT_ERROR_INVALID_IMAGE_AUTHENTICATION_TAG,
   CBOOT_ERROR_INVALID_IMAGE_APP,
   CBOOT_ERROR_INVALID_CONFIG,
   CBOOT_ERROR_IMAGE_NOT_READY,
   CBOOT_ERROR_SLOTS_OVERLAP,
   CBOOT_ERROR_UNKNOWN_SLOT_TYPE,
   CBOOT_ERROR_INVALID_STATE,
   CBOOT_ERROR_BUFFER_OVERFLOW,
   CBOOT_ERROR_INVALID_LENGTH,
   CBOOT_ERROR_MEMORY_DRIVER_OPEN_FAILED,
   CBOOT_ERROR_MEMORY_DRIVER_CLOSE_FAILED,
   CBOOT_ERROR_MEMORY_DRIVER_INIT_FAILED,
   CBOOT_ERROR_MEMORY_DRIVER_DEINIT_FAILED,
   CBOOT_ERROR_MEMORY_DRIVER_WRITE_FAILED,
   CBOOT_ERROR_MEMORY_DRIVER_READ_FAILED,
   CBOOT_ERROR_MEMORY_DRIVER_ERASE_FAILED,
   CBOOT_ERROR_MEMORY_DRIVER_GET_STATUS_FAILED,
   CBOOT_ERROR_MEMORY_DRIVER_GET_INFO_FAILED,
   CBOOT_ERROR_UNSUPPORTED_AUTH_ALGO,
   CBOOT_ERROR_UNSUPPORTED_SIGNATURE_ALGO,
   CBOOT_ERROR_UNSUPPORTED_CIPHER_ALGO,
   CBOOT_ERROR_UNSUPPORTED_CIPHER_MODE,
   CBOOT_ERROR_UNKNOWN_MEMORY_TYPE,
   CBOOT_ERROR_UNKNOWN_MEMORY_ROLE,
   CBOOT_ERROR_NO_UPDATE_AVAILABLE,
   CBOOT_ERROR_FALLBACK_FAILURE,
   CBOOT_ERROR_FALLBACK_ABORTED,
   CBOOT_ERROR_SLOT_EMPTY,
   CBOOT_ERROR_FIRMWARE_CORRUPTED,

} cboot_error_t;

// C++ guard
#ifdef __cplusplus
}
#endif

#endif
