#### Halide flags
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

all: test

pipeline: pipeline.cpp
	$(CXX) $(CXXFLAGS) -Wall pipeline.cpp $(LDFLAGS) $(LIB_HALIDE) -o pipeline -lpthread -ldl -lz -ltinfo -g

pipeline_hls.cpp pipeline_native.o: pipeline
	HL_DEBUG_CODEGEN=0 ./pipeline

run: run.cpp pipeline_hls.cpp pipeline_native.o
	$(CXX) $(CXXFLAGS) -Wall  -Wno-unknown-pragmas -Wno-unused-label  run.cpp pipeline_hls.cpp hls_target.cpp pipeline_native.o  -g -lpthread -ldl -o run

test: run
	./run

clean:
	rm -f pipeline pipeline_native.{h,o} pipeline_hls.{h,cpp} hls_target.{h,cpp} run
