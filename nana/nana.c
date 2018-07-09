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

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif // win32

#define VERSIONSTR "1.3"

////////////////////////////////////////////////////////////////////////////////
//
// Platform-specific code
//
#include "platform.h"

//
// 8x8 font
//
#include "font.h"

////////////////////////////////////////////////////////////////////////////////

static const char hextable[16] =
{'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

////////////////////////////////////////////////////////////////////////////////

void sleepcp(int milliseconds) // cross-platform sleep function
{
    #ifdef WIN32
    Sleep(milliseconds);
    #else
    usleep(milliseconds * 1000);
    #endif // win32
}

////////////////////////////////////////////////////////////////////////////////
//
// Bit lookup table
//
static uint64_t bitplane_hi_table[256];

static void table_init() {
    uint32_t i;
    for(i = 0; i < 256; i++) {
        bitplane_hi_table[i] =
            (((uint64_t)(i & 0x80)) >>  7) |
            (((uint64_t)(i & 0x40)) <<  2) |
            (((uint64_t)(i & 0x20)) << 11) |
            (((uint64_t)(i & 0x10)) << 20) |
            (((uint64_t)(i & 0x08)) << 29) |
            (((uint64_t)(i & 0x04)) << 38) |
            (((uint64_t)(i & 0x02)) << 47) |
            (((uint64_t)(i & 0x01)) << 56);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Clear a horizontal line on the screen with the given color (clipped)
//
static void clear_horizontal_line(
    int32_t x,
    int32_t y,
    int32_t w,
    uint8_t c
) {
    uint8_t arr[64];
    memset(arr, c, sizeof(arr));
    while(w > 0) {
        int32_t tw = w;
        if(tw > sizeof(arr)) { tw = sizeof(arr); }
        graphics_copy_line(x, y, arr, tw);
        w -= tw;
        x += tw;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Copy an image to the screen (clipped)
//
static void copy_image(
          int32_t  x,
          int32_t  y,
    const uint8_t* src,
          int32_t  w,
          int32_t  h
) {
    while(h > 0) {
        graphics_copy_line(x, y, src, w);
        src += w;
        y++;
        h--;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Draw an 8x8 character to the screen
//
static void draw_char(
    int32_t x,
    int32_t y,
    uint8_t ch,
    uint8_t cf,
    uint8_t cb
) {
    //
    // Decode the character to a temp array
    //
    uint8_t* src = font8x8 + 8 * ch;
    uint8_t tmp[8 * 8];
    int i;
    for(i = 0; i < 8; i++) {
        ((uint64_t*)tmp)[i] =
            (0x0101010101010101ull * cb) ^
            (bitplane_hi_table[src[i]] * (cf ^ cb));
    }
    //
    // Copy it to the screen
    //
    copy_image(x, y, tmp, 8, 8);
}

////////////////////////////////////////////////////////////////////////////////
//
// Draw a string to the screen
//
static void draw_string(
    int32_t     x,
    int32_t     y,
    const char* s,
    uint8_t     cf,
    uint8_t     cb
) {
    while(*s) {
        draw_char(x, y, *s, cf, cb);
        x += 8;
        s++;
    }
}

////////////////////////////////////////////////////////////////////////////////

static void draw_addresses(
    uint32_t address,
    uint32_t inc,
    uint32_t unitheight
) {
    int32_t i;
    int32_t toggle = 1;
    for(i = 8; i < 192; i += unitheight) {
        uint8_t f = address >= fileview_len ? 24 : 27;
        uint8_t b = (toggle ^= 1) ? 112 : 184;
        int32_t end = i + unitheight;
        int32_t start = i;
        if(end > 192) { end = 192; }
        if(i == 8) { f = 92; }
        if(start <= (192 - 8)) {
            int32_t j;
            if(address >= 0x1000000) {
                for(j = 0; j < 7; j++) {
                    draw_char(
                        (7 * j) - 1,
                        i,
                        hextable[(address >> (4 * (6 - j))) & 0xF],
                        f,
                        b
                    );
                }
            } else {
                uint32_t a = address;
                for(j = 5; j >= 0; j--) {
                    draw_char(
                        8 * j,
                        i,
                        hextable[a & 0xF],
                        f,
                        b
                    );
                    a >>= 4;
                    if(!a) { f = 24; }
                }
            }
            start += 8;
        }
        while(start < end) clear_horizontal_line(0, start++, 48, b);
        address += inc;
    }
}

static void set_visible_area(
    uint32_t w,
    uint32_t h,
    uint32_t lineheight
) {
    uint32_t y;
    uint32_t colorchoice = 0;
    uint32_t colorcount = 0;
    for(y = 0; y < 184; y++) {
        uint32_t x = 48;
        if(y < h) x += w;
        clear_horizontal_line(x, y + 8, 320 - x, colorchoice ? 112 : 184);
        colorcount++;
        if(colorcount >= lineheight) {
            colorcount = 0;
            colorchoice ^= 1;
        }
    }
}

static void draw_ruler(void) {
    int32_t i;
    for(i = 0; i < 34; i++) {
        if(!(i & 3)) {
            draw_char(48 + 8 * i, 0, hextable[i & 0xF], 26,
                (i & 1) ? 184 : 112
            );
        } else {
            draw_char(48 + 8 * i, 0, '.', 24,
                (i & 1) ? 184 : 112
            );
        }
    }
}

static void draw_filename(const char *f) {
    draw_string(0, 192, "                ", 92, 112);
    if((strlen(f)) > 15) {
        draw_string(0, 192, "..", 25, 112);
        draw_string(16, 192, f + strlen(f) - 13, 92, 112);
    } else {
        draw_string(0, 192, f, 92, 112);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode 1bpp (highest bit first)
//
static void decoderaw_1bpp_hi(
    const uint8_t* src,
          uint8_t* dst,
          uint32_t width
) {
    uint8_t* dst_end = dst + (width & (~7));
    for(; dst < dst_end; dst += 8, src++) {
        *((uint64_t*)dst) = bitplane_hi_table[src[0]];
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode 2bpp (highest bit first)
//
static void decoderaw_2bpp_hi(
    const uint8_t* src,
          uint8_t* dst,
          uint32_t width
) {
    uint8_t* dst_end = dst + (width & (~3));
    for(; dst < dst_end; dst += 4, src++) {
        dst[0] = (src[0] >> 6) & 3;
        dst[1] = (src[0] >> 4) & 3;
        dst[2] = (src[0] >> 2) & 3;
        dst[3] = (src[0] >> 0) & 3;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode 2bpp (lowest bit first)
//
static void decoderaw_2bpp_lo(
    const uint8_t* src,
          uint8_t* dst,
          uint32_t width
) {
    uint8_t* dst_end = dst + (width & (~3));
    for(; dst < dst_end; dst += 4, src++) {
        dst[0] = (src[0] >> 0) & 3;
        dst[1] = (src[0] >> 2) & 3;
        dst[2] = (src[0] >> 4) & 3;
        dst[3] = (src[0] >> 6) & 3;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode 4bpp (highest bit first)
//
static void decoderaw_4bpp_hi(
    const uint8_t* src,
          uint8_t* dst,
          uint32_t width
) {
    uint8_t* dst_end = dst + (width & (~1));
    for(; dst < dst_end; dst += 2, src++) {
        dst[0] = (src[0] >> 4) & 15;
        dst[1] = (src[0] >> 0) & 15;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode 4bpp (lowest bit first)
//
static void decoderaw_4bpp_lo(
    const uint8_t* src,
          uint8_t* dst,
          uint32_t width
) {
    uint8_t* dst_end = dst + (width & (~1));
    for(; dst < dst_end; dst += 2, src++) {
        dst[0] = (src[0] >> 0) & 15;
        dst[1] = (src[0] >> 4) & 15;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode 8bpp
//
static void decoderaw_byte(
    const uint8_t* src,
          uint8_t* dst,
          uint32_t width
) {
    // No decoding required
    memcpy(dst, src, width);
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode an 8x8 1bpp tile
//
static void decodetile_8x8_1bpp(
    const uint8_t* src,
          uint8_t* dst
) {
    decoderaw_1bpp_hi(src, dst, 64);
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode an 8x8 2bpp tile, SNES format
// (bitplanes 0 and 1 interleaved)
//
static void decodetile_8x8_2bpp_snes(
    const uint8_t* src,
          uint8_t* dst
) {
    uint8_t* dst_end = dst + 8 * 8;
    for(; dst < dst_end; dst += 8, src += 2) {
        *((uint64_t*)dst) =
            (bitplane_hi_table[src[0]] << 0) |
            (bitplane_hi_table[src[1]] << 1);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode an 8x8 2bpp tile, NES format
// (bitplane 0 followed by bitplane 1)
//
static void decodetile_8x8_2bpp_nes(
    const uint8_t* src,
          uint8_t* dst
) {
    uint8_t* dst_end = dst + 8 * 8;
    for(; dst < dst_end; dst += 8, src += 1) {
        *((uint64_t*)dst) =
            (bitplane_hi_table[src[0]] << 0) |
            (bitplane_hi_table[src[8]] << 1);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode an 8x8 4bpp tile, SNES format
// (bitplanes 0 and 1 interleaved, followed by bitplanes 2 and 3 interleaved)
//
static void decodetile_8x8_4bpp_snes(
    const uint8_t* src,
          uint8_t* dst
) {
    uint8_t* dst_end = dst + 8 * 8;
    for(; dst < dst_end; dst += 8, src += 2) {
        *((uint64_t*)dst) =
            (bitplane_hi_table[src[0x00]] << 0) |
            (bitplane_hi_table[src[0x01]] << 1) |
            (bitplane_hi_table[src[0x10]] << 2) |
            (bitplane_hi_table[src[0x11]] << 3);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode an 8x8 8bpp tile, SNES format
// (bitplanes 0 and 1 interleaved, then 2+3, then 4+5, then 6+7)
//
static void decodetile_8x8_8bpp_snes(
    const uint8_t* src,
          uint8_t* dst
) {
    uint8_t* dst_end = dst + 8 * 8;
    for(; dst < dst_end; dst += 8, src += 2) {
        *((uint64_t*)dst) =
            (bitplane_hi_table[src[0x00]] << 0) |
            (bitplane_hi_table[src[0x01]] << 1) |
            (bitplane_hi_table[src[0x10]] << 2) |
            (bitplane_hi_table[src[0x11]] << 3) |
            (bitplane_hi_table[src[0x20]] << 4) |
            (bitplane_hi_table[src[0x21]] << 5) |
            (bitplane_hi_table[src[0x30]] << 6) |
            (bitplane_hi_table[src[0x31]] << 7);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode an 8x8 4bpp tile, Genesis format
//
static void decodetile_8x8_4bpp_gen(
    const uint8_t* src,
          uint8_t* dst
) {
    //
    // This is just row-major 4bpp, highest bit first
    //
    decoderaw_4bpp_hi(src, dst, 64);
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode an 8x8 8bpp tile (bytes)
//
static void decodetile_8x8_byte(
    const uint8_t* src,
          uint8_t* dst
) {
    //
    // Just copy the bytes
    //
    decoderaw_byte(src, dst, 64);
}

////////////////////////////////////////////////////////////////////////////////

enum { MAX_TILE_WIDTH  = 8 };
enum { MAX_TILE_HEIGHT = 8 };

////////////////////////////////////////////////////////////////////////////////

struct TILE_FORMAT {
    int32_t id;
    const char *title;
    void (*decode)(const uint8_t*, uint8_t*);
    uint32_t bytespertile;
    uint32_t maxcolors;
    uint32_t tilewidth;
    uint32_t tileheight;
    uint32_t width;
};

static struct TILE_FORMAT tile_format_table[] = {
    {1, "[8x8:1bpp hi]"     , decodetile_8x8_1bpp     ,  8,   2, 8, 8, 256},
    {2, "[8x8:2bpp SNES]"   , decodetile_8x8_2bpp_snes, 16,   4, 8, 8, 128},
    {3, "[8x8:2bpp NES]"    , decodetile_8x8_2bpp_nes , 16,   4, 8, 8, 128},
    {4, "[8x8:4bpp SNES]"   , decodetile_8x8_4bpp_snes, 32,  16, 8, 8, 128},
    {5, "[8x8:4bpp Genesis]", decodetile_8x8_4bpp_gen , 32,  16, 8, 8, 256},
    {6, "[8x8:byte]"        , decodetile_8x8_byte     , 64, 256, 8, 8, 256},
    {7, "[8x8:8bpp SNES]"   , decodetile_8x8_8bpp_snes, 64, 256, 8, 8, 128},
    {0}
};

static int32_t tile_format = 1;

////////////////////////////////////////////////////////////////////////////////

struct RAW_FORMAT {
    int32_t id;
    const char *title;
    void (*decode)(const uint8_t*, uint8_t*, uint32_t);
    uint32_t pixelsperbyte;
    uint32_t maxcolors;
    uint32_t width;
};

static struct RAW_FORMAT raw_format_table[] = {
    {1, "[raw:1bpp hi]" , decoderaw_1bpp_hi  ,  8,   2,       256},
    {2, "[raw:2bpp hi]" , decoderaw_2bpp_hi  ,  4,   4,       256},
    {3, "[raw:2bpp lo]" , decoderaw_2bpp_lo  ,  4,   4,       256},
    {4, "[raw:4bpp hi]" , decoderaw_4bpp_hi  ,  2,  16,       256},
    {5, "[raw:4bpp lo]" , decoderaw_4bpp_lo  ,  2,  16,       256},
    {6, "[raw:byte]"    , decoderaw_byte     ,  1, 256,       256},
    {0}
};

static int32_t raw_format = 5;

enum { MAX_VISIBLE_WIDTH = 272 };

////////////////////////////////////////////////////////////////////////////////

static int32_t tile_mode = 0;

////////////////////////////////////////////////////////////////////////////////

static const uint8_t empty_tile[8*8] = {0};

static void draw_tile_screen(
    uint32_t address,
    const struct TILE_FORMAT *fmt
) {
    uint32_t bytesperrow = (fmt->width / fmt->tilewidth) * fmt->bytespertile;

    uint32_t tx, ty;
    uint8_t buf[MAX_TILE_WIDTH * MAX_TILE_HEIGHT];

    uint32_t w = fmt->width;
    if(w > MAX_VISIBLE_WIDTH) { w = MAX_VISIBLE_WIDTH; }

    for(ty = 0; ty < 23 * 8; ty += fmt->tileheight) {
        const uint8_t* data = NULL;

        if(address < fileview_len) {
            data = fileview_get(address, bytesperrow);
        }

        for(tx = 0; tx < w; tx += fmt->tilewidth) {
            if(data) {
                fmt->decode(data, buf);
                copy_image(tx + 48, ty + 8, buf, fmt->tilewidth, fmt->tileheight);
                data += fmt->bytespertile;
            } else {
                copy_image(tx + 48, ty + 8, empty_tile, fmt->tilewidth, fmt->tileheight);
            }
        }

        address += bytesperrow;
    }
}

////////////////////////////////////////////////////////////////////////////////

static void draw_raw_screen(
    uint32_t address,
    const struct RAW_FORMAT *fmt
) {
    uint32_t bytesperline = fmt->width / fmt->pixelsperbyte;
    uint32_t y;
    uint8_t buf[MAX_VISIBLE_WIDTH];
    uint32_t w = fmt->width;
    if(w > MAX_VISIBLE_WIDTH) { w = MAX_VISIBLE_WIDTH; }

    for(y = 0; y < 184; y++) {
        const uint8_t* data = NULL;

        if(address < fileview_len) {
            data = fileview_get(address, bytesperline);
        }

        if(data) {
            fmt->decode(data, buf, w);
            graphics_copy_line(48, y + 8, buf, w);
        } else {
            clear_horizontal_line(48, y + 8, w, 0);
        }

        address += bytesperline;
    }
}

static void draw_format(void) {
    char s[32];
    const char *t;
    size_t l;
    uint32_t w;

    if(tile_mode) {
        t = tile_format_table[tile_format].title;
        w = tile_format_table[tile_format].width;
    } else {
        t = raw_format_table [raw_format ].title;
        w = raw_format_table [raw_format ].width;
    }
    memset(s, 0x20, sizeof(s));
    strcpy(s, t);
    l = strlen(t);
    s[l] = 0x20;
    if(w > 99999) { w = 99999; }
    sprintf(s + 18, "%5dw", w);

    draw_string(128, 192, s, 28, 112);
}

static void draw_file_addresses(
    uint32_t address
) {
    uint32_t inc, unitheight;
    if(tile_mode) {
        struct TILE_FORMAT *fmt =
            &tile_format_table[tile_format];
        inc        = (fmt->width / fmt->tilewidth) * fmt->bytespertile;
        unitheight = fmt->tileheight;
    } else {
        struct RAW_FORMAT *fmt =
            &raw_format_table[raw_format];
        inc        = (fmt->width / fmt->pixelsperbyte) * 8;
        unitheight = 8;
    }
    draw_addresses(address, inc, unitheight);
}

static void draw_file_contents(
    uint32_t address
) {
    if(tile_mode) {
        draw_tile_screen(address, &tile_format_table[tile_format]);
    } else {
        draw_raw_screen (address, &raw_format_table [raw_format ]);
    }
}

static void draw_file(
    uint32_t address
) {
    draw_file_addresses(address);
    draw_file_contents (address);
}

static void run_viewer(void) {
    uint32_t address = 0;
    uint32_t bpl = 0;
    uint32_t bpu = 0;
    uint32_t unitheight = 0;
    uint32_t lppage = 0;
    uint32_t *width = NULL;
    uint32_t widthgran = 0;
    uint32_t i, k;
    int reformat = 1;
    uint32_t max_stride_width = 65536;
    for(;;) {
        sleepcp(100);
        if(reformat) {
            if(tile_mode) {
                struct TILE_FORMAT *fmt = &tile_format_table[tile_format];
                width      = &(fmt->width);
                widthgran  = fmt->tilewidth;
                unitheight = fmt->tileheight;
                bpu        = fmt->bytespertile;
                bpl        = bpu * ((*width) / widthgran);
                lppage     = (23 * 8) / fmt->tileheight;

                max_stride_width = (FILEVIEW_MAX_SLOP / fmt->bytespertile) * fmt->tilewidth;

            } else {
                struct RAW_FORMAT *fmt = &raw_format_table[raw_format];
                width      = &(fmt->width);
                widthgran  = fmt->pixelsperbyte;
                unitheight = 8;
                bpu        = 1;
                bpl        = (*width) / widthgran;
                lppage     = 23 * 8;

                max_stride_width = FILEVIEW_MAX_SLOP * fmt->pixelsperbyte;

            }

            if(max_stride_width > 65536) { max_stride_width = 65536; }

            draw_format();
            set_visible_area(*width, 184, unitheight);
            reformat = 0;
        }
        draw_file(address);

        k = input_getkey();

        switch(k) {
        case INPUT_KEY_ESCAPE:
            return;

        case INPUT_KEY_SHIFT_UP:
            if(address >= bpl) address -= bpl;
            break;

        case INPUT_KEY_UP:
            {   uint32_t amt = tile_mode ? bpl : 8 * bpl;
                if(address >= amt) address -= amt;
            }
            break;

        case INPUT_KEY_SHIFT_DOWN:
            if((address + bpl) < fileview_len) address += bpl;
            break;

        case INPUT_KEY_DOWN:
            {   uint32_t amt = tile_mode ? bpl : 8 * bpl;
                if((address + amt) < fileview_len) address += amt;
            }
            break;

        case INPUT_KEY_SHIFT_LEFT:
            if(address >= bpu) address -= bpu;
            break;

        case INPUT_KEY_LEFT:
            {   uint32_t amt = tile_mode ? bpu : 8 / widthgran;
                if(address >= amt) address -= amt;
            }
            break;

        case INPUT_KEY_SHIFT_RIGHT:
            if((address + bpu) < fileview_len) address += bpu;
            break;

        case INPUT_KEY_RIGHT:
            {   uint32_t amt = tile_mode ? bpu : 8 / widthgran;
                if((address + amt) < fileview_len) address += amt;
            }
            break;

        case INPUT_KEY_PAGEUP:
            for(i = 0; i < lppage; i++) {
                if(address >= bpl) address -= bpl;
            }
            break;

        case INPUT_KEY_PAGEDOWN:
            for(i = 0; i < lppage; i++) {
                if((address + bpl) < fileview_len) address += bpl;
            }
            break;

        case INPUT_KEY_HOME:
            address %= bpl;
            break;

        case INPUT_KEY_END:
            while((address + bpl) < fileview_len) address += bpl;
            for(i = 1; i < lppage; i++) {
                if(address >= bpl) address -= bpl;
            }
            break;

        case INPUT_KEY_SHIFT_HOME:
            address = 0;
            break;

        case INPUT_KEY_SHIFT_END:
            address = fileview_len ? fileview_len - 1 : 0;
            break;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {   int32_t id = k - '0';
            int32_t n = 0;
            if(tile_mode) {
                while(
                    (tile_format_table[n].id) &&
                    (tile_format_table[n].id != id)
                ) n++;
                if(tile_format_table[n].id) {
                    tile_format = n;
                    reformat = 1;
                }
            } else {
                while(
                    (raw_format_table[n].id) &&
                    (raw_format_table[n].id != id)
                ) n++;
                if(raw_format_table[n].id) {
                    raw_format = n;
                    reformat = 1;
                }
            }
            break;
        }

        case 't': case 'T':
            tile_mode = !tile_mode;
            reformat = 1;
            break;

        case '+':
            if(address < fileview_len) address++;
            break;

        case '-':
            if(address) address--;
            break;

        case '<':
            if((*width) > 8) {
                (*width) -= 8;
                reformat = 1;
            }
            break;

        case '>':
            if((*width) <= (max_stride_width - 8)) {
                (*width) += 8;
                reformat = 1;
            }
            break;

        case ',':
            if((*width) > widthgran) {
                (*width) -= widthgran;
                reformat = 1;
            }
            break;

        case '.':
            if((*width) < max_stride_width) {
                (*width) += widthgran;
                reformat = 1;
            }
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

static void banner(void) {
    fprintf(stderr,
        "Nana v" VERSIONSTR" by Neill Corlett (%s build)\n"
        "http://www.neillcorlett.com/\n",
        platform_name()
    );
}

int main(int argc, char **argv) {
    if(argc != 2) {
        banner();
        fprintf(stderr, "\nusage: %s filename\n", argv[0]);
        return 1;
    }
    table_init();

    if(platform_init()) { return 1; }
    if(fileview_open(argv[1])) { return 1; }
    if(graphics_start(argv[1])) { return 1; }

    draw_filename(argv[1]);
    draw_string(0, 0, "\x1\x1" VERSIONSTR " ", 184, 112);
    draw_ruler();

    run_viewer();

    graphics_end();
    fileview_close();
    platform_quit();

    fprintf(stderr, "\n");
    banner();
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
