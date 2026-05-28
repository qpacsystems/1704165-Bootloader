/**
  * @file      boot.gcc.s
  * @brief     Minimal startup for STM32H563VI bootloader (GCC)
  *
  * Trimmed vector table — only core exceptions, no peripheral IRQs.
  * The bootloader is bare-metal with no interrupts except SysTick.
  *
  * SPDX-License-Identifier: GPL-2.0-or-later
  */

  .syntax unified
  .cpu cortex-m33
  .fpu softvfp
  .thumb

.global g_pfnVectors
.global Default_Handler

.word _sidata
.word _sdata
.word _edata
.word _sbss
.word _ebss

  .section  .text.Reset_Handler
  .weak  Reset_Handler
  .type  Reset_Handler, %function
Reset_Handler:
  ldr   r0, =_estack
  mov   sp, r0

  bl  SystemInit

  /* Copy .data from flash to RAM */
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
  movs r3, #0
  b LoopCopyDataInit

CopyDataInit:
  ldr r4, [r2, r3]
  str r4, [r0, r3]
  adds r3, r3, #4

LoopCopyDataInit:
  adds r4, r0, r3
  cmp r4, r1
  bcc CopyDataInit

  /* Zero fill .bss */
  ldr r2, =_sbss
  ldr r4, =_ebss
  movs r3, #0
  b LoopFillZerobss

FillZerobss:
  str  r3, [r2]
  adds r2, r2, #4

LoopFillZerobss:
  cmp r2, r4
  bcc FillZerobss

  bl __libc_init_array
  bl main

LoopForever:
  b LoopForever

  .size Reset_Handler, .-Reset_Handler

  .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b Infinite_Loop
  .size Default_Handler, .-Default_Handler

/******************************************************************************
 * Minimal vector table — core exceptions only
 ******************************************************************************/
  .section .isr_vector,"a",%progbits
  .type g_pfnVectors, %object
  .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
  .word  _estack
  .word  Reset_Handler
  .word  NMI_Handler
  .word  HardFault_Handler
  .word  MemManage_Handler
  .word  BusFault_Handler
  .word  UsageFault_Handler
  .word  SecureFault_Handler
  .word  0
  .word  0
  .word  0
  .word  SVC_Handler
  .word  DebugMon_Handler
  .word  0
  .word  PendSV_Handler
  .word  SysTick_Handler
  /* IRQ 0-5: only FLASH needed for flash programming */
  .word  0                    /* WWDG */
  .word  0                    /* PVD_AVD */
  .word  0                    /* RTC */
  .word  0                    /* RTC_S */
  .word  0                    /* TAMP */
  .word  0                    /* RAMCFG */
  .word  FLASH_IRQHandler     /* FLASH */
  .word  0                    /* FLASH_S */

  /* Weak aliases */
  .weak  NMI_Handler
  .thumb_set NMI_Handler, Default_Handler
  .weak  HardFault_Handler
  .thumb_set HardFault_Handler, Default_Handler
  .weak  MemManage_Handler
  .thumb_set MemManage_Handler, Default_Handler
  .weak  BusFault_Handler
  .thumb_set BusFault_Handler, Default_Handler
  .weak  UsageFault_Handler
  .thumb_set UsageFault_Handler, Default_Handler
  .weak  SecureFault_Handler
  .thumb_set SecureFault_Handler, Default_Handler
  .weak  SVC_Handler
  .thumb_set SVC_Handler, Default_Handler
  .weak  DebugMon_Handler
  .thumb_set DebugMon_Handler, Default_Handler
  .weak  PendSV_Handler
  .thumb_set PendSV_Handler, Default_Handler
  .weak  SysTick_Handler
  .thumb_set SysTick_Handler, Default_Handler
  .weak  FLASH_IRQHandler
  .thumb_set FLASH_IRQHandler, Default_Handler
