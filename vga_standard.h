#ifndef VGA_STANDARD_H_INCLUDED
#define VGA_STANDARD_H_INCLUDED 1

typedef struct 
{
  char     *id;
  uint32_t hor_res;
  uint32_t hor_front_porch;
  uint32_t hor_sync_pulse;
  uint32_t hor_back_porch;
  uint32_t ver_res;
  uint32_t ver_front_porch;
  uint32_t ver_sync_pulse;
  uint32_t ver_back_porch;
} vga_standard_t;

extern const vga_standard_t vga_standard_list[];

#endif
