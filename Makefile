SDL_LDFLAGS := $(shell sdl-config --libs)
SDL_CCFLAGS := $(shell sdl-config --cflags)
CCFLAGS     := -I.
VVPFLAGS    := -M. -mvgasim

all: vgasim.vpi

demo: demo.vvp vgasim.vpi
	vvp $(VVPFLAGS) $<

%.vpi: %.c
	iverilog-vpi $< $(SDL_CCFLAGS) $(CCFLAGS) $(SDL_LDFLAGS)

%.vvp: %.v
	iverilog $< -o $@
