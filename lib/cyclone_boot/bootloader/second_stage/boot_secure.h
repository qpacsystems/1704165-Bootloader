/**
 * @file boot_secure.h
 * @brief CycloneBOOT Bootloader Secure Boot related functions
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

#ifndef _BOOT_SECURE_H
#define _BOOT_SECURE_H

#include "modules/image/image.h"
#include "second_stage/boot.h"

// Include crypto header files needed for image verification
#include "ecc/ec.h"
#include "ecc/ecdsa.h"
#include "pkix/pem_key_import.h"
#include "pkc/rsa.h"
#include "pkix/pem_key_import.h"

#define ECDSA_SIGNATURE_SIZE 64 //For curve SECP256K1_CURVE
#define RSA_SIGNATURE_SIZE   256 //For RSA-2048

cboot_error_t bootCheckRuntimeImageSignature(BootContext *context, Slot *slot);

#endif
