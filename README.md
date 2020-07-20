# Introduction
This library is a free and open source RGB332 format Bitmap library.
RGB332 format is suitable for embedded systems because it does not consume much memory.
You can create/delete bitmap images, get the color of a point, draw points/lines/rectangles, etc.
This library also includes bicubic interpolation function.  
The library consists of only two files: `bmp_rgb332.c` and `bmp_rgb332.h`.  
*RGB332 bitmap uses 256 color palette data (1024 Bytes), data size will be larger than RGB565 or RGB888 if the image size is small.

# Test
`test.c` is test program.  
After downloading this repository, you can run the test program by executing the following command.  
```
gcc -o program test.c bmp_rgb332.c && ./program
```
