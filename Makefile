TARGET			:= vgasim

CCOMP				:= iverilog-vpi
SOURCES     := vgasim.c vga_standard.c
SDL_LDFLAGS := $(shell sdl2-config --libs)
SDL_CCFLAGS := $(shell sdl2-config --cflags)
CFLAGS      := -I.

VCOMP			  := iverilog
VFLAGS			:= 

CLEANEXT 		:= o d vpi vvp vcd

.PHONY: all clean requirements

all: requirements $(TARGET)

requirements: #TODO

demo: demo.vvp $(TARGET)
	vvp -M. -m$(TARGET) $<

$(TARGET): $(SOURCES)
	$(CCOMP) $(SDL_CCFLAGS) $(CFLAGS) $^ $(SDL_LDFLAGS)

%.vvp: %.v
	$(VCOMP) $(VFLAGS) $< -o $@

clean:
	rm -rf $(addprefix *.,$(CLEANEXT))

-include $(patsubst %.o,%.d,$(OBJECTS))
