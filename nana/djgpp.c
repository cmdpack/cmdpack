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

#include "platform.h"

//
// Graphics and input code for DOS/DJGPP
//

#include <sys/farptr.h>
#include <bios.h>
#include <dpmi.h>
#include <go32.h>
#include <crt0.h>
int _crt0_startup_flags=
    _CRT0_FLAG_PRESERVE_UPPER_CASE |
    _CRT0_FLAG_USE_DOS_SLASHES |
    _CRT0_FLAG_DISALLOW_RESPONSE_FILES |
    _CRT0_FLAG_NEARPTR |
    _CRT0_FLAG_NULLOK |
    _CRT0_FLAG_NONMOVE_SBRK |
    _CRT0_FLAG_LOCK_MEMORY;

////////////////////////////////////////////////////////////////////////////////

const char* platform_name(void) {
    return "DOS/DJGPP";
}

int32_t platform_init(void) {
    return 0; // ok
}

void platform_quit(void) {
    // no-op
}

////////////////////////////////////////////////////////////////////////////////
//
// Graphics:
//

// INT 10h function 0
static void bios_setvideomode(uint8_t mode) {
    __dpmi_regs r;
    r.x.ax = mode & 0xFF;
    __dpmi_int(0x10, &r);
}

int graphics_start(const char* name) {
    bios_setvideomode(0x13);
    return 0;
}

void graphics_end(void) {
    bios_setvideomode(0x03);
}

void graphics_copy_line(int32_t x, int32_t y, const uint8_t* src, int32_t w) {
    //
    // Clip
    //
    if(x >= 320) { return; }
    if(y < 0 || y >= 200) { return; }
    if(w <= 0) { return; }
    if(w > 320) { w = 320; }
    if(x < 0) {
        w += x;
        src -= x;
        x = 0;
        if(w <= 0) { return; }
    }
    if((x + w) > 320) {
        w = 320 - x;
    }

    _farsetsel(_dos_ds);
    uint32_t dest = 0xA0000 + 320 * y + x;

    //
    // Copy initial bytes
    //
    while(w && (dest & 3)) {
        _farnspokeb(dest, (*src));
        dest++;
        src++;
        w--;
    }

    //
    // Copy 4 bytes at a time
    //
    uint32_t dest_end = dest + (w & (~3));
    while(dest < dest_end) {
        _farnspokel(dest, (*((uint32_t*)src)));
        dest += 4;
        src += 4;
    }
    w &= 3;

    //
    // Copy final bytes
    //
    while(w) {
        _farnspokeb(dest, (*src));
        dest++;
        src++;
        w--;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Input:
//
int32_t input_getkey(void) {
    uint16_t k = bioskey(0);
    uint16_t m = bioskey(2);

    //graphics_end(); fprintf(stderr, "k=0x%04X m=0x%04X\n", k, m); exit(1);

    switch(k >> 8) {
    case  1: return INPUT_KEY_ESCAPE;
    case 72: return (m & 3) ? INPUT_KEY_SHIFT_UP    : INPUT_KEY_UP;
    case 80: return (m & 3) ? INPUT_KEY_SHIFT_DOWN  : INPUT_KEY_DOWN;
    case 75: return (m & 3) ? INPUT_KEY_SHIFT_LEFT  : INPUT_KEY_LEFT;
    case 77: return (m & 3) ? INPUT_KEY_SHIFT_RIGHT : INPUT_KEY_RIGHT;
    case 73: return INPUT_KEY_PAGEUP;
    case 81: return INPUT_KEY_PAGEDOWN;
    case 71: return (m & 3) ? INPUT_KEY_SHIFT_HOME  : INPUT_KEY_HOME;
    case 79: return (m & 3) ? INPUT_KEY_SHIFT_END   : INPUT_KEY_END;
    }

    return k & 0xFF; // presumably, an ASCII code
}

////////////////////////////////////////////////////////////////////////////////
