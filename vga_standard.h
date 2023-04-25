#ifndef VGA_STANDARD_H_INCLUDED
#define VGA_STANDARD_H_INCLUDED 1

typedef struct 
{
  uint32_t pixel_freq;
  uint16_t hor_res;
  uint16_t hor_front_porch;
  uint16_t hor_sync_pulse;
  uint16_t hor_back_porch;
  uint16_t ver_res;
  uint16_t hor_front_porch;
  uint16_t hor_sync_pulse;
  uint16_t hor_back_porch;
  char     id;
} vga_standard_t;

extern const vga_standard_t vga_standard_list[];

#endif
