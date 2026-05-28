/**
 * @file stm32h5xx_hal_conf.h
 * @brief Minimal HAL configuration for standalone bootloader
 *
 * Only enables the HAL modules needed by the bootloader:
 * RCC, FLASH, GPIO, PWR, CORTEX, ICACHE
 */

#ifndef STM32H5xx_HAL_CONF_H
#define STM32H5xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#define __FPU_PRESENT 1U
#define __FPU_USED    1U

/* Module Selection — minimal set for bootloader */
#define HAL_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_ICACHE_MODULE_ENABLED

/* Oscillator Values */
#if !defined(HSE_VALUE)
  #define HSE_VALUE    25000000U
#endif
#if !defined(HSE_STARTUP_TIMEOUT)
  #define HSE_STARTUP_TIMEOUT    100U
#endif
#if !defined(CSI_VALUE)
  #define CSI_VALUE    4000000UL
#endif
#if !defined(HSI_VALUE)
  #define HSI_VALUE    64000000UL
#endif
#if !defined(HSI48_VALUE)
  #define HSI48_VALUE  48000000UL
#endif
#if !defined(LSI_VALUE)
  #define LSI_VALUE    32000UL
#endif
#if !defined(LSE_VALUE)
  #define LSE_VALUE    32768UL
#endif
#if !defined(LSE_STARTUP_TIMEOUT)
  #define LSE_STARTUP_TIMEOUT    5000UL
#endif
#if !defined(EXTERNAL_CLOCK_VALUE)
  #define EXTERNAL_CLOCK_VALUE   12288000UL
#endif

/* System Configuration */
#define VDD_VALUE              3300UL
#define TICK_INT_PRIORITY      (15UL)
#define USE_RTOS               0U
#define PREFETCH_ENABLE        0U

/* No register callbacks needed in bootloader */
#define USE_HAL_ADC_REGISTER_CALLBACKS    0U
#define USE_HAL_TIM_REGISTER_CALLBACKS    0U
#define USE_HAL_UART_REGISTER_CALLBACKS   0U

/* Include enabled module headers */
#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32h5xx_hal_rcc.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32h5xx_hal_gpio.h"
#endif
#ifdef HAL_ICACHE_MODULE_ENABLED
  #include "stm32h5xx_hal_icache.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32h5xx_hal_cortex.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32h5xx_hal_flash.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32h5xx_hal_pwr.h"
#endif

/* Assert configuration */
#define assert_param(expr) ((void)0U)

#ifdef __cplusplus
}
#endif

#endif /* STM32H5xx_HAL_CONF_H */
