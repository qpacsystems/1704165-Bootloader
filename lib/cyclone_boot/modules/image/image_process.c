/**
 * @file image_process.c
 * @brief CycloneBOOT Image Process routines
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

// Switch to the appropriate trace level
#define TRACE_LEVEL CBOOT_TRACE_LEVEL

// Dependencies
#include "debug.h"
#include "image/image.h"
#include "image/image_utils.h"

// Private function prototypes
cboot_error_t imageProcessOutputBinary(Image *image, uint8_t *data, size_t length);
cboot_error_t imageProcessOutputImage(Image *image, uint8_t *data, size_t length);

/**
 * @brief Process parsed image input data.
 * @param[in,out] context Pointer to the ImageProcess context
 * @return Status code
 **/
cboot_error_t imageProcessInputImage(ImageProcessContext *context)
{
   cboot_error_t cerror;

   // Check parameters validity
   if(context == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Initialize variables
   cerror = CBOOT_NO_ERROR;

   // Image Process receiving image header state?
   if(context->inputImage.state == IMAGE_STATE_RECV_APP_HEADER)
   {
      // Process image header
      cerror = imageProcessAppHeader(context);

      // Is any error?
      if(cerror)
         return cerror;
   }
   // Image Process receiving image data state?
   else if(context->inputImage.state == IMAGE_STATE_RECV_APP_DATA)
   {
      // Process image data
      cerror = imageProcessAppData(context);
      // Is any error?
      if(cerror)
         return cerror;
   }
   // Image Process receiving image check data state?
   else if(context->inputImage.state == IMAGE_STATE_RECV_APP_CHECK)
   {
      // Process image check data
      cerror = imageProcessAppCheck(context);
      // Is any error?
      if(cerror)
         return cerror;
   }
   else
   {
      // Wrong state
      cerror = CBOOT_ERROR_INVALID_STATE;
   }

   // Successful process
   return cerror;
}

/**
 * @brief Process parsed image input data to generate the output image which
 * can either a simple binary (no header and check information) or an image.
 * The image can optionnaly be encrypted and/or bootable (padding added).
 * @param[in,out] context Pointer to the ImageProcess context
 * @param[in] data Output image data chunk to be processed
 * @param[in] length Length of the output data chunk.
 * @return Status code
 **/

cboot_error_t imageProcessOutput(ImageProcessContext *context, uint8_t *data, size_t length)
{
   // Check parameters
   if(context == NULL || data == NULL || length == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Select process function according output slot content type
   if(context->outputImage.activeSlot->cType & SLOT_CONTENT_BINARY)
   {
      return imageProcessOutputBinary(&context->outputImage, data, length);
   }
   else
   {
      return imageProcessOutputImage(&context->outputImage, data, length);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Process parsed image input data to generate output binary.
 * It means that we will keep only the binary part of the input image.
 * @param[in,out] context Pointer to the output image context
 * @param[in] data Output image data chunk to be processed (comming from Input
 * image)
 * @param[in] length Length of the output data chunk.
 * @return Status code
 **/

cboot_error_t imageProcessOutputBinary(Image *image, uint8_t *data, size_t length)
{
   cboot_error_t cerror;
   size_t n;
   size_t written;

   // Check parameters validity
   if(image == NULL || data == NULL || length == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Process incoming data
   while(length > 0)
   {
      // The buffer can hold at most its size
      n = MIN(length, sizeof(image->buffer) - image->bufferLen);

      // Copy the data to the buffer
      memcpy(image->bufferPos, data, n);

      // Advance the data pointer
      data += n;
      // Remaining bytes to process
      length -= n;
      // Update buffer data length
      image->bufferLen += n;
      // Update buffer data free position
      image->bufferPos += n;

      // Process output image data block
      if(image->state == IMAGE_STATE_WRITE_APP_INIT)
      {
         // Set firmware address offset
         image->pos = 0;

         // Write first output image data block (with the flush flag)
         cerror = memoryWriteSlot(image->activeSlot, image->pos, image->buffer, image->bufferLen,
            &written, MEMORY_WRITE_RESET_FLAG);
         // Is any error?
         if(cerror)
         {
            // cerror = ERROR_FAILURE;
            return cerror;
         }

         // Update firmware write address position
         image->pos += written;
         // Update output image data written bytes number
         image->written += image->bufferLen;

         // Update buffer data length
         image->bufferLen = 0;
         // Move remaining buffer data at the beginning of data
         image->bufferPos = image->buffer;

         // Debug message
         TRACE_DEBUG("output written bytes :0x%X/0x%X\r\n", image->written,
            image->firmwareLength);

         // Change state
         imageChangeState(image, IMAGE_STATE_WRITE_APP_DATA);
      }
      else if(image->state == IMAGE_STATE_WRITE_APP_DATA)
      {
         if(image->written + image->bufferLen == image->firmwareLength)
         {
            // Write output image data block
            cerror = memoryWriteSlot(image->activeSlot, image->pos, image->buffer,
               image->bufferLen, &written, MEMORY_WRITE_FORCE_FLAG);
            // Is any error?
            if(cerror)
            {
               // cerror = ERROR_FAILURE;
               return cerror;
            }

            // Update firmware write address position
            image->pos += written;
            // Update output image data written bytes number
            image->written += image->bufferLen;

            // Update buffer data length
            image->bufferLen = 0;
            // Reset buffer data free position
            image->bufferPos = image->buffer;

            // Change state
            imageChangeState(image, IMAGE_STATE_WRITE_APP_END);
         }
         else
         {
            // Write output image data block
            cerror = memoryWriteSlot(image->activeSlot, image->pos, image->buffer,
               image->bufferLen, &written, MEMORY_WRITE_DEFAULT_FLAG);
            // Is any error?
            if(cerror)
            {
               // cerror = ERROR_FAILURE;
               return cerror;
            }

            // Update firmware write address position
            image->pos += written;
            // Update output image data written bytes number
            image->written += image->bufferLen;

            // Update buffer data length
            image->bufferLen = 0;
            // Move remaining buffer data at the beginning of data
            image->bufferPos = image->buffer;
         }

         // Debug message
         TRACE_DEBUG("output written bytes :0x%zX/0x%zX\r\n", image->written,
            image->firmwareLength);
      }
      // End of the image output process
      else
      {
         // For sanity
      }
   }

   // Successful processing
   return CBOOT_NO_ERROR;
}

/**
 * @brief Process parsed image input data to generate output image.
 * The output image will can be either:
 * - a simple image with unencrypted firmware binary and CRC32 integrity tag
 * - a cipher image with encrypted firmware binary and CRC32 integrity tag
 * Also the output image can be bootable (padding added).
 * @param[in,out] context Pointer to the output image context
 * @param[in] data Output image data chunk to be processed (comming from the
 * input image)
 * @param[in] length Length of the output data chunk.
 * @return Status code
 **/

cboot_error_t imageProcessOutputImage(Image *image, uint8_t *data, size_t length)
{
   cboot_error_t cerror;
   size_t n;
   size_t written;
   ImageHeader *imgHeader;
#if ((CIPHER_SUPPORT == ENABLED) && (IMAGE_OUTPUT_ENCRYPTED == ENABLED))
   uint8_t cipherBuff[MAX_CIPHER_BLOCK_SIZE];
#endif

   // Check parameters validity
   if(image == NULL || data == NULL || length == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Debug message
   TRACE_DEBUG("output written bytes :0x%X/0x%X\r\n", image->written, image->firmwareLength);

   // Process the incoming data
   while(length > 0)
   {
      // The buffer can hold at most it size
      n = MIN(length, sizeof(image->buffer) - image->bufferLen);

      // Copy the data to the buffer
      memcpy(image->bufferPos, data, n);

      // Advance the data pointer
      data += n;
      // Remaining bytes to process
      length -= n;
      // Update buffer data length
      image->bufferLen += n;
      // Update buffer data free position
      image->bufferPos += n;

      // Format new Header
      if(image->state == IMAGE_STATE_WRITE_APP_HEADER)
      {
         // Point to image header
         imgHeader = (ImageHeader *)image->buffer;

         // Set new header image index
         imgHeader->imgIndex = image->newImageIdx;

         // Save original image data size
         image->firmwareLength = imgHeader->dataSize;

#if ((CIPHER_SUPPORT == ENABLED) && (IMAGE_OUTPUT_ENCRYPTED == ENABLED))
         // Make sure header image data size is a multiple of encryption algo block
         // size
         if(imgHeader->dataSize % image->cipherEngine.ivLen != 0)
            imgHeader->dataSize +=
               image->cipherEngine.ivLen - (imgHeader->dataSize % image->cipherEngine.ivLen);
#endif
         // Compute new image header crc
         cerror = imageComputeHeaderCrc(imgHeader);
         // Is any error?
         if(cerror)
            return cerror;

         // Update application check computation tag (could be integrity tag or
         // authentication tag or hash signature tag)
         cerror = verifyProcess(&image->verifyContext, (uint8_t *)&imgHeader->headCrc,
            CRC32_DIGEST_SIZE);
         // Is any error?
         if(cerror)
            return cerror;

         // Write new image header (with flush)
         cerror = memoryWriteSlot(image->activeSlot, image->pos, (uint8_t *)imgHeader,
            sizeof(ImageHeader), &written, MEMORY_WRITE_RESET_FLAG);
         if(cerror)
            return cerror;

         // Update firmware write position
         image->pos += written;

#if ((CIPHER_SUPPORT == ENABLED) && (IMAGE_OUTPUT_ENCRYPTED == ENABLED))
         // Update image check data computation tag (crc tag)
         cerror = verifyProcess(&image->verifyContext, image->cipherEngine.iv,
            image->cipherEngine.ivLen);
         if(cerror)
            return cerror;

         // Write new image cipher IV vector into memory
         cerror =
            memoryWriteSlot(image->activeSlot, image->pos, (uint8_t *)image->cipherEngine.iv,
            image->cipherEngine.ivLen, &written, 0);
         if(cerror)
            return cerror;

         // Update firmware write position
         image->pos += written;

         // Prepare magic number with padding to match cipher algo bloc size
         memset(cipherBuff, 0, sizeof(cipherBuff));
         cerror = cipherComputeMagicNumberCrc((uint32_t *)cipherBuff);
         // Is any error?
         if(cerror)
            return cerror;

         // Encrypt padded image magic number
         cerror = cipherEncryptData(&image->cipherEngine, cipherBuff,
            image->cipherEngine.algo->blockSize);
         if(cerror)
            return cerror;

         // Update image check data computation tag (crc tag)
         cerror = verifyProcess(&image->verifyContext, (uint8_t *)cipherBuff,
            image->cipherEngine.algo->blockSize);
         if(cerror)
            return cerror;

         // Write encrypted padded image magic number into memory
         cerror = memoryWriteSlot(image->activeSlot, image->pos, cipherBuff,
            image->cipherEngine.algo->blockSize, &written, 0);
         if(cerror)
            return cerror;

         // Update firmware write position
         image->pos += written;
#endif

         // Reset buffer position
         image->bufferPos = image->buffer;
         // Reset buffer data
         memset(image->buffer, 0x00, sizeof(image->buffer));
         // Reset buffer data length
         image->bufferLen = 0;

         // Change state
         imageChangeState(image, IMAGE_STATE_WRITE_APP_DATA);
      }
      // Format image app
      else if(image->state == IMAGE_STATE_WRITE_APP_DATA)
      {
         // Reached the end of image firmware binary section?
         if(image->written + image->bufferLen == image->firmwareLength)
         {
#if ((CIPHER_SUPPORT == ENABLED) && (IMAGE_OUTPUT_ENCRYPTED == ENABLED))
            // Encryp data bloc size must be a multiple of cipher data block size
            n = (image->bufferLen / 16) * 16;

            // Encrypt image data block
            cerror = cipherEncryptData(&image->cipherEngine, image->buffer, n);
            if(cerror)
               return cerror;

            // Update image check data computation tag (crc tag)
            cerror = verifyProcess(&image->verifyContext, image->buffer, n);
            if(cerror)
               return cerror;

            // Is there any data left in buffer?
            if(n != image->bufferLen)
            {
               // Write encrypted image data into memory
               cerror = memoryWriteSlot(image->activeSlot, image->pos, image->buffer, n,
                  &written, MEMORY_WRITE_DEFAULT_FLAG);
               if(cerror)
                  return cerror;

               // Update firmware write position
               image->pos += written;
               // Update written data
               image->written += n;

               // Update buffer data length
               image->bufferLen -= n;
               // Update buffer position
               image->bufferPos -= image->bufferLen;
               // Move remaining buffer data at the beginning
               memcpy(image->buffer, image->bufferPos, image->bufferLen);
               // Reset buffer position
               image->bufferPos = image->buffer + image->bufferLen;

               // Compute padding size to reach allowed encryption block size
               n = image->cipherEngine.ivLen - image->bufferLen;
               // Add paddind to the remaining data
               memset(image->bufferPos, 0x00, n);
               // Update context buffer length
               image->bufferLen += n;

               // Encrypt last image data block
               cerror =
                  cipherEncryptData(&image->cipherEngine, image->buffer, image->bufferLen);
               if(cerror)
                  return cerror;

               // Update image check data computation tag (crc tag)
               cerror = verifyProcess(&image->verifyContext, image->buffer, image->bufferLen);
               if(cerror)
                  return cerror;
            }

            // Write last encrypted image data block (force write)
            cerror = memoryWriteSlot(image->activeSlot, image->pos, image->buffer,
               image->bufferLen, &written, MEMORY_WRITE_FORCE_FLAG);
            if(cerror)
               return cerror;

            // Update firmware write position
            image->pos += written;
#else
            // Update image check data computation tag (crc tag)
            cerror = verifyProcess(&image->verifyContext, image->buffer, image->bufferLen);
            if(cerror)
               return cerror;

            // Write image data block into memory
            cerror = memoryWriteSlot(image->activeSlot, image->pos, image->buffer,
               image->bufferLen, &written, MEMORY_WRITE_DEFAULT_FLAG);
            if(cerror)
               return cerror;

            // Update firmware write position
            image->pos += written;
#endif

            // Update written data
            image->written += n;

            // Finalize image check data computation tag (crc tag)
            cerror = verifyGenerateCheckData(&image->verifyContext, image->buffer,
               image->verifyContext.imageCheckDigestSize,
               &image->bufferLen);

            // Write new image check data tag (crc tag)
            cerror = memoryWriteSlot(image->activeSlot, image->pos, image->buffer,
               image->bufferLen, &written, MEMORY_WRITE_FORCE_FLAG);
            if(cerror)
               return cerror;

            // Update firmware write position
            image->pos += written;

            // Change state
            imageChangeState(image, IMAGE_STATE_WRITE_APP_END);
         }
         else
         {
#if ((CIPHER_SUPPORT == ENABLED) && (IMAGE_OUTPUT_ENCRYPTED == ENABLED))
            // Encryption data bloc size must be a multiple of cipher data block
            // size
            n = (image->bufferLen / 16) * 16;

            // Encrypt image data block
            cerror = cipherEncryptData(&image->cipherEngine, image->buffer, n);
            if(cerror)
               return cerror;

            // Update image check data computation tag (crc tag)
            cerror = verifyProcess(&image->verifyContext, image->buffer, n);
            if(cerror)
               return cerror;

            // Write encrypted image data into memory
            cerror = memoryWriteSlot(image->activeSlot, image->pos, image->buffer, n, &written,
               MEMORY_WRITE_DEFAULT_FLAG);
            if(cerror)
               return cerror;

            // Update firmware write position
            image->pos += written;

            image->written += n;

            // Update buffer data length
            image->bufferLen -= n;
            // Update buffer position
            image->bufferPos -= image->bufferLen;
            // Move remianing buffer data at the beginning
            memcpy(image->buffer, image->bufferPos, image->bufferLen);
            // Reset buffer position
            image->bufferPos = image->buffer + image->bufferLen;
#else
            // Update image check data computation tag (crc tag)
            cerror = verifyProcess(&image->verifyContext, image->buffer, image->bufferLen);
            if(cerror)
               return cerror;

            // Write encrypted image data into memory
            cerror = memoryWriteSlot(image->activeSlot, image->pos, image->buffer,
               image->bufferLen, &written, MEMORY_WRITE_DEFAULT_FLAG);
            if(cerror)
               return cerror;

            // Update firmware write position
            image->pos += written;

            // Update context buffer length
            image->written += image->bufferLen;

            // Update buffer data length
            image->bufferLen = 0;
            // Reset buffer position
            image->bufferPos = image->buffer;
            // Reset buffer
            memset(image->buffer, 0, sizeof(image->buffer));
#endif
         }
      }
      else
      {
         // For sanity
      }
   }

   // Successful process
   return CBOOT_NO_ERROR;
}
