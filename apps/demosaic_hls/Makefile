include ../support/Makefile.inc

#### HLS flags

AUTOPILOT_ROOT := /cad/xilinx/vivado/2015.2/Vivado_HLS/2015.2
AUTOPILOT_MACH := lnx64
AUTOPILOT_TOOL := ${AUTOPILOT_ROOT}/${AUTOPILOT_MACH}/tools
AUTOPILOT_TECH := ${AUTOPILOT_ROOT}/common/technology
IFLAG += -I "${AUTOPILOT_TOOL}/systemc/include"
IFLAG += -I "${AUTOPILOT_ROOT}/include"
IFLAG += -I "${AUTOPILOT_ROOT}/include/ap_sysc"
IFLAG += -I "${AUTOPILOT_TOOL}/auto_cc/include"

IFLAG += -I "../hls_support"

include ../hls_support/Makefile.autopilot

#### end of HLS flags



all: demosaic.png

pipeline: pipeline.cpp
	$(CXX) $(CXXFLAGS) -Wall pipeline.cpp $(LDFLAGS) $(LIB_HALIDE) -o pipeline -lpthread -ldl -lz -ltinfo -g

pipeline_hls.cpp pipeline_native.o: pipeline
	HL_DEBUG_CODEGEN=0 ./pipeline

run: run.cpp pipeline_native.o pipeline_hls.cpp hls_target.cpp
	$(CXX) $(CXXFLAGS) -g -Wall -Wno-unknown-pragmas -Werror $^ -lpthread -ldl -o $@  $(PNGFLAGS)

demosaic.png: run
	./run

clean:
	rm -f pipeline pipeline_hls.{h,cpp} pipeline_native.{h,o} filter
