/* Include system header files -----------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Include user header files -------------------------------------------------*/
#include "bmp_rgb332.h"

/* Imported variables --------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#ifndef M_PI
#define M_PI 3.141592653
#endif

#ifndef RANGE
#define RANGE(x, min, max)	( (x < min) ? min : (x > max) ? max : x )
#endif

static const uint32_t FileHeaderSize  = 14; /* = 0x0E */
static const uint32_t InfoHeaderSize  = 40; /* = 0x28 */
static const uint32_t PaletteSize     = 256*4;
static const uint32_t AllHeaderOffset = FileHeaderSize + InfoHeaderSize + PaletteSize;
static const uint8_t rg_palette[8]    = {0x00, 0x24, 0x49, 0x6D, 0x92, 0xB6, 0xDB, 0xFF};
static const uint8_t b_palette[4]     = {0x00, 0x55, 0xAA, 0xFF};

/* Private types -------------------------------------------------------------*/
/* Private enum tag ----------------------------------------------------------*/
/* Private struct/union tag --------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static BMP_RGB332_Malloc_Function bmp_rgb332_malloc = malloc;
static BMP_RGB332_free_Function bmp_rgb332_free = free;

/* Exported function prototypes ----------------------------------------------*/
void	  BMP_RGB332_setAllocFunc(BMP_RGB332_Malloc_Function, BMP_RGB332_free_Function);
uint8_t * BMP_RGB332_create      (uint32_t, uint32_t);
void      BMP_RGB332_free        (uint8_t *);
uint32_t  BMP_RGB332_getWidth    (uint8_t *);
uint32_t  BMP_RGB332_getHeight   (uint8_t *);
uint32_t  BMP_RGB332_getFileSize (uint8_t *);
uint32_t  BMP_RGB332_getImageSize(uint8_t *);
uint32_t  BMP_RGB332_getOffset   (uint8_t *);
void      BMP_RGB332_setPixelRGB (uint8_t *, uint32_t, uint32_t, uint8_t, uint8_t, uint8_t);
void      BMP_RGB332_getPixelRGB (uint8_t *, uint32_t, uint32_t, uint8_t *, uint8_t *, uint8_t *);
void      BMP_RGB332_drawLineRGB (uint8_t *, int32_t, int32_t, int32_t, int32_t, uint8_t, uint8_t, uint8_t);
void      BMP_RGB332_drawRectRGB (uint8_t *, uint32_t, uint32_t, uint32_t, uint32_t, uint8_t, uint8_t, uint8_t);
void      BMP_RGB332_fillRGB     (uint8_t *, uint8_t, uint8_t, uint8_t);
uint8_t * BMP_RGB332_copy(uint8_t *);
uint8_t * BMP_RGB332_resize_bicubic(uint8_t *, uint32_t, uint32_t);

/* Private function prototypes -----------------------------------------------*/
static uint8_t convertRGBtoRGB332(uint8_t, uint8_t, uint8_t);
static uint32_t BMP_RGB332_read_uint32_t(uint8_t *);
static uint16_t BMP_RGB332_read_uint16_t(uint8_t *);
static void BMP_RGB332_write_uint32_t(uint32_t, uint8_t *);
static void BMP_RGB332_write_uint16_t(uint16_t, uint8_t *);
static uint32_t BMP_RGB332_getBytesPerRow(uint32_t);

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Set memory allocation funciton.
  * 		Call only once, before calling any other function from parson API.
  * 		If not called, malloc and free from stdlib will be used for all allocations.
  * @param  malloc_func memory allocate function.
  * @param  free_func memory free function.
  * @retval None
  */
void BMP_RGB332_setAllocFunc(BMP_RGB332_Malloc_Function malloc_func, BMP_RGB332_free_Function free_func)
{
	bmp_rgb332_malloc = malloc_func;
	bmp_rgb332_free = free_func;
}

/**
  * @brief  Create BMP RGB332 image.
  * @param  width width of image [pixel]
  * @param  height height of image [pixel]
  * @retval pointer to the created image. When error, return NULL
  */
uint8_t *BMP_RGB332_create(uint32_t width, uint32_t height)
{
    uint8_t *pbmp;
    uint32_t bytes_per_row = BMP_RGB332_getBytesPerRow(width);
    uint32_t image_size = bytes_per_row * height;
    uint32_t data_size = AllHeaderOffset + image_size;

    /* Allocate the bitmap data */
    pbmp = (uint8_t *)bmp_rgb332_malloc(sizeof(uint8_t) * data_size);
    if (pbmp == NULL)
        return NULL;
    for(uint32_t i = 0; i < data_size; i++)
        *(pbmp + i) = 0;

    // Set header's default values
    uint8_t *tmp = pbmp;
    *(tmp  +  0) = 'B';                                        // 'B' : Magic number
    *(tmp  +  1) = 'M';                                        // 'M' : Magic number
    BMP_RGB332_write_uint32_t(data_size        , tmp + 0x02);  // File Size
    BMP_RGB332_write_uint16_t(0                , tmp + 0x06);  // Reserved1
    BMP_RGB332_write_uint16_t(0                , tmp + 0x08);  // Reserved2
    BMP_RGB332_write_uint32_t(AllHeaderOffset  , tmp + 0x0A);  // Offset
    tmp += FileHeaderSize;    // Next

    // Info header
    BMP_RGB332_write_uint32_t( InfoHeaderSize  , tmp + 0x00);   // HeaderSize
    BMP_RGB332_write_uint32_t( width           , tmp + 0x04);  // width  (*** Signed value ***)
    BMP_RGB332_write_uint32_t( height          , tmp + 0x08);  // height (*** Signed value ***)
    BMP_RGB332_write_uint16_t( 1               , tmp + 0x0C);  // planes
    BMP_RGB332_write_uint16_t( 8               , tmp + 0x0E);  // Bit count
    BMP_RGB332_write_uint32_t( 0               , tmp + 0x10);  // Bit compression
    BMP_RGB332_write_uint32_t( image_size      , tmp + 0x14);  // Image size
    BMP_RGB332_write_uint32_t( 0               , tmp + 0x18);  // X pixels per meter
    BMP_RGB332_write_uint32_t( 0               , tmp + 0x1C);  // Y pixels per meter
    BMP_RGB332_write_uint32_t( 256             , tmp + 0x20);  // Color index
    BMP_RGB332_write_uint32_t( 0               , tmp + 0x24);  // Important index
    tmp += InfoHeaderSize;    // Next

    // Palette data
    for(uint16_t i = 0; i < 256; i++)
    {
        *tmp++ = b_palette [(i & 0x03)];        // Blue
        *tmp++ = rg_palette[(i & 0x1C) >> 2];   // Greem
        *tmp++ = rg_palette[(i & 0xE0) >> 5];   // Red
        *tmp++ = 0;                             // Reserved
    }

    return pbmp;
}

/**
  * @brief  Free BMP RGB332 image.
  * @param  pbmp pointer to a image
  * @retval None
  */
void BMP_RGB332_free(uint8_t *pbmp)
{
	bmp_rgb332_free(pbmp);
}


/**
  * @brief  Get width in pixel of a image.
  * @param  pbmp pointer to a image
  * @retval width [pixel]
  */
uint32_t BMP_RGB332_getWidth(uint8_t *pbmp)
{
    return BMP_RGB332_read_uint32_t(pbmp + FileHeaderSize + 0x04);
}

/**
  * @brief  Get height in pixel of a image.
  * @param  pbmp pointer to a image
  * @retval height [pixel]
  */
uint32_t BMP_RGB332_getHeight(uint8_t *pbmp)
{
    return BMP_RGB332_read_uint32_t(pbmp + FileHeaderSize + 0x08);
}

/**
  * @brief  Get file size of a image.
  * @param  pbmp pointer to a image
  * @retval file size [byte]
  */
uint32_t BMP_RGB332_getFileSize(uint8_t *pbmp)
{
    return BMP_RGB332_read_uint32_t(pbmp + 0x02);
}

/**
  * @brief  Get image size of a image.
  * @param  pbmp pointer to a image
  * @retval Image size [byte]
  */
uint32_t BMP_RGB332_getImageSize(uint8_t *pbmp)
{
    return BMP_RGB332_read_uint32_t(pbmp + FileHeaderSize + 0x14);
}

/**
  * @brief  Get header offset size of a image.
  * @param  pbmp pointer to a image
  * @retval Header offset size [byte]
  */
uint32_t BMP_RGB332_getOffset(uint8_t *pbmp)
{
    return BMP_RGB332_read_uint32_t(pbmp + 0x0A);
}

/**
  * @brief  Draw a color in RGB format on a specified pixel.
  * @param  pbmp pointer to a image
  * @param  x	x of a image(Range:[0,width-1] ) [pixel]
  * @param  y	y of a image(Range:[0,height-1]) [pixel]
  * @param  r	Red   value [0, 255] (Lower 5 bits are ignored)
  * @param  g	Green value [0, 255] (Lower 5 bits are ignored)
  * @param  b	Blue  value [0, 255] (Lower 6 bits are ignored)
  * @retval None
  */
void BMP_RGB332_setPixelRGB(uint8_t *pbmp, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t width  = BMP_RGB332_getWidth (pbmp);
    uint32_t height = BMP_RGB332_getHeight (pbmp);

    if(pbmp == NULL || x >= width || y >= height)
        return;

    uint32_t bytes_per_row = BMP_RGB332_getBytesPerRow(width);
    uint8_t col = convertRGBtoRGB332(r, g, b);

    *(pbmp + AllHeaderOffset + bytes_per_row * (height - y - 1) + x) = col;
}

/**
  * @brief  Get a color in RGB format on a specified pixel.
  * @param  pbmp pointer to a image
  * @param  x	x of a image(Range:[0,width-1] ) [pixel]
  * @param  y	y of a image(Range:[0,height-1]) [pixel]
  * @param  r	Pointer to a red   value [0, 255] (Lower 3 bits are 0)
  * @param  g	Pointer to a green value [0, 255] (Lower 2 bits are 0)
  * @param  b	Pointer to a blue  value [0, 255] (Lower 3 bits are 0)
  * @retval None
  */
void BMP_RGB332_getPixelRGB(uint8_t *pbmp, uint32_t x, uint32_t y, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint32_t width  = BMP_RGB332_getWidth(pbmp);
    uint32_t height = BMP_RGB332_getHeight(pbmp);

    if(pbmp == NULL || x >= width || y >= height)
        return;

    uint32_t bytes_per_row = BMP_RGB332_getBytesPerRow(width);
    uint8_t col = *(pbmp + AllHeaderOffset + bytes_per_row * (height - y - 1) + x);
    *r = rg_palette[(col & 0xE0) >> 5];
    *g = rg_palette[(col & 0x1C) >> 2];
    *b = b_palette [(col & 0x03)];
}

/**
  * @brief  Draws a straight line in a specified RGB color.
  * @param  pbmp pointer to a image
  * @param  x0	Start x position of a line(Range:[0,width-1] ) [pixel]
  * @param  y0  Start y position of a line(Range:[0,width-1] ) [pixel]
  * @param  x1	End   x position of a line(Range:[0,width-1] ) [pixel]
  * @param  y1  End   y position of a line(Range:[0,width-1] ) [pixel]
  * @param  r	Red   value [0, 255] (Lower 5 bits are ignored)
  * @param  g	Green value [0, 255] (Lower 5 bits are ignored)
  * @param  b	Blue  value [0, 255] (Lower 6 bits are ignored)
  * @retval None
  * @detail Bresenham's line algorithm
  *         ref : https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C
  *         ref : https://ja.wikipedia.org/wiki/%E3%83%96%E3%83%AC%E3%82%BC%E3%83%B3%E3%83%8F%E3%83%A0%E3%81%AE%E3%82%A2%E3%83%AB%E3%82%B4%E3%83%AA%E3%82%BA%E3%83%A0#.E6.9C.80.E9.81.A9.E5.8C.96
  */
void BMP_RGB332_drawLineRGB(uint8_t *pbmp,
		int32_t x0, int32_t y0, int32_t x1, int32_t y1,
        uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t width  = BMP_RGB332_getWidth (pbmp);
    uint32_t height = BMP_RGB332_getHeight (pbmp);

    if(pbmp == NULL || x0 < 0 || x0 >= width || x1 < 0 || x1 >= width || y0 < 0 || y0 >= height || y1 < 0 || y1 >= height)
        return;

    uint32_t bytes_per_row = BMP_RGB332_getBytesPerRow(width);
    uint8_t col = convertRGBtoRGB332(r, g, b);

    int32_t dx = x1 - x0 > 0 ? x1 - x0 : x0 - x1;
    int32_t sx = x0 < x1 ? 1 : -1;
    int32_t dy = y1 - y0 > 0 ? y1 - y0 : y0 - y1;
    int32_t sy = y0 < y1 ? 1 : -1;
    int32_t err = dx - dy;
    int32_t e2;

    uint8_t *pbmp_data = pbmp + AllHeaderOffset;
    for (;;)
    {
        *(pbmp_data + bytes_per_row * (height - y0 - 1) + x0) = col;

        if (x0 == x1 && y0 == y1)
            break;

        e2 = 2*err;
        if (e2 > -dy) {err -= dy;   x0 += sx;}
        if (e2 <  dx) {err += dx;   y0 += sy;}
    }
}


/**
  * @brief  Draws a Rectangle in a specified RGB color.
  * @param  pbmp pointer to a image
  * @param  x0	Start x position of a line(Range:[0,width-1] ) [pixel]
  * @param  y0  Start y position of a line(Range:[0,width-1] ) [pixel]
  * @param  x1	End   x position of a line(Range:[0,width-1] ) [pixel]
  * @param  y1  End   y position of a line(Range:[0,width-1] ) [pixel]
  * @param  r	Red   value [0, 255] (Lower 5 bits are ignored)
  * @param  g	Green value [0, 255] (Lower 5 bits are ignored)
  * @param  b	Blue  value [0, 255] (Lower 6 bits are ignored)
  * @retval None
  */
void BMP_RGB332_drawRectRGB(uint8_t *pbmp,
		uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1,
        uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t width = BMP_RGB332_getWidth(pbmp);
    uint32_t height = BMP_RGB332_getHeight(pbmp);

    if (pbmp == NULL || x0 >= width || x1 >= width || y0 >= height || y1 >= height)
        return;

    uint32_t swap;
    if(x0 > x1)
    {
        swap = x0;
        x0 = x1;
        x1 = swap;
    }
    if (y0 > y1)
    {
        swap = y0;
        x0 = y1;
        y1 = swap;
    }

    uint32_t bytes_per_row = BMP_RGB332_getBytesPerRow(width);
    uint8_t col = convertRGBtoRGB332(r, g, b);

    uint8_t *pbmp_data = pbmp + AllHeaderOffset;

    int32_t y_addr_start = bytes_per_row*(height - y0 - 1);
    int32_t y_addr_end   = bytes_per_row*(height - y1 - 1);
    int32_t x_addr_start = x0;// * 2;
    int32_t x_addr_end   = x1;// * 2;
    for(int32_t y_addr = y_addr_start; y_addr >= y_addr_end; y_addr -= bytes_per_row)
    {
        for(int32_t x_addr = x_addr_start; x_addr <= x_addr_end; x_addr += 1)
            *(pbmp_data + y_addr + x_addr) = col;
    }
}

/**
  * @brief  Fill image in a specified RGB color.
  * @param  pbmp pointer to a image
  * @param  r	Red   value [0, 255] (Lower 5 bits are ignored)
  * @param  g	Green value [0, 255] (Lower 5 bits are ignored)
  * @param  b	Blue  value [0, 255] (Lower 6 bits are ignored)
  * @retval None
  */
void BMP_RGB332_fillRGB(uint8_t *pbmp, uint8_t r, uint8_t g, uint8_t b)
{
    if (pbmp == NULL)
        return;

    BMP_RGB332_drawRectRGB(pbmp, 0, 0, BMP_RGB332_getWidth(pbmp)-1, BMP_RGB332_getHeight(pbmp)-1, r, g, b);
}


/**
  * @brief  Copy image.
  * @param  pbmp pointer to a source image
  * @retval pointer to the copied image. When error, return NULL
  */
uint8_t *BMP_RGB332_copy(uint8_t *pbmp)
{
    uint8_t *pbmpDst = BMP_RGB332_create(BMP_RGB332_getWidth(pbmp), BMP_RGB332_getHeight(pbmp));
    if(pbmpDst == NULL)
        return NULL;
    memcpy(pbmpDst, pbmp, BMP_RGB332_getFileSize(pbmp));
    return pbmpDst;
}


/**
  * @brief  Bicubic Interpolation.
  * @param  pbmpSrc pointer to a source image
  * @param  width width of interpolated image [pixel]
  * @param  height height of interpolated image [pixel]
  * @retval pointer to the created image. When error, return NULL.
  * @detail ref: http://docs-hoffmann.de/bicubic03042002.pdf (p.8)
  */
uint8_t *BMP_RGB332_resize_bicubic(uint8_t *pbmpSrc, uint32_t width, uint32_t height)
{
	uint8_t *pbmpDst;
	int src_x_size, src_y_size, dst_x_size, dst_y_size;
	float C[4][3];
	float d0[3], d2[3], d3[3], a0[3], a1, a2, a3;
	int x, y;
	float dx, dy;
	float tx, ty;
    uint8_t rgb1[3], rgb2[3], rgb_out[3];

	pbmpDst = BMP_RGB332_create(width, height);
	if(pbmpDst == NULL)
		return NULL;

    for (int i = 0; i < 4; i++)
    for (int j = 0; j < 3; j++)
        C[i][j] = 0.0f;

	src_x_size = BMP_RGB332_getWidth (pbmpSrc);
	src_y_size = BMP_RGB332_getHeight(pbmpSrc);
	dst_x_size = width;
	dst_y_size = height;

	tx = (float)src_x_size / dst_x_size;
	ty = (float)src_y_size / dst_y_size;

	for (int dstCol = 0; dstCol < dst_y_size; dstCol++)
	{
		for (int dstRow = 0; dstRow < dst_x_size; dstRow++)
		{
			x = (int) (tx * dstRow);
			y = (int) (ty * dstCol);

			dx = tx * dstRow - x;
			dy = ty * dstCol - y;

			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j <= 3; j++)
				{
					BMP_RGB332_getPixelRGB(pbmpSrc, RANGE((x - 1) + i, 0, src_x_size-1), RANGE(y - 1 + j, 0, src_y_size-1), &rgb1[0], &rgb1[1], &rgb1[2]);
					BMP_RGB332_getPixelRGB(pbmpSrc, RANGE((x)     + i, 0, src_x_size-1), RANGE(y - 1 + j, 0, src_y_size-1), &rgb2[0], &rgb2[1], &rgb2[2]);
					for(int rgb_i = 0; rgb_i < 3; rgb_i++)
						d0[rgb_i] = rgb1[rgb_i] - rgb2[rgb_i];

					BMP_RGB332_getPixelRGB(pbmpSrc, RANGE((x + 1) + i, 0, src_x_size-1), RANGE(y - 1 + j, 0, src_y_size-1), &rgb1[0], &rgb1[1], &rgb1[2]);
					BMP_RGB332_getPixelRGB(pbmpSrc, RANGE((x)     + i, 0, src_x_size-1), RANGE(y - 1 + j, 0, src_y_size-1), &rgb2[0], &rgb2[1], &rgb2[2]);
					for(int rgb_i = 0; rgb_i < 3; rgb_i++)
						d2[rgb_i] = rgb1[rgb_i] - rgb2[rgb_i];

					BMP_RGB332_getPixelRGB(pbmpSrc, RANGE((x + 2) + i, 0, src_x_size-1), RANGE(y - 1 + j, 0, src_y_size-1), &rgb1[0], &rgb1[1], &rgb1[2]);
					BMP_RGB332_getPixelRGB(pbmpSrc, RANGE((x)     + i, 0, src_x_size-1), RANGE(y - 1 + j, 0, src_y_size-1), &rgb2[0], &rgb2[1], &rgb2[2]);
					for(int rgb_i = 0; rgb_i < 3; rgb_i++)
						d3[rgb_i] = rgb1[rgb_i] - rgb2[rgb_i];

					BMP_RGB332_getPixelRGB(pbmpSrc, RANGE((x)     + i, 0, src_x_size-1), RANGE(y - 1 + j, 0, src_y_size-1), &rgb1[0], &rgb1[1], &rgb1[2]);
					for(int rgb_i = 0; rgb_i < 3; rgb_i++)
						a0[rgb_i] = rgb1[rgb_i];

					for(int rgb_i = 0; rgb_i < 3; rgb_i++)
					{
						a1 = -1.0f / 3 * d0[rgb_i] +            d2[rgb_i] - 1.0f / 6 * d3[rgb_i];
						a2 =  1.0f / 2 * d0[rgb_i] + 1.0f / 2 * d2[rgb_i];
						a3 = -1.0f / 6 * d0[rgb_i] - 1.0f / 2 * d2[rgb_i] + 1.0f / 6 * d3[rgb_i];
						C[j][rgb_i] = a0[rgb_i] + a1 * dx + a2 * dx * dx + a3 * dx * dx * dx;

						float d0_2, d2_2, d3_2, a0_2;
						d0_2 = C[0][rgb_i] - C[1][rgb_i];
						d2_2 = C[2][rgb_i] - C[1][rgb_i];
						d3_2 = C[3][rgb_i] - C[1][rgb_i];
						a0_2 = C[1][rgb_i];
						a1 = -1.0f / 3 * d0_2 +            d2_2 - 1.0f / 6 * d3_2;
						a2 =  1.0f / 2 * d0_2 + 1.0f / 2 * d2_2;
						a3 = -1.0f / 6 * d0_2 - 1.0f / 2 * d2_2 + 1.0f / 6 * d3_2;

						rgb_out[rgb_i] = RANGE( (int)(a0_2 + a1 * dy + a2 * dy * dy + a3 * dy * dy * dy + 0.5f), 0, 255);
					}
					BMP_RGB332_setPixelRGB(pbmpDst, dstRow + i, dstCol,  rgb_out[0], rgb_out[1], rgb_out[2]);
				}
			}
		}
	}
	return pbmpDst;
}


/* Private functions ---------------------------------------------------------*/
static uint8_t convertRGBtoRGB332(uint8_t r, uint8_t g, uint8_t b)
{
    return   (r & 0xE0)
          | ((g & 0xE0) >> 3)
          | ((b & 0xC0) >> 6);
}


// Calculate the number of bytes used to store a single image row.
// This is always rounded up to the next multiple of 4.
static uint32_t BMP_RGB332_getBytesPerRow(uint32_t width)
{
    if(width & 0x00000003)
        return (width & 0xFFFFFFFC) + 0x00000004;
    else
        return width;
}


/**************************************************************
    Reads a little-endian unsigned int from the file.
    Returns non-zero on success.
**************************************************************/
static uint32_t BMP_RGB332_read_uint32_t(uint8_t *pSrc)
{
    uint32_t retval = 0x00000000;
    retval |= (uint32_t)*(pSrc + 3) << 24;
    retval |= (uint32_t)*(pSrc + 2) << 16;
    retval |= (uint32_t)*(pSrc + 1) <<  8;
    retval |= (uint32_t)*(pSrc    );
    return retval;
}

/**************************************************************
    Reads a little-endian unsigned int from the file.
    Returns non-zero on success.
**************************************************************/
static uint16_t BMP_RGB332_read_uint16_t(uint8_t *pSrc)
{
    uint16_t retval = 0x0000;
    retval |= (uint16_t)*(pSrc + 1) <<  8;
    retval |= (uint16_t)* pSrc;
    return retval;
}

/**************************************************************
    Writes a little-endian unsigned short int to the file.
    Returns non-zero on success.
**************************************************************/
static void BMP_RGB332_write_uint32_t(uint32_t Src, uint8_t *pDst)
{
    *(pDst + 3) = (uint8_t)( ( Src & 0xff000000 ) >> 24 );
    *(pDst + 2) = (uint8_t)( ( Src & 0x00ff0000 ) >> 16 );
    *(pDst + 1) = (uint8_t)( ( Src & 0x0000ff00 ) >> 8  );
    *pDst       = (uint8_t)  ( Src & 0x000000ff )        ;
}

/**************************************************************
	Writes a little-endian unsigned short int to the file.
	Returns non-zero on success.
**************************************************************/
static void BMP_RGB332_write_uint16_t(uint16_t Src, uint8_t *pDst)
{
    *(pDst + 1) = (uint8_t)( ( Src & 0xff00 ) >> 8 );
	*pDst       = (uint8_t)  ( Src & 0x00ff )       ;
}

/***************************************************************END OF FILE****/
