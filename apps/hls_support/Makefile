#### HLS flags

AUTOPILOT_ROOT := /cad/xilinx/vivado/2015.2/Vivado_HLS/2015.2
AUTOPILOT_MACH := lnx64
AUTOPILOT_TOOL := ${AUTOPILOT_ROOT}/${AUTOPILOT_MACH}/tools
AUTOPILOT_TECH := ${AUTOPILOT_ROOT}/common/technology
IFLAG += -I "${AUTOPILOT_TOOL}/systemc/include"
IFLAG += -I "${AUTOPILOT_ROOT}/include"
IFLAG += -I "${AUTOPILOT_ROOT}/include/ap_sysc"
IFLAG += -I "${AUTOPILOT_TOOL}/auto_cc/include"

include ./Makefile.autopilot


#### end of HLS flags
all: run


test_linebuffer: test_linebuffer.cpp Stencil.h Linebuffer.h
	$(CXX) -std=c++11 $(CXXFLAGS) -Wall -Wno-unused-label -Wno-unknown-pragmas test_linebuffer.cpp  -o test_linebuffer

run: test_linebuffer
	./test_linebuffer

clean:
	rm -f test_linebuffer
