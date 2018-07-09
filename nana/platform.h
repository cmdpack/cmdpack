#pragma once

////////////////////////////////////////////////////////////////////////////////
//
// Nana graphics viewer
// Copyright (C) 1999,2010 Neill Corlett
//
////////////////////////////////////////////////////////////////////////////////
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__DJGPP__)

// Define standard integer types for djgpp
typedef   signed char       int8_t;
typedef   signed short      int16_t;
typedef   signed int        int32_t;
typedef   signed long long  int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

#elif defined(_MSC_VER)

// Define standard integer types for Visual Studio
typedef   signed __int8   int8_t;
typedef   signed __int16  int16_t;
typedef   signed __int32  int32_t;
typedef   signed __int64  int64_t;
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#else

// Assume C99 compatibility otherwise
#include <stdint.h>

#endif

////////////////////////////////////////////////////////////////////////////////
//
// Platform-specific functions
//
////////////////////////////////////////////////////////////////////////////////

const char* platform_name(void);

//
// Initialize/quit everything
//
int32_t platform_init(void);
void platform_quit(void);

//
// Start/end graphics
//
int32_t graphics_start(const char* name);
void graphics_end(void);

//
// Copy a horizontal line to the screen (clipped)
//
void graphics_copy_line(int32_t x, int32_t y, const uint8_t* src, int32_t w);

//
// Wait for a key, and return it
// Return value is the ASCII code or one of the following INPUT_KEY_s:
//
int32_t input_getkey(void);
enum {
    INPUT_KEY_ESCAPE = 0x100,
    INPUT_KEY_UP,
    INPUT_KEY_DOWN,
    INPUT_KEY_LEFT,
    INPUT_KEY_RIGHT,
    INPUT_KEY_PAGEUP,
    INPUT_KEY_PAGEDOWN,
    INPUT_KEY_HOME,
    INPUT_KEY_END,
    INPUT_KEY_SHIFT_UP,
    INPUT_KEY_SHIFT_DOWN,
    INPUT_KEY_SHIFT_LEFT,
    INPUT_KEY_SHIFT_RIGHT,
    INPUT_KEY_SHIFT_HOME,
    INPUT_KEY_SHIFT_END,
};

////////////////////////////////////////////////////////////////////////////////
//
// Open a view of a file
// Returns nonzero on error
//
int32_t fileview_open(const char* filename);
//
// Close the file view
//
void fileview_close(void);

//
// Get file length
//
extern uint32_t fileview_len;

//
// Get a window on the currently open file
// Returns NULL if it can't be retrieved for some reason
//
const uint8_t* fileview_get(uint32_t start, uint32_t len);

//
// The maximum number of bytes we can ask for at the end of a fileview_get
//
enum { FILEVIEW_MAX_SLOP = 65536 };

////////////////////////////////////////////////////////////////////////////////
