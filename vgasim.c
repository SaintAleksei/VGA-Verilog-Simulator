#include <vpi_user.h>
#include <SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

// SA: It is used to exclude useless for my own purposes code
#define SA_EXCLUDE 1

typedef struct
{
  SDL_Surface *surface;
  vpiHandle red;
  vpiHandle green;
  vpiHandle blue;
  vpiHandle hsync;
  vpiHandle vsync;
  vpiHandle pclk;
  uint32_t rmax;
  uint32_t gmax;
  uint32_t bmax;
  uint32_t x;
  uint32_t y;
  uint32_t last_time;
} pixel_clock_cb_data_t;

#if !SA_EXCLUDE
int offset_x = 0, offset_y = 0;
int offset_delta_x = 0, offset_delta_y = 0;
int clocks_until_update_offset = 0;
#endif

static void vgasim_events() {
  // Allow the simulation to be ended by closing the SDL window.
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        // SA: FIXME: Finish only current screen
        vpi_sim_control(vpiFinish, 0);
        break;
#if !SA_EXCLUDE
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
          case 273: // UP
            offset_delta_y = -1; break;
          case 274: // DOWN
            offset_delta_y = 1; break;
          case 276: // LEFT
            offset_delta_x = -1; break;
          case 275: // RIGHT
            offset_delta_x = 1; break;
        }
        break;
      case SDL_KEYUP:
        offset_delta_x = 0;
        offset_delta_y = 0;
        break;
#endif
      default:
        ;
    }
  }

#if !SA_EXCLUDE
  if (clocks_until_update_offset == 0) {
    offset_x += offset_delta_x;
    offset_y += offset_delta_y;
    clocks_until_update_offset = 255;
  }
  else {
    clocks_until_update_offset--;
  }
#endif
}

static int vgasim_pixel_cb(s_cb_data *cb_data) {
  // Only draw a pixel on a rising pixel clock.
  if (! cb_data->value->value.integer) {
    return 0;
  }

  pixel_clock_cb_data_t *cb_ctx = (pixel_clock_cb_data_t *) cb_data->user_data;

  s_vpi_value value;
  value.format = vpiIntVal;

#if !SA_EXCLUDE
  SDL_Rect clear_rect;
  clear_rect.x = 0;
  clear_rect.y = 0;
  clear_rect.w = width;
  clear_rect.h = 1;

  // For now we consider only the low clock, since we generally
  // care only about differences anyway.
  unsigned int time = cb_data->time->low;
  unsigned int last_pclk_len = pclk_len;
  pclk_len = time - last_time;
  if (pclk_len != last_pclk_len) {
    if (last_pclk_len > 0) {
      vpi_printf(
        "WARNING: Pixel clock period changed from %i to %i\n",
        last_pclk_len, pclk_len
      );
    }
    else {
      vpi_printf(
        "INFO: Pixel clock period is %i\n", pclk_len
      );
    }
  }
  last_time = time;
#endif

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

#if !SA_EXCLUDE
  int phys_x = x + offset_x;
  int phys_y = y + offset_y;
#endif

  // If we're in bounds, draw a pixel.
  if (cb_ctx->x < cb_ctx->surface->w && 
      cb_ctx->y < cb_ctx->surface->h) {
    print_time("Before lock");
    SDL_LockSurface(cb_ctx->surface);
    print_time("After lock");
    uint32_t pixel_value = (
      rval << cb_ctx->surface->format->Rshift |
      gval << cb_ctx->surface->format->Gshift |
      bval << cb_ctx->surface->format->Bshift
    );
    unsigned offset = (cb_ctx->y * (cb_ctx->surface->pitch / 4)) + cb_ctx->x;
    ((uint32_t*)cb_ctx->surface->pixels)[offset] = pixel_value;
    SDL_UnlockSurface(cb_ctx->surface);
  }

#if !SA_EXCLUDE
  vpi_get_value(hsync, &value);
  int new_hsync = ! value.value.integer;
  vpi_get_value(vsync, &value);
  int new_vsync = ! value.value.integer;
  x++;
  if (in_hsync && ! new_hsync) {
    if ((x - 1) != line_len) {
      if ((x - 1) > width) {
        vpi_printf(
          "WARNING: Line length is %i, but was expecting %i\n",
          x - 1, width
        );
      }
    }
    line_len = x - 1;
    x = 0;
    // Update the screen every lines.
    SDL_UpdateRect(surface, 0, y, width, 1);
    y++;
    if (y < height) {
      clear_rect.y = y;
      SDL_FillRect(
        surface, &clear_rect, SDL_MapRGB(surface->format, 0, 0, 0)
      );
    }
  }
  if (in_vsync && ! new_vsync) {
    SDL_UpdateRect(surface, 0, 0, 0, 0);
    y = 0;
  }
  in_hsync = new_hsync;
  in_vsync = new_vsync;
#endif

  cb_ctx->x++;
  if (cb_ctx->x == cb_ctx->surface->w)
    cb_ctx->x = 0;

  if (cb_ctx->x == 0)
    cb_ctx->y++;

  if (cb_ctx->y == cb_ctx->surface->h)
  {
    SDL_Flip(cb_ctx->surface);
    cb_ctx->y = 0;
  }

  // Take an opportunity to poll for events.
  vgasim_events();

  return 0;
}

static int vgasim_init_calltf(char *user_data) {
#if !SA_EXCLUDE
  if (surface) {
    vpi_printf("WARNING: Don't call $vgasimInit more than once\n");
    return 0;
  }
#endif

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
  uint32_t width = argval.value.integer;
  arg = vpi_scan(args_iter);
  vpi_get_value(arg, &argval);
  uint32_t height = argval.value.integer;

  cb_ctx->red   = vpi_scan(args_iter);
  cb_ctx->green = vpi_scan(args_iter);
  cb_ctx->blue  = vpi_scan(args_iter);
  cb_ctx->hsync = vpi_scan(args_iter);
  cb_ctx->vsync = vpi_scan(args_iter);
  cb_ctx->pclk  = vpi_scan(args_iter);

  vpi_free_object(args_iter);

  SDL_Surface *surface = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE);
  if (!surface)
  {
    vpi_printf("ERROR: Can\'t create window\n");
    free(cb_ctx);
    return 0;
  }
  
  cb_ctx->surface = surface;


  s_cb_data cb_config;
  struct t_vpi_time time;
  time.type = vpiSimTime;
  cb_config.user_data = cb_ctx;
  cb_config.reason = cbValueChange;
  cb_config.cb_rtn = vgasim_pixel_cb;
  cb_config.time = &time;
  cb_config.value = &argval; // borrow this from earlier
  cb_config.obj = cb_ctx->pclk;
  vpi_register_cb(&cb_config);
  time.type = vpiSimTime;

#if !SA_EXCLUDE
  argval.format = vpiObjTypeVal;
  vpi_get_value(r, &argval);
#endif

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
