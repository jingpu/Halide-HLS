include ../support/Makefile.inc


#### HLS flags

CSIM_DESIGN = 1
__SIM_FPO__ = 1
__SIM_OPENCV__ = 1
__SIM_FFT__ = 1
__SIM_FIR__ = 1

AUTOPILOT_ROOT := /cad/xilinx/vivado/2014.3/Vivado_HLS/2014.3
AUTOPILOT_MACH := lnx64
ifdef AP_GCC_M32
  AUTOPILOT_MACH := Linux_x86
  IFLAG += -m32
endif
AUTOPILOT_TOOL := ${AUTOPILOT_ROOT}/${AUTOPILOT_MACH}/tools
AUTOPILOT_TECH := ${AUTOPILOT_ROOT}/common/technology

IFLAG += -I "${AUTOPILOT_TOOL}/systemc/include"
IFLAG += -I "${AUTOPILOT_ROOT}/include"
IFLAG += -I "${AUTOPILOT_ROOT}/include/ap_sysc"
IFLAG += -I "${AUTOPILOT_TECH}/generic/SystemC"
IFLAG += -I "${AUTOPILOT_TECH}/generic/SystemC/AESL_FP_comp"
IFLAG += -I "${AUTOPILOT_TECH}/generic/SystemC/AESL_comp"
IFLAG += -I "${AUTOPILOT_TOOL}/auto_cc/include"
IFLAG += -I "/usr/include/x86_64-linux-gnu"

IFLAG += -D__SIM_FPO__
IFLAG += -D__SIM_OPENCV__
IFLAG += -D__SIM_FFT__
IFLAG += -D__SIM_FIR__


include ./Makefile.autopilot


#### end of HLS flags

all: pipeline_c.c

pipeline: pipeline.cpp
	$(CXX) $(CXXFLAGS) -Wall pipeline.cpp $(LDFLAGS) ../../$(LIB_HALIDE) -o pipeline -lpthread -ldl -lz -ltinfo -g

pipeline_c.c: pipeline
	./pipeline

pipeline_c.h: pipeline
	./pipeline

pipeline_native.h: pipeline
	./pipeline

pipeline_native.o: pipeline
	./pipeline

run: run.cpp pipeline_native.h pipeline_hls.c pipeline_hw.cpp stencil.h
	$(CXX) $(CXXFLAGS) -Wall run.cpp pipeline_hls.c pipeline_hw.cpp pipeline_native.o -lpthread -o run 

test: run
	./run

clean:
	rm -f run pipeline_native.{h,o} pipeline_c.{c,h} pipeline
