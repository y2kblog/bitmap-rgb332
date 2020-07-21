# Introduction
This library is a open source RGB332 format bitmap library.
RGB332 format is suitable for embedded systems because it does not consume much memory.
You can create/delete bitmap images, get the color of a point, draw points/lines/rectangles, etc.
This library also includes bicubic interpolation function.  
The library consists of only two files: `bmp_rgb332.c` and `bmp_rgb332.h`.  

#### File size
<img src="https://latex.codecogs.com/gif.latex?4&space;\lceil&space;\dfrac{w}{4}&space;\rceil&space;h&space;&plus;&space;1078&space;\&space;{\rm[bytes]\&space;,&space;where}\&space;w:{\rm&space;width&space;[pixel]},&space;h:{\rm&space;height[pixel]}" title="4 \lceil \dfrac{w}{4} \rceil h + 1078 \ {\rm[bytes]\ , where}\ w:{\rm width [pixel]}, h:{\rm height[pixel]}" />
<!-- 4 \lceil \dfrac{w}{4} \rceil h + 1078 \ {\rm[bytes]\ , where}\ 
 w:{\rm width [pixel]}, h:{\rm height[pixel]} -->

RGB332 bitmap uses 256 color palette data (1024 bytes), data size will be larger than RGB565 or RGB888 if the image size is small.

# Test
`test.c` is test program.  
After downloading this repository, you can run the test program by executing the following command.  
```
gcc -o program test.c bmp_rgb332.c && ./program
```
