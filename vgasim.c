#include <vpi_user.h>
#include <SDL.h>

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <vga_standard.h>

const vga_standard_t vga_standard =
{
  .id = "640x480_60Hz",
  .hor_res = 640,
  .ver_res = 480,
  .hor_front_porch = 16,
  .hor_sync_pulse = 96,
  .hor_back_porch = 48,
  .ver_front_porch = 10,
  .ver_sync_pulse = 2,
  .ver_back_porch = 33
};

typedef struct
{
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  uint32_t *video_buffer;
  vpiHandle red;
  vpiHandle green;
  vpiHandle blue;
  vpiHandle hsync;
  vpiHandle vsync;
  vpiHandle pclk;
  uint32_t rmax;
  uint32_t gmax;
  uint32_t bmax;
  uint32_t width;
  uint32_t heigh;
  uint32_t hor_cnt;
  uint32_t ver_cnt;
  uint32_t hor_back_porch_start;
  uint32_t hor_data_start;
  uint32_t hor_front_porch_start;
  uint32_t hor_max;
  uint32_t ver_back_porch_start;
  uint32_t ver_data_start;
  uint32_t ver_front_porch_start;
  uint32_t ver_max;
} pixel_clock_cb_data_t;

static void vgasim_events() {
  // Allow the simulation to be ended by closing the SDL window.
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        // SA: FIXME: Finish only current screen
        vpi_sim_control(vpiFinish, 0);
        break;
      default:
        ;
    }
  }
}

static int vgasim_pixel_cb(s_cb_data *cb_data) {
  // Take an opportunity to poll for events.
  vgasim_events();
  // Only draw a pixel on a rising pixel clock.
  if (! cb_data->value->value.integer) {
    return 0;
  }

  pixel_clock_cb_data_t *cb_ctx = (pixel_clock_cb_data_t *) cb_data->user_data;

  s_vpi_value value;
  value.format = vpiIntVal;

  unsigned int hsync, vsync;
  vpi_get_value(cb_ctx->hsync, &value);
  hsync = value.value.integer;
  vpi_get_value(cb_ctx->vsync, &value);
  vsync = value.value.integer;

#if 0
  vpi_printf("vgasim: %d;%d;%d;%d;\n", cb_ctx->hor_cnt, cb_ctx->ver_cnt, hsync, vsync);
#endif

  // Check for VGA standard, reset if failed
  if ((cb_ctx->hor_cnt < cb_ctx->hor_back_porch_start && hsync)   ||
      (cb_ctx->hor_cnt >= cb_ctx->hor_back_porch_start && !hsync) ||
      (cb_ctx->ver_cnt < cb_ctx->ver_back_porch_start && vsync)   ||
      (cb_ctx->ver_cnt >= cb_ctx->ver_back_porch_start && !vsync))
  {
    cb_ctx->hor_cnt = 0;
    cb_ctx->ver_cnt = 0; 

    return 0;
  }

  if ((cb_ctx->hor_cnt >= cb_ctx->hor_data_start        &&
       cb_ctx->hor_cnt < cb_ctx->hor_front_porch_start) &&
      (cb_ctx->ver_cnt >= cb_ctx->ver_data_start        &&
       cb_ctx->ver_cnt < cb_ctx->ver_front_porch_start))
  {
    unsigned int rval, gval, bval;

    // Capture r, g and b to get the color value for this pixel.
    vpi_get_value(cb_ctx->red, &value);
    rval = value.value.integer;
    vpi_get_value(cb_ctx->green, &value);
    gval = value.value.integer;
    vpi_get_value(cb_ctx->blue, &value);
    bval = value.value.integer;

    // Scale the values to 8 bits per channel.
    // (We'll lose detail here if the source is >8 bits per channel)
    rval = (rval * 255) / cb_ctx->rmax;
    gval = (gval * 255) / cb_ctx->gmax;
    bval = (bval * 255) / cb_ctx->bmax;

    uint32_t pixel_value = rval << 24 | gval << 16 | bval << 8 | 0xFF;
    uint32_t x = cb_ctx->hor_cnt - cb_ctx->hor_data_start;
    cb_ctx->video_buffer[x] = pixel_value;
  }

  cb_ctx->hor_cnt++;
  if (cb_ctx->hor_cnt == cb_ctx->hor_max)
    cb_ctx->hor_cnt = 0;

  if (cb_ctx->hor_cnt == 0)
  {
    SDL_Rect update_rect = {0, cb_ctx->ver_cnt - cb_ctx->ver_data_start, cb_ctx->width, 1};
    SDL_UpdateTexture(cb_ctx->texture, &update_rect,
                      cb_ctx->video_buffer,
                      cb_ctx->width * sizeof(uint32_t));
    SDL_RenderClear(cb_ctx->renderer);
    SDL_RenderCopy(cb_ctx->renderer, cb_ctx->texture, NULL, NULL);
    SDL_RenderPresent(cb_ctx->renderer);
    cb_ctx->ver_cnt++;;
  }

  if (cb_ctx->ver_cnt == cb_ctx->ver_max)
    cb_ctx->ver_cnt = 0;

  return 0;
}

static int vgasim_init_calltf(char *user_data) {
  pixel_clock_cb_data_t *cb_ctx = calloc(1, sizeof(pixel_clock_cb_data_t));
  if (!cb_ctx)
  {
    vpi_printf("ERROR: Can\'t allocate memory for context\n");
    return 0;
  }

  vpiHandle systfref, args_iter, arg;
  struct t_vpi_value argval;

  systfref = vpi_handle(vpiSysTfCall, NULL);
  args_iter = vpi_iterate(vpiArgument, systfref);

  argval.format = vpiIntVal;

  arg = vpi_scan(args_iter);
  vpi_get_value(arg, &argval);
  cb_ctx->width = argval.value.integer;
  arg = vpi_scan(args_iter);
  vpi_get_value(arg, &argval);
  cb_ctx->heigh = argval.value.integer;

  cb_ctx->video_buffer = calloc(cb_ctx->width,
                                sizeof(*cb_ctx->video_buffer));
  if (!cb_ctx->video_buffer)
  {
    vpi_printf("ERROR: Can\'t allocate memory for video buffer\n");
    free(cb_ctx);
    return 0;
  }

  cb_ctx->red   = vpi_scan(args_iter);
  cb_ctx->green = vpi_scan(args_iter);
  cb_ctx->blue  = vpi_scan(args_iter);
  cb_ctx->hsync = vpi_scan(args_iter);
  cb_ctx->vsync = vpi_scan(args_iter);
  cb_ctx->pclk  = vpi_scan(args_iter);

  vpi_free_object(args_iter);

  SDL_Window *window = SDL_CreateWindow("VGA Verilog Simulator",
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED,
                                        cb_ctx->width, cb_ctx->heigh,
                                        SDL_WINDOW_SHOWN);
  if (!window)
  {
    vpi_printf("ERROR: Can\'t create window\n");
    free(cb_ctx->video_buffer);
    free(cb_ctx);
    return 0;
  }
  
  cb_ctx->window = window;

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer)
  {
    vpi_printf("ERROR: Can\'t create renderer\n");
    SDL_DestroyWindow(cb_ctx->window);
    free(cb_ctx->video_buffer);
    free(cb_ctx);
    return 0;
  }

  cb_ctx->renderer = renderer;

  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           cb_ctx->width, cb_ctx->heigh);
  if (!texture)
  {
    vpi_printf("ERROR: Can\'t create texture\n");
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    free(cb_ctx->video_buffer);
    free(cb_ctx);
    return 0;
  }

  cb_ctx->texture = texture;

  cb_ctx->hor_back_porch_start  = vga_standard.hor_sync_pulse;
  cb_ctx->hor_data_start        = cb_ctx->hor_back_porch_start +
                                  vga_standard.hor_back_porch;
  cb_ctx->hor_front_porch_start = cb_ctx->hor_data_start + 
                                  vga_standard.hor_res;
  cb_ctx->hor_max               = cb_ctx->hor_front_porch_start +
                                  vga_standard.hor_front_porch;
  cb_ctx->ver_back_porch_start  = vga_standard.ver_sync_pulse;
  cb_ctx->ver_data_start        = cb_ctx->ver_back_porch_start +
                                  vga_standard.ver_back_porch;
  cb_ctx->ver_front_porch_start = cb_ctx->ver_data_start + 
                                  vga_standard.ver_res;
  cb_ctx->ver_max               = cb_ctx->ver_front_porch_start +
                                  vga_standard.ver_front_porch;

  s_cb_data cb_config;
  s_vpi_time time;
  time.type = vpiSimTime;
  cb_config.user_data = cb_ctx;
  cb_config.reason = cbValueChange;
  cb_config.cb_rtn = vgasim_pixel_cb;
  cb_config.time = &time;
  cb_config.value = &argval; // borrow this from earlier
  cb_config.obj = cb_ctx->pclk;
  vpi_register_cb(&cb_config);
  time.type = vpiSimTime;

  cb_ctx->rmax = 1 << vpi_get(vpiSize, cb_ctx->red);
  cb_ctx->gmax = 1 << vpi_get(vpiSize, cb_ctx->green);
  cb_ctx->bmax = 1 << vpi_get(vpiSize, cb_ctx->blue);

  return 0;
}

void vgasim_register() {
  SDL_Init(SDL_INIT_VIDEO);

  s_vpi_systf_data config;

  config.type = vpiSysTask;
  config.tfname = "$vgasimInit";
  config.calltf = vgasim_init_calltf;
  config.compiletf = NULL;
  config.sizetf = 0;
  config.user_data = 0;
  vpi_register_systf(&config);
}

void (*vlog_startup_routines[])() = {
  vgasim_register,
  0
};
