/**
 * @file cboot_debug.h
 * @brief Enhanced Debugging facilities
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

#ifndef _CBOOT_DEBUG_H
#define _CBOOT_DEBUG_H

// Dependencies
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "debug.h"

// ===== Safe char_t typedef =====
#ifndef _CHAR_T_DEFINED
typedef char char_t;
#define _CHAR_T_DEFINED
#endif

// ===== BOOT Trace Levels =====
#define CBOOT_TRACE_LEVEL_OFF      0
#define CBOOT_TRACE_LEVEL_FATAL    1
#define CBOOT_TRACE_LEVEL_ERROR    2
#define CBOOT_TRACE_LEVEL_WARN     3
#define CBOOT_TRACE_LEVEL_INFO     4
#define CBOOT_TRACE_LEVEL_DEBUG    5
#define CBOOT_TRACE_LEVEL_VERBOSE  6

#ifndef CBOOT_TRACE_LEVEL
#define CBOOT_TRACE_LEVEL CBOOT_TRACE_LEVEL_DEBUG
#endif

// ===== Color definitions =====
#define CLR_RESET   "\033[0m"
#define CLR_RED     "\033[31m"
#define CLR_YELLOW  "\033[33m"
#define CLR_GREEN   "\033[32m"
#define CLR_CYAN    "\033[36m"

// ===== Timestamp callback =====
typedef uint64_t (*log_get_time_func_t)(void);
static log_get_time_func_t boot_log_get_time_cb = NULL;

static inline void BOOT_LOG_SetTimestampCallback(log_get_time_func_t cb) {
   boot_log_get_time_cb = cb;
}

// ===== Filename-only macro =====
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// ===== Internal logging function =====
static inline void _BOOT_log_message(const char *color,
   const char *level,
   const char *file,
   int line,
   const char *fmt, ...)
{
   osSuspendAllTasks();

   va_list args;
   va_start(args, fmt);

   // Raw log (starts with newline)
   if(fmt[0] == '\n')
   {
      vfprintf(stderr, fmt, args);
      va_end(args);
      osResumeAllTasks();
      return;
   }

   // Timestamp string
   char ts_buf[32] = {0};
#if defined(__unix__) || defined(_WIN32)
    #include <time.h>
   time_t now = time(NULL);
   struct tm *tm_info = localtime(&now);
   strftime(ts_buf, sizeof(ts_buf), "%H:%M:%S", tm_info);
#else
   if(boot_log_get_time_cb)
   {
      uint64_t t = boot_log_get_time_cb();
      snprintf(ts_buf, sizeof(ts_buf), "%llu ms", (unsigned long long)t);
   }
   else
   {
      strcpy(ts_buf, "----");
   }
#endif

   fprintf(stderr, "%s[%s] %-5s %s:%d | ", color, ts_buf, level, file, line);
   vfprintf(stderr, fmt, args);
   fprintf(stderr, "%s\n", color[0] ? CLR_RESET : "");

   va_end(args);
   osResumeAllTasks();
}

// ===== BOOT Trace Macros =====
#if CBOOT_TRACE_LEVEL >= CBOOT_TRACE_LEVEL_FATAL
#define CBOOT_TRACE_FATAL(...) _BOOT_log_message(CLR_RED, "FATAL", __FILENAME__, __LINE__, __VA_ARGS__)
#else
#define CBOOT_TRACE_FATAL(...)
#endif

#if CBOOT_TRACE_LEVEL >= CBOOT_TRACE_LEVEL_ERROR
#define CBOOT_TRACE_ERROR(...) _BOOT_log_message(CLR_RED, "ERROR", __FILENAME__, __LINE__, __VA_ARGS__)
#else
#define CBOOT_TRACE_ERROR(...)
#endif

#if CBOOT_TRACE_LEVEL >= CBOOT_TRACE_LEVEL_WARN
#define CBOOT_TRACE_WARN(...) _BOOT_log_message(CLR_YELLOW, "WARN", __FILENAME__, __LINE__, __VA_ARGS__)
#else
#define CBOOT_TRACE_WARN(...)
#endif

#if CBOOT_TRACE_LEVEL >= CBOOT_TRACE_LEVEL_INFO
#define CBOOT_TRACE_INFO(...) _BOOT_log_message(CLR_GREEN, "INFO", __FILENAME__, __LINE__, __VA_ARGS__)
#else
#define CBOOT_TRACE_INFO(...)
#endif

#if CBOOT_TRACE_LEVEL >= CBOOT_TRACE_LEVEL_DEBUG
#define CBOOT_TRACE_DEBUG(...) _BOOT_log_message(CLR_CYAN, "DEBUG", __FILENAME__, __LINE__, __VA_ARGS__)
#else
#define CBOOT_TRACE_DEBUG(...)
#endif

#if CBOOT_TRACE_LEVEL >= CBOOT_TRACE_LEVEL_VERBOSE
#define CBOOT_TRACE_VERBOSE(...) _BOOT_log_message(CLR_CYAN, "VERBOSE", __FILENAME__, __LINE__, __VA_ARGS__)
#else
#define CBOOT_TRACE_VERBOSE(...)
#endif

// ===== Internal helper for array logging =====
static inline void _BOOT_trace_display_array(const char *color,
   const char *level,
   const char *file,
   int line,
   const char_t *prepend,
   const void *data,
   size_t length)
{
   const uint8_t *buf = (const uint8_t *)data;
   if(!prepend)
      prepend = "";

   // Print header
   _BOOT_log_message(color, level, file, line, "[ARRAY] %zu bytes:", length);

   // Print hex dump
   for(size_t i = 0; i < length; i++)
   {
      if(i % 16 == 0)
         fprintf(stderr, "%s", prepend);
      fprintf(stderr, "%02X ", buf[i]);
      if((i % 16) == 15 || i == length - 1)
         fprintf(stderr, "\n");
   }
}

// ===== CBOOT_TRACE_ARRAY Macro =====
#ifndef CBOOT_TRACE_ARRAY
#define CBOOT_TRACE_ARRAY(level_val, p, len, prepend) \
        do { \
           if(level_val) { \
              _BOOT_trace_display_array( \
   (level_val == CBOOT_TRACE_LEVEL_DEBUG)?CLR_CYAN: \
   (level_val == CBOOT_TRACE_LEVEL_INFO)?CLR_GREEN: \
   (level_val == CBOOT_TRACE_LEVEL_WARN)?CLR_YELLOW: \
   (level_val == CBOOT_TRACE_LEVEL_ERROR || level_val == CBOOT_TRACE_LEVEL_FATAL)?CLR_RED:"", \
   #level_val, __FILENAME__, __LINE__, prepend, p, len); \
           } \
        } while(0)
#endif

// ===== Debug init/display for PC/MCU =====
void bootDebugInit(uint32_t baudrate);
void bootDebugDisplayArray(FILE *stream, const char_t *prepend, const void *data, size_t length);

#endif // _CBOOT_DEBUG_H
