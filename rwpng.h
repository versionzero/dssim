/*---------------------------------------------------------------------------

   pngquant:  RGBA -> RGBA-palette quantization program             rwpng.h

  ---------------------------------------------------------------------------

      Copyright (c) 1998-2000 Greg Roelofs.  All rights reserved.

      This software is provided "as is," without warranty of any kind,
      express or implied.  In no event shall the author or contributors
      be held liable for any damages arising in any way from the use of
      this software.

      Permission is granted to anyone to use this software for any purpose,
      including commercial applications, and to alter it and redistribute
      it freely, subject to the following restrictions:

      1. Redistributions of source code must retain the above copyright
         notice, disclaimer, and this list of conditions.
      2. Redistributions in binary form must reproduce the above copyright
         notice, disclaimer, and this list of conditions in the documenta-
         tion and/or other materials provided with the distribution.
      3. All advertising materials mentioning features or use of this
         software must display the following acknowledgment:

            This product includes software developed by Greg Roelofs
            and contributors for the book, "PNG: The Definitive Guide,"
            published by O'Reilly and Associates.

  ---------------------------------------------------------------------------*/
#include "png.h"

#ifndef TRUE
#  define TRUE 1
#  define FALSE 0
#endif

#ifndef MAX
#  define MAX(a,b)  ((a) > (b)? (a) : (b))
#  define MIN(a,b)  ((a) < (b)? (a) : (b))
#endif

typedef enum {
    SUCCESS = 0,
    READ_ERROR = 2,
    TOO_MANY_COLORS = 5,
    NOT_OVERWRITING_ERROR = 15,
    CANT_WRITE_ERROR = 16,
    OUT_OF_MEMORY_ERROR = 17,
    PNG_OUT_OF_MEMORY_ERROR = 24,
    INIT_OUT_OF_MEMORY_ERROR = 34,
    INTERNAL_LOGIC_ERROR = 18,
    BAD_SIGNATURE_ERROR = 21,
    LIBPNG_FATAL_ERROR = 25,
    LIBPNG_INIT_ERROR = 35,
    LIBPNG_WRITE_ERROR = 55,
    LIBPNG_WRITE_WHOLE_ERROR = 45,
} pngquant_error;

typedef struct {
    png_uint_32 width;
    png_uint_32 height;
    png_uint_32 rowbytes;
    double gamma;
    int interlaced;
    unsigned char *rgba_data;
    unsigned char **row_pointers;
} read_info;

typedef struct {
    void *png_ptr;
    void *info_ptr;
    png_uint_32 width;
    png_uint_32 height;
    double gamma;
    int interlaced;
    int num_palette;
    int num_trans;
    png_color palette[256];
    unsigned char trans[256];
    unsigned char *indexed_data;
    unsigned char **row_pointers;
} write_info;

/* prototypes for public functions in rwpng.c */

void rwpng_version_info(void);

int rwpng_read_image(FILE *infile, read_info *mainprog_ptr);

int rwpng_write_image_init(FILE *outfile, write_info *mainprog_ptr);

int rwpng_write_image_whole(write_info *mainprog_ptr);

int rwpng_write_image_row(write_info *mainprog_ptr);

int rwpng_write_image_finish(write_info *mainprog_ptr);
