/**
 * @file memory.c
 * @brief CycloneBOOT Memory Layer Abstraction
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
#include "memory.h"

#include "debug.h"
#include "os_port.h"

#include <stdio.h>
#include <string.h>

#if !((defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) || defined(__GNUC__) ||           \
   defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__) || defined(__TASKING__) ||                 \
   defined(__CWCC__) || defined(__TI_ARM__))
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

// Private global variables
static uint8_t memWriteBuffer[64] = {0};
static uint8_t *memWriteBufferPos = memWriteBuffer;
static size_t memWriteBufferLen = 0;

// Private memory-related routines prototypes
cboot_error_t slotsInit(Memory *memory);
bool_t isSlotsOverlap(Slot *slot1, Slot *slot2);
cboot_error_t cleanupSlotHandler(Slot *slot);

/**
 * @brief Memory initialization function
 **/

cboot_error_t memoryInit(Memory *memories, size_t nbMemories)
{
   error_t error;
   cboot_error_t cerror;
   uint_t i;
   const void *memoryDriver;
   MemoryType memoryType;
   MemoryRole memoryRole;
   Memory *memory;

   // Check parameters
   if(memories == NULL || nbMemories == 0 || nbMemories > NB_MEMORIES)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Initialize memories
   for(i = 0; i < nbMemories; i++)
   {
      memory = &memories[i];
      memoryDriver = memory->driver;
      memoryType = memory->memoryType;
      memoryRole = memory->memoryRole;

      // Check memory role
      if(memoryRole < MEMORY_ROLE_PRIMARY || memoryRole > MEMORY_ROLE_SECONDARY)
         return CBOOT_ERROR_UNKNOWN_MEMORY_ROLE;

      // Check driver validity
      if(memoryDriver == NULL)
         return CBOOT_ERROR_INVALID_PARAMETERS;

      // Check memory type
      if(memoryType == MEMORY_TYPE_FLASH)
      {
         // Initialize flash memory driver
         error = ((const FlashDriver *)memoryDriver)->init();
         if(error != NO_ERROR)
            return CBOOT_ERROR_MEMORY_DRIVER_INIT_FAILED;
      }
#if (MEMORIES_FS_SUPPORT == ENABLED)
      else if(memoryType == MEMORY_TYPE_FS)
      {
         // Initialize file system memory driver
         error = ((const FsDriver *)memoryDriver)->init();
         if(error != NO_ERROR)
            return CBOOT_ERROR_MEMORY_DRIVER_INIT_FAILED;
      }
#endif
      else
      {
         // Unknown memory type
         return CBOOT_ERROR_UNKNOWN_MEMORY_TYPE;
      }

      // Initialize slots
      cerror = slotsInit(memory);
      // Is any error?
      if(cerror)
         return cerror;
   }

   // Succesful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Memory de-initialization function
 **/

cboot_error_t memoryDeInit(Memory *memories, size_t nbMemories)
{

   error_t error;
   uint_t i;
   const void *memoryDriver;
   MemoryType memoryType;

   // Check parameters
   if(memories == NULL || nbMemories == 0 || nbMemories > NB_MEMORIES)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   for(i = 0; i < nbMemories; i++)
   {
      memoryDriver = memories[i].driver;
      memoryType = memories[i].memoryType;

      if(memoryType == MEMORY_TYPE_FLASH)
      {
         // Deinitialize flash memory driver
         error = ((const FlashDriver *)memoryDriver)->deInit();
         if(error != NO_ERROR)
            return CBOOT_ERROR_MEMORY_DRIVER_DEINIT_FAILED;
      }
#if (MEMORIES_FS_SUPPORT == ENABLED)
      else if(memoryType == MEMORY_TYPE_FS)
      {
         // Deinitialize file system memory driver
         error = ((const FsDriver *)memoryDriver)->deInit();
         if(error != NO_ERROR)
         {
            return CBOOT_ERROR_MEMORY_DRIVER_DEINIT_FAILED;
         }
      }
#endif
      else
      {
         // Unknown memory type
         return CBOOT_ERROR_UNKNOWN_MEMORY_TYPE;
      }
   }

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Memory get driver information function
 **/
cboot_error_t memoryGetInfo(Memory *memory, MemoryInfo *info)
{
   error_t error;
   const void *mInfo;

   // Check parameters
   if(memory == NULL || info == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Is memory a flash?
   if(memory->memoryType == MEMORY_TYPE_FLASH)
   {
      // Get flash memory info
      error = ((const FlashDriver *)memory->driver)->getInfo((const FlashInfo **)&mInfo);
      // Is any error?
      if(error)
         return CBOOT_ERROR_MEMORY_DRIVER_GET_INFO_FAILED;

      // Format memory info
      info->addr = ((const FlashInfo *)mInfo)->flashAddr;
      info->size = ((const FlashInfo *)mInfo)->flashSize;
      info->bank1Addr = ((const FlashInfo *)mInfo)->bank1Addr;
      info->bank2Addr = ((const FlashInfo *)mInfo)->bank2Addr;
      info->bankSize = ((const FlashInfo *)mInfo)->bankSize;
      info->writeSize = ((const FlashInfo *)mInfo)->writeSize;
      info->flags = ((const FlashInfo *)mInfo)->flags;
   }
#if (MEMORIES_FS_SUPPORT == ENABLED)
   // Is memory a file system?
   else if(memory->memoryType == MEMORY_TYPE_FS)
   {
      // Get filesystem memory info
      error = ((const FsDriver *)memory->driver)->getInfo((const FsInfo **)&mInfo);
      // Is any error?
      if(error)
         return CBOOT_ERROR_MEMORY_DRIVER_GET_INFO_FAILED;

      // Format memory info
      info->size = ((const FsInfo *)mInfo)->fsSize;
      info->flags = ((const FsInfo *)mInfo)->flags;
   }
#endif
   else
   {
      // Unknown memory type
      return CBOOT_ERROR_UNKNOWN_MEMORY_TYPE;
   }

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Get Memory Status function
 **/

cboot_error_t memoryGetSlotStatus(Slot *slot, SlotStatus *status)
{
   error_t error = NO_ERROR;
   FlashStatus flashStatus = FLASH_STATUS_OK;
#if (MEMORIES_FS_SUPPORT == ENABLED)
   FsStatus fsStatus = FS_STATUS_OK;
#endif

   const void *memoryDriver = ((const Memory *)slot->memParent)->driver;

   if(slot->type == SLOT_TYPE_DIRECT)
   {
      error = ((const FlashDriver *)memoryDriver)->getStatus(&flashStatus);
      if(error)
         return CBOOT_ERROR_MEMORY_DRIVER_GET_STATUS_FAILED;
      *status = (SlotStatus)flashStatus;
   }
#if (MEMORIES_FS_SUPPORT == ENABLED)
   else if(slot->type == SLOT_TYPE_FILE)
   {
      error = ((const FsDriver *)memoryDriver)->getStatus(&fsStatus);
      if(error)
         return CBOOT_ERROR_MEMORY_DRIVER_GET_STATUS_FAILED;
      *status = (SlotStatus)fsStatus;
   }
#endif
   else
   {
      return CBOOT_ERROR_UNKNOWN_SLOT_TYPE;
   }

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Write Data into Memory function
 **/

cboot_error_t memoryWriteSlot(Slot *slot, uint32_t offset, uint8_t *buffer, size_t length,
   size_t *written, uint8_t flag)
{
   cboot_error_t cboot_error;
   error_t error;
   size_t n;
   size_t writeBlockSize;
   Memory *memory;
   MemoryInfo memoryInfo;
   const void *memoryDriver;

   // Check parameters validity
   if(slot == NULL || buffer == NULL || written == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Get memory driver
   memory = (Memory *)slot->memParent;
   memoryDriver = memory->driver;

   // Initialize variables
   cboot_error = CBOOT_NO_ERROR;
   error = NO_ERROR;
   *written = 0;

   if(slot->type == SLOT_TYPE_DIRECT)
   {
      // Get memory driver information
      cboot_error = memoryGetInfo(memory, &memoryInfo);
      // Is any error?
      if(cboot_error)
         return cboot_error;

      // Get memory driver write block size
      writeBlockSize = memoryInfo.writeSize;

      // Reset of memory write buffer required?
      if(flag == MEMORY_WRITE_RESET_FLAG)
      {
         memoryResetWriteBuffer();
      }

      // Process incoming data
      while(length > 0)
      {
         // Fill temporary buffer to reach allowed flash memory write block size
         n = MIN(length, writeBlockSize - memWriteBufferLen);

         // Fill buffer
         memcpy(memWriteBufferPos, buffer, n);
         // Update temporary buffer position
         memWriteBufferPos += n;
         // Update temporary buffer length
         memWriteBufferLen += n;
         // Advance data pointer
         buffer += n;
         // Remaining bytes to process
         length -= n;

         // Enough data to write?
         if(memWriteBufferLen == writeBlockSize)
         {
            // Write image data into memory
            error = ((const FlashDriver *)memoryDriver)
               ->write(slot->addr + offset, (uint8_t *)memWriteBuffer, writeBlockSize);
            // Is any error?
            if(error)
            {
               // Debug message
               TRACE_ERROR("Failed to write image data into flash memory!\r\n");
               return CBOOT_ERROR_FAILURE;
            }

            // Update written bytes
            *written += writeBlockSize;

            // Increase offset
            offset += writeBlockSize;

            // Reset temporary buffer position
            memWriteBufferPos = memWriteBuffer;
            // Reset temporary buffer length
            memWriteBufferLen = 0;
         }
      }

      // Force writting of memory write buffer required?
      if(memWriteBufferLen != 0 && flag == MEMORY_WRITE_FORCE_FLAG)
      {
         // Complete buffer with padding to reach minimum allowed write block size
         memset(memWriteBufferPos, 0x00, writeBlockSize - memWriteBufferLen);

         // Write image data into external flash memory
         error = ((const FlashDriver *)memoryDriver)
            ->write(slot->addr + offset, (uint8_t *)memWriteBuffer, writeBlockSize);
         // Is any error?
         if(error)
         {
            // Debug message
            TRACE_ERROR("Failed to write image data into memory!\r\n");
            return CBOOT_ERROR_FAILURE;
         }

         // Update written bytes
         *written += writeBlockSize;

         // Increase offset
         offset += writeBlockSize;

         // Reset temporary buffer position
         memWriteBufferPos = memWriteBuffer;
         // Reset temporary buffer length
         memWriteBufferLen = 0;
      }
   }
#if (MEMORIES_FS_SUPPORT == ENABLED)
   else if(slot->type == SLOT_TYPE_FILE)
   {
      error = ((const FsDriver *)memoryDriver)->write(slot->file, offset, buffer, length);
      if(error)
      {
         cleanupSlotHandler(slot);
         return CBOOT_ERROR_MEMORY_DRIVER_WRITE_FAILED;
      }
   }
#endif
   else
   {
      return CBOOT_ERROR_UNKNOWN_SLOT_TYPE;
   }

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Read Data from Memory function
 **/

cboot_error_t memoryReadSlot(Slot *slot, uint32_t offset, uint8_t *buffer, size_t length)
{
   error_t error = NO_ERROR;

   const void *memoryDriver = ((const Memory *)slot->memParent)->driver;

   if(slot->type == SLOT_TYPE_DIRECT)
   {
      error = ((const FlashDriver *)memoryDriver)->read(slot->addr + offset, buffer, length);
      if(error)
      {
         cleanupSlotHandler(slot);
         return CBOOT_ERROR_MEMORY_DRIVER_READ_FAILED;
      }
   }
#if (MEMORIES_FS_SUPPORT == ENABLED)
   else if(slot->type == SLOT_TYPE_FILE)
   {
      error = ((const FsDriver *)memoryDriver)->read(slot->file, offset, buffer, length);
      if(error)
      {
         cleanupSlotHandler(slot);
         return CBOOT_ERROR_MEMORY_DRIVER_READ_FAILED;
      }
   }
#endif
   else
   {
      return CBOOT_ERROR_UNKNOWN_SLOT_TYPE;
   }

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Copy source slot content to destination slot
 * @param[in] src Pointer to source slot to copy from
 * @param[out] dst Pointer to destination slot to copy in
 * @param[in] bytesNumber number of bytes to be copied from src slot to dst slot
 * @return Status code
 **/

cboot_error_t memoryCopySlot(Slot *src, Slot *dst, size_t bytesNumber)
{
   cboot_error_t cerror;
   uint8_t buffer[512];
   size_t readOffset;
   size_t writeOffset;
   size_t written;
   MemoryInfo srcMemInfo;
   MemoryInfo dstMemInfo;

   // Check parameters
   if(src == NULL || dst == NULL || bytesNumber == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Get source slot memory parent information
   cerror = memoryGetInfo((Memory *)(src->memParent), &srcMemInfo);
   if(cerror)
      return cerror;

   // Get destination slot memory parent information
   cerror = memoryGetInfo((Memory *)(dst->memParent), &dstMemInfo);
   if(cerror)
      return cerror;

   // Check the number of bytes to copy isn't above respecitve slots size
   if(bytesNumber >= srcMemInfo.size || bytesNumber >= dstMemInfo.size)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   // Reset memory write buffer
   memoryResetWriteBuffer();

   readOffset = 0;
   writeOffset = 0;
   written = 0;

   while(bytesNumber >= sizeof(buffer))
   {
      // Read data from source slot
      cerror = memoryReadSlot(src, readOffset, buffer, sizeof(buffer));
      if(cerror)
         return cerror;

      // Write data to destination slot
      cerror = memoryWriteSlot(dst, writeOffset, buffer, sizeof(buffer), &written,
         MEMORY_WRITE_DEFAULT_FLAG);
      if(cerror)
         return cerror;

      readOffset += sizeof(buffer);
      writeOffset += written;
      bytesNumber -= sizeof(buffer);
   }

   // Remaining data to be copied ?
   if(bytesNumber > 0)
   {
      // Read last data from source slot
      cerror = memoryReadSlot(src, readOffset, buffer, bytesNumber);
      if(cerror)
         return cerror;

      // Write last data to destination slot (with force flag
      //  in case data still remains in memory write buffer)
      cerror = memoryWriteSlot(dst, writeOffset, buffer, bytesNumber, &written,
         MEMORY_WRITE_FORCE_FLAG);
      if(cerror)
         return cerror;
   }

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Erase Data from Memory function
 **/

cboot_error_t memoryEraseSlot(Slot *slot, uint32_t offset, size_t length)
{
   error_t error = NO_ERROR;

   if(slot == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   const void *memoryDriver = ((const Memory *)slot->memParent)->driver;

   if(slot->type == SLOT_TYPE_DIRECT)
   {
      error = ((const FlashDriver *)memoryDriver)->erase(slot->addr + offset, length);
      if(error)
      {
         cleanupSlotHandler(slot);
         return CBOOT_ERROR_MEMORY_DRIVER_ERASE_FAILED;
      }
   }
#if (MEMORIES_FS_SUPPORT == ENABLED)
   else if(slot->type == SLOT_TYPE_FILE)
   {
      error = ((const FsDriver *)memoryDriver)->erase(slot->file, offset, length);
      if(error)
      {
         cleanupSlotHandler(slot);
         return CBOOT_ERROR_MEMORY_DRIVER_ERASE_FAILED;
      }
   }
#endif
   else
   {
      return CBOOT_ERROR_UNKNOWN_SLOT_TYPE;
   }

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Compare two slots
 **/
void memoryCompareSlot(Slot *src, Slot *dst, uint8_t *result)
{
   // TODO: slots in a file system not compared.
   if(src->type != dst->type || src->cType != dst->cType || src->memParent != dst->memParent)
   {
      *result = 1;   // Different
   }
   else if(src->addr != dst->addr || src->size != dst->size)
   {
      *result = 1;   // Different
   }
   else
   {
      *result = 0;   // Identical
   }
}

cboot_error_t memoryCleanup(Memory *memories, size_t nbMemories)
{
   uint8_t i;
   uint8_t j;
   Memory *memory;
   Slot *slot;

   // Check parameters
   if(memories == NULL || nbMemories == 0)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   for(i = 0; i < nbMemories; i++)
   {
      memory = &memories[i];

      // Loop through memory slots
      for(j = 0; j < memory->nbSlots; j++)
      {
         slot = &memory->slots[j];
         cleanupSlotHandler(slot);
      }
   }

   // Successful process
   return CBOOT_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool_t isSlotsOverlap(Slot *slot1, Slot *slot2)
{
   bool_t res;

   res = 0;

   if(slot1 != NULL && slot2 != NULL && slot1->type == slot2->type)
   {
      if(slot1->type == SLOT_TYPE_DIRECT)
      {
         // Are flash slots 1 & 2 overlapping?
         if(((slot1->addr >= slot2->addr) && (slot1->addr < (slot2->addr + slot2->size))) ||
            ((slot2->addr >= slot1->addr) && (slot2->addr < (slot1->addr + slot1->size))))
         {
            // Slots overlapping
            res = 1;
         }
      }
#if (MEMORIES_FS_SUPPORT == ENABLED)
      else if(slot1->type == SLOT_TYPE_FILE)
      {
         // Are file slot overlapping (they have the same path)?
         if(strcasecmp(slot1->path, slot2->path) == 0)
         {
            // Slots overlapping
            res = 1;
         }
      }
#endif
      else
      {
         // Unknown slot type
         res = 1;
      }
   }
   else
   {
      // Bad parameters
      res = 1;
   }

   // Return result
   return res;
}

cboot_error_t cleanupSlotHandler(Slot *slot)
{
   error_t error;
   Memory const *memory = (Memory *)slot->memParent;

#if (MEMORIES_FS_SUPPORT == ENABLED)
   if(slot->type == SLOT_TYPE_FILE)
   {

      const FsDriver *driver = memory->driver;

      error = driver->close(slot->file);
      if(error)
         return CBOOT_ERROR_MEMORY_DRIVER_CLOSE_FAILED;
   }
#endif

   if(slot->type == SLOT_TYPE_DIRECT)
   {
      const FlashDriver *driver = memory->driver;
      error = driver->deInit();
      if(error)
         return CBOOT_ERROR_MEMORY_DRIVER_DEINIT_FAILED;
   }

   // Successful process
   return CBOOT_NO_ERROR;
}

cboot_error_t slotsInit(Memory *memory)
{
   uint_t i;
   const void *memoryDriver;
   Slot *slot;

   // Check parameters
   if(memory == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   if(memory->nbSlots == 0 || memory->nbSlots > NB_MAX_MEMORY_SLOTS)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   memoryDriver = memory->driver;

   // Loop through memory slots
   for(i = 0; i < memory->nbSlots; i++)
   {
      slot = &memory->slots[i];


      // Set memory parent for each slot
      slot->memParent = (void *)memory;

      // Is it a direct slot? (flash slot)

      if(slot->type == SLOT_TYPE_DIRECT)
      {
         // Check slot start address matches a sector address
         if(!(((const FlashDriver *)memoryDriver)->isSectorAddr(slot->addr)))
            return CBOOT_ERROR_INVALID_ADDRESS;
      }

      if(i < memory->nbSlots - 1)
      {
         // Check slots overlapping
         if(isSlotsOverlap(&memory->slots[0], &memory->slots[i + 1]))
            return CBOOT_ERROR_SLOTS_OVERLAP;
      }

#if (MEMORIES_FS_SUPPORT == ENABLED)
      // Is it a file slot? (file system slot)
      if(slot->type == SLOT_TYPE_FILE)
      {
         // Open slot
         slot->file = ((const FsDriver *)memoryDriver)->open(slot->path, 0);
         if(slot->file == NULL)
         {
            return CBOOT_ERROR_MEMORY_DRIVER_OPEN_FAILED;
         }
      }
#endif
   }

   // Successful process
   return CBOOT_NO_ERROR;
}

/**
 * @brief Search a slot in memory that match the given slot content type.
 * @param memory Pointer to memory in which we are looking for
 * @param slotCType Slot content type we are looking for
 * @param slot Pointer to the slot we are looking for
 * @return cboot_error_t
 **/
cboot_error_t memoryGetSlotByCType(Memory *memory, uint8_t slotCType, Slot **slot)
{
   uint_t i;

   if(memory == NULL || slotCType == 0 || slot == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   *slot = NULL;

   for(i = 0; i < memory->nbSlots; i++)
   {
      if(memory->slots[i].cType == slotCType)
      {
         *slot = &memory->slots[i];
         break;
      }
   }

   if(*slot == NULL)
   {
      return CBOOT_ERROR_FAILURE;
   }
   else
   {
      return CBOOT_NO_ERROR;
   }
}

cboot_error_t memoryGetMemoryByRole(Memory *memories, size_t nb_memories, MemoryRole role,
   Memory **memory)
{
   uint_t i;

   if(memories == NULL || nb_memories == 0 || memory == NULL)
      return CBOOT_ERROR_INVALID_PARAMETERS;

   *memory = NULL;

   for(i = 0; i < nb_memories; i++)
   {
      if(memories[i].memoryRole == role)
      {
         *memory = &memories[i];
         break;
      }
   }

   if(*memory == NULL)
   {
      return CBOOT_ERROR_FAILURE;
   }
   else
   {
      return CBOOT_NO_ERROR;
   }
}

/**
 * @brief Initialize memory write buffer
 **/

void memoryInitWriteBuffer(void)
{
   memset(memWriteBuffer, 0, sizeof(memWriteBuffer));
   memWriteBufferPos = memWriteBuffer;
   memWriteBufferLen = 0;
}

/**
 * @brief Reset memory write buffer
 **/

void memoryResetWriteBuffer(void) {
   memoryInitWriteBuffer();
}
