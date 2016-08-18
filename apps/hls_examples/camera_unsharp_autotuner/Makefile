#### Halide flags
HALIDE_BIN_PATH := ../../..
HALIDE_SRC_PATH := ../../..
include ../../support/Makefile.inc

#### HLS flags
include ../hls_support/Makefile.inc
HLS_LOG = vivado_hls.log

.PHONY: all run_hls
all: out.png
run_hls: $(HLS_LOG)

pipeline: pipeline.cpp
	$(CXX) $(CXXFLAGS) -Wall -g $^ $(LIB_HALIDE) -o $@ $(LDFLAGS) -ltinfo $(PNGFLAGS)

out.png: pipeline
	HL_ZYNQ_NUM_ACC=4 HL_ZYNQ_ACC_LAT=768 HL_NUM_THREADS=1 ./pipeline 3700 2.0 50  ../../images/bayer_raw.png


$(HLS_LOG): ../hls_support/run_hls.tcl pipeline_hls.cpp run.cpp
	RUN_PATH=$(realpath ./) \
	RUN_ARGS=$(realpath ./../../images/bayer_raw.png) \
	vivado_hls -f $< -l $(HLS_LOG)

clean:
	rm -f pipeline run out.png
	rm -f pipeline_native.h pipeline_native.o
	rm -f pipeline_hls.h pipeline_hls.cpp
	rm -f hls_target.h hls_target.cpp
