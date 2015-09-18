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
// File viewer using regular fopen, etc.
//

static const uint32_t FILE_BLOCK_SIZE = 0x10000;

static FILE*    file_ptr;

       uint32_t fileview_len;

static uint32_t file_blocks;
static uint8_t* file_contents;
static uint8_t* file_block_loaded;

/*
typedef   signed __int8   int8_t;
typedef   signed __int16  int16_t;
typedef   signed __int32  int32_t;
typedef   signed __int64  int64_t;
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
*/

//
// Open a view of a file
// Returns nonzero on error
//
int fileview_open(
    const char* filename
) {
    uint32_t slop_blocks;
    uint32_t nblocks_clear;

    file_ptr = fopen(filename, "rb");
    if(!file_ptr) {
        perror(filename);
        return 1;
    }
    fseek(file_ptr, 0, SEEK_END);
    fileview_len = ftell(file_ptr);
    //
    // Avoid overflow (a malloc of this size will likely fail on DOS anyway)
    //
    if(fileview_len > 0xFFF00000) {
        fileview_len = 0xFFF00000;
    }

    //
    // Calculate the total number of blocks we need
    //
    slop_blocks = (FILEVIEW_MAX_SLOP + (FILE_BLOCK_SIZE - 1)) / FILE_BLOCK_SIZE;
    file_blocks = ((fileview_len + (FILE_BLOCK_SIZE - 1)) / FILE_BLOCK_SIZE) + slop_blocks;

    //
    // Create loaded map
    //
    file_block_loaded = malloc((file_blocks + 7) / 8);
    if(!file_block_loaded) {
        fclose(file_ptr);
        fprintf(stderr, "Out of memory\n");
        return 1;
    }
    memset(file_block_loaded, 0, (file_blocks + 7) / 8);

    //
    // Allocate space for actual file contents
    //
    file_contents = malloc(file_blocks * FILE_BLOCK_SIZE);
    if(!file_contents) {
        fclose(file_ptr);
        fprintf(stderr, "Out of memory\n");
        return 1;
    }

    //
    // Clear the last block + slop blocks
    //
    nblocks_clear = slop_blocks + 1;
    if(nblocks_clear > file_blocks) {
        nblocks_clear = file_blocks;
    }
    memset(
        file_contents + FILE_BLOCK_SIZE * (file_blocks - nblocks_clear),
        0,
        FILE_BLOCK_SIZE * nblocks_clear
    );

    return 0;
}

//
// Close the file view
//
void fileview_close(void) {
    fclose(file_ptr);
    free(file_block_loaded);
    free(file_contents);
}

static void file_syncblock(uint32_t block) {
    if(block >= file_blocks) return;
    if((file_block_loaded[block >> 3] >> (block & 7)) & 1) return;

    //
    // TODO: errors are ignored here
    //
    fseek(file_ptr, block * FILE_BLOCK_SIZE, SEEK_SET);
    fread(file_contents + FILE_BLOCK_SIZE * block, 1, FILE_BLOCK_SIZE, file_ptr);

    file_block_loaded[block >> 3] |= 1 << (block & 7);
}

//
// Get a window on the currently open file
// Returns NULL if it can't be retrieved for some reason
//
const uint8_t* fileview_get(uint32_t start, uint32_t len) {
    uint32_t a, b;

    if(start > fileview_len) { return NULL; }
    if(len <= 0) { return NULL; }
    if(len > fileview_len) { len = fileview_len; }

    a = start;
    b = start + len;
    if(a >= b) { return NULL; }

    a = (a                        ) / FILE_BLOCK_SIZE;
    b = (b + (FILE_BLOCK_SIZE - 1)) / FILE_BLOCK_SIZE;
    while(a < b) { file_syncblock(a++); }

    return file_contents + start;
}

////////////////////////////////////////////////////////////////////////////////
