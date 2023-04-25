SDL_LDFLAGS := $(shell sdl2-config --libs)
SDL_CCFLAGS := $(shell sdl2-config --cflags)
CCFLAGS     := -I.
VVPFLAGS    := -M. -mvgasim

all: vgasim.vpi

demo: demo.vvp vgasim.vpi
	vvp $(VVPFLAGS) $<

%.vpi: %.c
	iverilog-vpi $< $(SDL_CCFLAGS) $(CCFLAGS) $(SDL_LDFLAGS)

%.vvp: %.v
	iverilog $< -o $@
