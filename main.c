
/*
 Copyright (c) 2011 porneL. All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are
 permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of
    conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list
    of conditions and the following disclaimer in the documentation and/or other materials
    provided with the distribution.

 THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdlib.h>
#include <math.h>
#include "rwpng.h"

typedef struct {
    unsigned char r,g,b,a;
} rgba8;

typedef struct {
    float r,g,b,a;
} rgbaf;

typedef struct {
    float l,A,b,a;
} laba;

/* Converts 0..255 pixel to internal 0..1 with premultiplied alpha */
inline static rgbaf rgba8_to_f(const float gamma, rgba8 px)
{
    float r = powf((px.r)/255.0f, 1.0f/gamma),
          g = powf((px.g)/255.0f, 1.0f/gamma),
          b = powf((px.b)/256.0f, 1.0f/gamma),
          a = (px.a)/255.0f;

    return (rgbaf){r*a,g*a,b*a,a};
}

/* Converts premultiplied alpha 0..1 to 0..255 */
inline static rgba8 rgbaf_to_8(const float gamma, rgbaf px)
{
    if (px.a < 1.0/256.0f) {
        return (rgba8){0,0,0,0};
    }

    float r,g,b,a;
    r = powf(px.r/px.a, gamma)*256.0f; // *256, because it's rounded down
    g = powf(px.g/px.a, gamma)*256.0f;
    b = powf(px.b/px.a, gamma)*256.0f;
    a = px.a*256.0f;

    return (rgba8){
        r>=255 ? 255 : (r<=0 ? 0 : r),
        g>=255 ? 255 : (g<=0 ? 0 : g),
        b>=255 ? 255 : (b<=0 ? 0 : b),
        a>=255 ? 255 : a,
    };
}


static const float D65x = 0.9505f, D65y = 1.0f, D65z = 1.089f;

inline static laba rgba_to_laba(const double gamma, const rgba8 px)
{
    float r = powf(px.r/255.0f, 1.0f/gamma),
          g = powf(px.g/255.0f, 1.0f/gamma),
          b = powf(px.b/255.0f, 1.0f/gamma),
          a = px.a/255.0f;

    float fx = (r*0.4124f + g*0.3576f + b*0.1805f)/D65x;
    float fy = (r*0.2126f + g*0.7152f + b*0.0722f)/D65y;
    float fz = (r*0.0193f + g*0.1192f + b*0.9505f)/D65z;
    const float epsilon = 216.0/24389.0;
    fx = ((fx > epsilon)? powf(fx, (1.0f/3.0f)) : (7.787f*fx + 16.0f/116.0f));
    fy = ((fy > epsilon)? powf(fy, (1.0f/3.0f)) : (7.787f*fy + 16.0f/116.0f));
    fz = ((fz > epsilon)? powf(fz, (1.0f/3.0f)) : (7.787f*fz + 16.0f/116.0f));
    return (laba){
        (116.0f * fy - 16.0f)/100.0*a,
        (86.2f + 500.0f * (fx - fy))/220.0*a, /* 86 is a fudge to make the value positive */
        (107.9f + 200.0f * (fy - fz))/220.0*a, /* 107 is a fudge to make the value positive */
        a};
}

/* Macros to avoid repeating every line 4 times */

#define LABA_OP(dst,X,op,Y) dst = (laba){ \
    (X).l op (Y).l, \
    (X).A op (Y).A, \
    (X).b op (Y).b, \
    (X).a op (Y).a} \

#define LABA_OPC(dst,X,op,Y) dst = (laba){ \
    (X).l op (Y), \
    (X).A op (Y), \
    (X).b op (Y), \
    (X).a op (Y)} \

#define LABA_OP1(dst,op,Y) dst = (laba) {\
    dst.l op (Y).l, \
    dst.A op (Y).A, \
    dst.b op (Y).b, \
    dst.a op (Y).a} \


typedef void rowcallback(laba *, int width);

static void square_row(laba *row, int width)
{
    for(int i=0; i < width; i++) {
        LABA_OP(row[i],row[i],*,row[i]);
    }
}

/*
 Blurs image horizontally (width 2*size+1) and writes it transposed to dst (called twice gives 2d blur)
 Callback is executed on every row before blurring
*/
static void transposing_1d_blur(laba *restrict src, laba *restrict dst, int width, int height, const int size, rowcallback *const callback)
{
    const double sizef = size;

    for(int j=0; j < height; j++) {
        laba *restrict row = src + j*width;

        // preprocess line
        if (callback) callback(row,width);

        // accumulate sum for pixels outside line
        laba sum;
        LABA_OPC(sum,row[0],*,sizef);
        for(int i=0; i < size; i++) {
            LABA_OP1(sum,+=,row[i]);
        }

        // blur with left side outside line
        for(int i=0; i < size; i++) {
            LABA_OP1(sum,-=,row[0]);
            LABA_OP1(sum,+=,row[i+size]);

            LABA_OPC(dst[i*height + j],sum,/,sizef*2.0);
        }

        for(int i=size; i < width-size; i++) {
            LABA_OP1(sum,-=,row[i-size]);
            LABA_OP1(sum,+=,row[i+size]);

            LABA_OPC(dst[i*height + j],sum,/,sizef*2.0);
        }

        // blur with right side outside line
        for(int i=width-size; i < width; i++) {
            LABA_OP1(sum,-=,row[i-size]);
            LABA_OP1(sum,+=,row[width-1]);

            LABA_OPC(dst[i*height + j],sum,/,sizef*2.0);
        }
    }
}

/*
 Filters image with callback and blurs (lousy approximate of gaussian) it proportionally to
*/
static void blur(laba *restrict src, laba *restrict tmp, laba *restrict dst, int width, int height, rowcallback *const callback)
{
    int small=1, big=1;
    if (MIN(height,width) > 100) big++;
    if (MIN(height,width) > 200) big++;
    if (MIN(height,width) > 500) small++;
    if (MIN(height,width) > 800) big++;

    transposing_1d_blur(src, tmp, width, height, 1, callback);
    transposing_1d_blur(tmp, dst, height, width, 1, NULL);
    transposing_1d_blur(src, tmp, width, height, small, NULL);
    transposing_1d_blur(tmp, dst, height, width, small, NULL);
    transposing_1d_blur(dst, tmp, width, height, big, NULL);
    transposing_1d_blur(tmp, dst, height, width, big, NULL);
}

static void write_image(const char *filename, const rgba8 *pixels, int width, int height, float gamma)
{
    FILE *outfile = fopen(filename, "wb");
    if (!outfile) return;

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    png_init_io(png_ptr, outfile);
    png_set_compression_level(png_ptr, Z_BEST_SPEED);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA, 0, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_gAMA(png_ptr, info_ptr, gamma);
    png_write_info(png_ptr, info_ptr);

    for(int i=0; i < height; i++) {
        png_write_row(png_ptr, (png_bytep)(pixels + i*width));
    }

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
}

/*
 Algorithm based on Rabah Mehdi's C++ implementation

 frees memory in read_info structs.
 saves dissimilarity visualisation as ssimfilename (pass NULL if not needed)
 */
static double dssim_image(read_info *image1, read_info *image2, const char *ssimfilename)
{
    float gamma1 = image1->gamma,
          gamma2 = image2->gamma;
    int width = MIN(image1->width,image2->width),
       height = MIN(image1->height,image2->height);

    laba *restrict img1 = malloc(width*height*sizeof(laba));
    laba *restrict img2 = malloc(width*height*sizeof(laba));
    laba *restrict img1_img2 = malloc(width*height*sizeof(laba));

    int offset=0;
    for(int j=0; j < height; j++) {
        rgba8 *restrict px1 = (rgba8 *)image1->row_pointers[j];
        rgba8 *restrict px2 = (rgba8 *)image2->row_pointers[j];
        for(int i=0; i < width; i++, offset++) {

            laba f1 = rgba_to_laba(gamma1,px1[i]);
            laba f2 = rgba_to_laba(gamma2,px2[i]);

            // Compose image on coloured background to better judge dissimilarity with various backgrounds
            int n=i^j;
            if (n&4) {
                f1.l += 1.0-f1.a; // using premultiplied alpha
                f2.l += 1.0-f2.a;
            }
            if (n&8) {
                f1.A += 1.0-f1.a;
                f2.A += 1.0-f2.a;
            }

            if (n&16) {
                f1.b += 1.0-f1.a;
                f2.b += 1.0-f2.a;
            }

            img1[offset] = f1;
            img2[offset] = f2;

            // part of computation
            LABA_OP(img1_img2[offset],f1,*,f2);
        }
    }

    // freeing memory as early as possible
    free(image1->row_pointers); image1->row_pointers = NULL;
    free(image1->rgba_data); image1->rgba_data = NULL;
    free(image2->row_pointers); image2->row_pointers = NULL;
    free(image2->rgba_data); image2->rgba_data = NULL;

    laba *tmp = malloc(width*height*sizeof(laba));
    laba *restrict sigma12 = malloc(width*height*sizeof(laba));
    blur(img1_img2, tmp, sigma12, width, height, NULL);

    laba *restrict mu1 = img1_img2; //free(img1_img2); malloc(width*height*sizeof(f_pixel));
    blur(img1, tmp, mu1, width, height, NULL);
    laba *restrict sigma1_sq = malloc(width*height*sizeof(laba));
    blur(img1, tmp, sigma1_sq, width, height, square_row);

    laba *restrict mu2 = img1; //free(img1); malloc(width*height*sizeof(f_pixel));
    blur(img2, tmp, mu2, width, height, NULL);
    laba *restrict sigma2_sq = malloc(width*height*sizeof(laba));
    blur(img2, tmp, sigma2_sq, width, height, square_row);
    free(img2);
    free(tmp);

    rgba8 *ssimmap = (rgba8*)mu1; // result can overwrite source. it's safe because sizeof(rgb) <= sizeof(fpixel)

    const double c1 = 0.01*0.01, c2 = 0.03*0.03;
    laba avgssim = {0,0,0,0};

#define SSIM(r) 1.0-((2.0*(mu1[offset].r*mu2[offset].r) + c1) \
                *(2.0*(sigma12[offset].r-(mu1[offset].r*mu2[offset].r)) + c2)) \
                / \
                (((mu1[offset].r*mu1[offset].r) + (mu2[offset].r*mu2[offset].r) + c1) \
                 *((sigma1_sq[offset].r-(mu1[offset].r*mu1[offset].r)) + (sigma2_sq[offset].r-(mu2[offset].r*mu2[offset].r)) + c2))

    for(offset=0; offset < width*height; offset++) {
        laba ssim = (laba) {
            SSIM(l),
            SSIM(A),
            SSIM(b),
            SSIM(a),
        };

        LABA_OP1(avgssim,+=,ssim);

        if (ssimfilename) {
            float max = MAX(MAX(ssim.l,ssim.A),ssim.b);
            float maxsq = max*max;
            ssimmap[offset] = rgbaf_to_8(gamma2, (rgbaf){
                ssim.a + maxsq,
                max + maxsq,
                max*0.5f + ssim.a*0.5f + maxsq,
                1
            });
        }
    }

    // mu1 is reused for ssimmap
    free(mu2);
    free(sigma12);
    free(sigma1_sq);
    free(sigma2_sq);

    LABA_OPC(avgssim,avgssim,/,((double)width*height));

    // I'm arbitrarily lowering score for alpha dissimilarity
    // (in premultiplied alpha colorspace it's going to affect colors anyway)
    double minavgssim = MAX(MAX(avgssim.l,avgssim.A),MAX(avgssim.b,avgssim.a*0.75));

    if (ssimfilename) {
        write_image(ssimfilename,ssimmap,width,height,1.0/2.2);
    }
    free(ssimmap);

    return minavgssim;
}

/*
 Reads image into read_info struct. Returns non-zero on error
 */
static int read_image(const char *filename, read_info *image)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) return 1;

    int retval = rwpng_read_image(fp, image);

    fclose(fp);
    return retval;
}

int main(int argc, const char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s file1 file2 [output]\n",argv[0]);
        return 1;
    }

    const char *file1 = argv[1];
    const char *file2 = argv[2];
    const char *outfile = argc > 3 ? argv[3] : NULL;

    read_info image1;
    int retval = read_image(file1, &image1);
    if (retval) {
        fprintf(stderr, "Can't read %s\n",file1);
        return retval;
    }

    read_info image2;
    retval = read_image(file2, &image2);
    if (retval) {
        fprintf(stderr, "Can't read %s\n",file2);
        return retval;
    }

    double dssim = dssim_image(&image1, &image2, outfile);
    printf("%.4f\n",dssim);
    return 0;
}
