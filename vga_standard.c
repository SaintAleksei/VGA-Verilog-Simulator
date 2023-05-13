#include <stdint.h>
#include <vga_standard.h>

const vga_standard_t vga_standard_list[VGA_STANDARD_LIST_SIZE] =
{
  {"640x480_60Hz", 640, 16, 96, 48, 480, 11, 2, 31},
  {"1600x1200_60Hz", 1600, 64, 192, 304, 1200, 1, 3, 46}
  // SA: Add any other VGA standard you need
};
