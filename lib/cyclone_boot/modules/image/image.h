/**
 * @file image.h
 * @brief CycloneBOOT Image managment
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

#ifndef _IMAGE_H
#define _IMAGE_H

// Dependencies
#include "boot_config.h"
#include "compiler_port.h"
#include "core/cboot_error.h"
#include "core/crypto.h"
#include "memory/memory.h"
#include "core/verify.h"

/*
 * CycloneBOOT Open is licensed under GPL version 2. In particular:
 *
 * - If you link your program to CycloneBOOT Open, the result is a derivative
 *   work that can only be distributed under the same GPL license terms.
 *
 * - If additions or changes to CycloneBOOT Open are made, the result is a
 *   derivative work that can only be distributed under the same license terms.
 *
 * - The GPL license requires that you make the source code available to
 *   whoever you make the binary available to.
 *
 * - If you sell or distribute a hardware product that runs CycloneBOOT Open,
 *   the GPL license requires you to provide public and full access to all
 *   source code on a nondiscriminatory basis.
 *
 * If you fully understand and accept the terms of the GPL license, then edit
 * the os_port_config.h header and add the following directive:
 *
 * #define GPL_LICENSE_TERMS_ACCEPTED
 */

#ifndef GPL_LICENSE_TERMS_ACCEPTED
   #error Before compiling CycloneBOOT Open, you must accept the terms of the GPL license
#endif

// Image input encryption support
#ifndef IMAGE_INPUT_ENCRYPTED
#define IMAGE_INPUT_ENCRYPTED DISABLED
#elif (IMAGE_INTPUT_ENCRYPTED != ENABLED && IMAGE_INTPUT_ENCRYPTED != DISABLED)
#error IMAGE_INTPUT_ENCRYPTED parameter is not valid!
#endif

// Image output encryption support (for encrypted external memory if needed)
#ifndef IMAGE_OUTPUT_ENCRYPTED
#define IMAGE_OUTPUT_ENCRYPTED DISABLED
#elif (IMAGE_OUTPUT_ENCRYPTED != ENABLED && IMAGE_OUTPUT_ENCRYPTED != DISABLED)
#error IMAGE_OUTPUT_ENCRYPTED parameter is not valid!
#endif

// Acceptable input or output image encryption support
#if (((IMAGE_OUTPUT_ENCRYPTED == ENABLED) || (IMAGE_INTPUT_ENCRYPTED == ENABLED)) &&               \
   (CIPHER_SUPPORT == DISABLED))
#error CIPHER_SUPPORT MUST be ENABLED if IMAGE_INPUT_ENCRYPTED or IMAGE_OUTPUT_ENCRYPTED is enabled!
#endif

// Add image encryption related dependencies
#if (CIPHER_SUPPORT == ENABLED) &&                                                                 \
   ((IMAGE_INPUT_ENCRYPTED == ENABLED) || (IMAGE_OUTPUT_ENCRYPTED == ENABLED))
#include "security/cipher.h"
#endif

// Maximum image check data size
#define IMAGE_MAX_CHECK_DATA_SIZE 512

/**
 * @brief Image type definition
 **/

typedef enum
{
   IMAGE_TYPE_APP = 0,
   IMAGE_TYPE_BOOT,
   IMAGE_TYPE_NONE
} ImageType;

/**
 * @brief Image states
 **/

typedef enum
{
   IMAGE_STATE_IDLE,
   IMAGE_STATE_RECV_APP_HEADER,
   IMAGE_STATE_RECV_APP_DATA,
   IMAGE_STATE_RECV_APP_CHECK,
   IMAGE_STATE_VALIDATE_APP,
   IMAGE_STATE_APP_REBOOT,
   IMAGE_STATE_WRITE_APP_INIT,
   IMAGE_STATE_WRITE_APP_HEADER,
   IMAGE_STATE_WRITE_APP_DATA,
   IMAGE_STATE_WRITE_APP_CHECK,
   IMAGE_STATE_WRITE_APP_END
} ImageState;

/**
 * @brief Image version comparison flags definition
 **/

typedef enum
{
   IMAGE_VERSION_EQUAL,
   IMAGE_VERSION_SUPERIOR,
   IMAGE_VERSION_INFERIOR
} ImageVersionComparisonFlag;

// Image Header Major version
#define IMAGE_HEADER_VERSION_MAJOR 1
// Image Header Minor version
#define IMAGE_HEADER_VERSION_MINOR 3
// Image Header Revision number
#define IMAGE_HEADER_VERSION_PATCH 0
// Image Header version
#define IMAGE_HEADER_VERSION                                                                       \
        (uint32_t)(((IMAGE_HEADER_VERSION_MAJOR & 0xFF) << 16) |                                       \
        ((IMAGE_HEADER_VERSION_MINOR & 0xFF) << 8) | (IMAGE_HEADER_VERSION_PATCH & 0xFF))

#if ((defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) || defined(__GNUC__) ||            \
   defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__) || defined(__TASKING__) ||                  \
   defined(__CWCC__) || defined(__TI_ARM__))

/**
 *@brief Image version field definition
 **/
typedef __packed_struct
{
   uint8_t major;
   uint8_t minor;
   uint16_t revision;
   uint32_t buildNum;
}
ImageVersion;

/**
 * @brief Image header definition
 **/

typedef __packed_struct
{
   uint32_t headVers;      ///< Image header version
   uint32_t imgIndex;      ///< Image index
   uint8_t imgType;        ///< Image type
   uint32_t dataPadding;   ///< Image data padding
   uint32_t dataSize;      ///< Image data size
   uint32_t binarySize;    ///<Image firmware size
   ImageVersion dataVers;  ///< Image data version
   uint64_t imgTime;       ///< Image data generated time
   uint8_t reserved[23];   ///< Reserved field
   uint32_t headCrc;       ///< Image header CRC32 integrity tag
}
ImageHeader;

#else

#undef interface
#undef __start_packed
#define __start_packed __pragma(pack(push, 1))
#undef __end_packed
#define __end_packed __pragma(pack(pop))
#define __weak

/**
 *@brief Image version field definition
 **/
__start_packed typedef struct
{
   uint8_t major;
   uint8_t minor;
   uint16_t revision;
   uint32_t buildNum;
} ImageVersion __end_packed;

/**
 * @brief Image header definition
 **/

__start_packed typedef struct
{
   uint32_t headVers;      ///< Image header version
   uint32_t imgIndex;      ///< Image index
   uint8_t imgType;        ///< Image type
   uint32_t dataPadding;   ///< Image data padding
   uint32_t dataSize;      ///< Image data size
   uint32_t binarySize;    ///<Image firmware size
   ImageVersion dataVers;  ///< Image data version
   uint64_t imgTime;       ///< Image data generated time
   uint8_t reserved[23];   ///< Reserved field
   uint32_t headCrc;       ///< Image header CRC32 integrity tag
} ImageHeader __end_packed;

#endif

/**
 * @brief Image context definition
 **/

typedef struct
{
   uint8_t buffer[128];  ///< Image processing buffer
   uint8_t *bufferPos;   ///< Position in image processing buffer
   size_t bufferLen;     ///< Number of byte in image processing buffer

   Slot *activeSlot;  ///< Pointer to the slot to write the image in

   uint16_t newImageIdx;  ///< Image index number

   uint32_t firmwareAddr;  ///< Image firmware data write address
   size_t firmwareLength;  ///< Image data firmware length
   uint32_t pos;           ///< Image current firmware data write position
   size_t written;         ///< Current written firmware data byte number

   ImageState state;

#if ((CIPHER_SUPPORT == ENABLED) &&                                                                \
   ((IMAGE_INPUT_ENCRYPTED == ENABLED) || (IMAGE_OUTPUT_ENCRYPTED == ENABLED)))
   CipherEngine cipherEngine;  ///< Image cipher engine
   bool_t ivRetrieved;
   uint32_t magicNumberCrc;
   bool_t magicNumberCrcRetrieved;
#endif

   VerifyContext verifyContext;  ///< Image verification context

   uint8_t checkData[IMAGE_MAX_CHECK_DATA_SIZE];  ///< Image check data buffer
   uint8_t *checkDataPos;                         ///< Position in image check data buffer
   size_t checkDataLen;   ///< Current number of byte in image check data buffer
   size_t checkDataSize;  ///< Image check data buffer size

} Image;

/**
 * @brief Image Anti-Rollback callback definition
 **/

typedef bool_t (*ImageAntiRollbackCallback)(ImageVersion currentAppVersion,
   ImageVersion updateAppVersion);

/**
 * @brief Image Process context definition
 **/

typedef struct
{
   Image inputImage;   ///< Input Image context
   Image outputImage;  ///< Output Image context

   ImageVersion currentAppVersion;                     ///< Current Application version
   ImageAntiRollbackCallback imgAntiRollbackCallback;  ///< Anti-Rollback callback

   Memory *memories;  ///< Memories list
} ImageProcessContext;

// C++ guard
#ifdef __cplusplus
extern "C"
{
#endif

// CycloneBOOT Image related functions
cboot_error_t imageCheckHeader(ImageHeader *header);
cboot_error_t imageGetHeader(uint8_t *buffer, size_t bufferLen, ImageHeader **header);
cboot_error_t imageComputeHeaderCrc(ImageHeader *header);
cboot_error_t imageCheckIntegrity(Image *image);

// C++ guard
#ifdef __cplusplus
}
#endif
#endif //!_IMAGE_H
