/**
 * @file sam_ed_7x_mcu_driver.c
 * @brief SAMD7x series MCU driver
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

// Dependencies
#include "sam_ed_7x_mcu_driver.h"

#include "sam.h"

#include <stdlib.h>

// SAM(E|D)7x MCU driver private function forward declaration
void mcuBootAppImageAsm(uint32_t sp, uint32_t rh);
void nvmLockFlash(void);

/**
 * @brief Return mcu vector table offset
 * @return VTOR offset value
 **/

uint32_t mcuGetVtorOffset(void) {
   return MCU_VTOR_OFFSET;
}

/**
 * @brief Reset MCU system
 **/

void mcuSystemReset(void) {
   NVIC_SystemReset();
}

/**
 * @brief Jump to the application at the given address.
 * @param[in] address Application start address
 **/

void mcuJumpToApplication(uint32_t address)
{
   uint32_t stackPointer;
   uint32_t programCounter;

   // Lock the Flash to disable the flash control register access
   nvmLockFlash();

   // Make sure the CPU is in privileged mode (refer to ARM application
   // note http://www.keil.com/support/docs/3913.htm)

   // Disable IRQ
   __disable_irq();

   // Disable all enabled interrupts in NVIC
   NVIC->ICER[0] = 0xFFFFFFFF;
   NVIC->ICER[1] = 0xFFFFFFFF;
   NVIC->ICER[2] = 0xFFFFFFFF;
   NVIC->ICER[3] = 0xFFFFFFFF;
   NVIC->ICER[4] = 0xFFFFFFFF;
   NVIC->ICER[5] = 0xFFFFFFFF;
   NVIC->ICER[6] = 0xFFFFFFFF;
   NVIC->ICER[7] = 0xFFFFFFFF;

   // Disable all enabled peripherals which might generate interrupt requests,
   // and clear all pending interrupt flags in those peripherals

   // Clear all pending interrupt requests in NVIC
   NVIC->ICPR[0] = 0xFFFFFFFF;
   NVIC->ICPR[1] = 0xFFFFFFFF;
   NVIC->ICPR[2] = 0xFFFFFFFF;
   NVIC->ICPR[3] = 0xFFFFFFFF;
   NVIC->ICPR[4] = 0xFFFFFFFF;
   NVIC->ICPR[5] = 0xFFFFFFFF;
   NVIC->ICPR[6] = 0xFFFFFFFF;
   NVIC->ICPR[7] = 0xFFFFFFFF;

   // Disable SysTick module
   SysTick->CTRL = 0;
   // Clear its exception pending bit
   SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;

   // Disable individual fault handlers
   SCB->SHCSR &=
      ~(SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk);

   // Activate the MSP, if the core is found to currently run with the PSP. As
   // the compiler might still uses the stack, the PSP needs to be copied to
   // the MSP before this
   if(__get_CONTROL() & CONTROL_SPSEL_Msk)
   {
      __set_MSP(__get_PSP());
      __set_CONTROL(__get_CONTROL() & ~CONTROL_SPSEL_Msk);
   }

   // Set stack pointer
   stackPointer = *((uint32_t *)address);
   // Set program counter
   programCounter = *((uint32_t *)(address + 4));

   // Barriars
   __DSB();
   __ISB();

   // Set vector table address
   SCB->VTOR = (uint32_t)address;

   // Barriars
   __DSB();
   __ISB();

   // -- Enable interrupts
   __enable_irq();

   // Boot to the application
   mcuBootAppImageAsm(stackPointer, programCounter);
}

/**
 * @brief Jump to the user application (helper function)
 * @param[in] sp Address of the stack pointer
 * @param[in] rh Address of the reset handler
 **/

#if defined(__GNUC__)
__attribute__((naked, noreturn)) void mcuBootAppImageAsm(uint32_t sp, uint32_t rh)
{
   __asm("MSR  MSP, r0");
   __asm("BX   r1");
}
#elif defined(__CC_ARM)
__asm __attribute__((noreturn)) void mcuBootAppImageAsm(uint32_t sp, uint32_t rh)
{
   MSR MSP, r0 BX r1
}
#elif defined(__IAR_SYSTEMS_ICC__)
void mcuBootAppImageAsm(uint32_t sp, uint32_t rh)
{
   __asm("MSR  MSP, r0");
   __asm("BX   r1");
}
#endif

/**
 * @brief Lock full flash main space memory
 **/

void nvmLockFlash(void)
{
#if 0
   size_t i;
   size_t regionSize;
   uint32_t regionAddr;

   //Get flash region size
   regionSize = (FLASH_SIZE / 32);
   //Set region address to the flash address (first region address)
   regionAddr = FLASH_ADDR;

   //Wait NVM controller to be ready
   while(!(NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_READY_Msk))
      ;

   //Loop through flash regions
   for(i = 0; i < regionSize; i++)
   {
      //Set address of the region to be locked
      NVMCTRL_REGS->NVMCTRL_ADDR = regionAddr;

      //Send lock region command
      NVMCTRL_REGS->NVMCTRL_CTRLB = NVMCTRL_CTRLB_CMD_UR | NVMCTRL_CTRLB_CMDEX_KEY;

      //Wait NVM controller to be ready
      while(!(NVMCTRL_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_READY_Msk))
         ;

      //Clear NVM DONE flag
      NVMCTRL_REGS->NVMCTRL_INTFLAG |= NVMCTRL_INTFLAG_DONE_Msk;

      //Update region address to the next region address
      regionAddr += regionSize;
   }
#endif
}
