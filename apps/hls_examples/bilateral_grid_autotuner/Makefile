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
	HL_ZYNQ_NUM_ACC=4 HL_ZYNQ_ACC_LAT=768 HL_NUM_THREADS=1 ./pipeline ../../images/benchmark_8mp_gray.png

run: run.cpp pipeline_hls.cpp hls_target.cpp pipeline_native.o
	$(CXX) $(CXXFLAGS) -O1 -DNDEBUG $(HLS_CXXFLAGS) -g -Wall -Werror $^ -lpthread -ldl $(PNGFLAGS) -o $@

$(HLS_LOG): ../hls_support/run_hls.tcl pipeline_hls.cpp run.cpp
	RUN_PATH=$(realpath ./) \
	RUN_ARGS=$(realpath ./../../images/benchmark_8mp_gray.png) \
	vivado_hls -f $< -l $(HLS_LOG)

clean:
	rm -f pipeline run run_zynq run_cuda
	rm -f pipeline_hls.cpp pipeline_zynq.c hls_target.cpp
	rm -f *.png
	rm -f *.h
	rm -f *.o
	rm -f *.html
