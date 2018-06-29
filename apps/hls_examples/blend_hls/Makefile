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
	$(CXX) $(CXXFLAGS) -Wall -g $^ $(LIB_HALIDE) -o $@ $(LDFLAGS) -ltinfo

pipeline_hls_seedark.cpp seedark_pipeline.o pipeline_zynq_seedark.c: pipeline
	HL_DEBUG_CODEGEN=0 ./pipeline

run: run.cpp pipeline_hls_seedark.cpp hls_target.cpp seedark_pipeline.o
	$(CXX) $(CXXFLAGS) -O1 -DNDEBUG $(HLS_CXXFLAGS) -g -Wall -Werror $^ -o $@ $(IMAGE_IO_FLAGS) $(LDFLAGS)

out.png: run
	./run ../../images/left0224.png ../../images/right0224.png

$(HLS_LOG): ../hls_support/run_hls.tcl pipeline_hls_seedark.cpp run.cpp
	RUN_PATH=$(realpath ./) \
	RUN_ARGS=$(realpath ../../images/left0224.png) $(realpath ../../images/right0224.png) \
	vivado_hls -f $< -l $(HLS_LOG)

pipeline_zynq.o: pipeline_zynq_seedark.c
	$(CXX) -c -O2 $(CXXFLAGS) -g -Wall -Werror $^ -o $@

run_zynq.o: run_zynq.cpp
	$(CXX) -c $(CXXFLAGS) -g -Wall -Werror $^ -o $@  $(PNGFLAGS)

HalideRuntimeZynq.o: ../hls_support/HalideRuntimeZynq.cpp
	$(CXX) -Wall -Werror -I ../../../src/runtime $^ -c -o $@

run_zynq: pipeline_zynq.o seedark_pipeline.o run_zynq.o HalideRuntimeZynq.o
	$(CXX) -Wall -Werror $^ -lpthread -ldl -o $@  $(IMAGE_IO_FLAGS)

out_zynq.png: run_zynq
	HL_NUM_THREADS=3 ./run_zynq  ../../images/left0224.png ../../images/right0224.png

clean:
	rm -f pipeline run run_zynq
	rm -f out.png out_zynq.png
	rm -f seedark_pipeline.h seedark_pipeline.o
	rm -f pipeline_hls_seedark.cpp pipeline_hls_seedark.h
	rm -f pipeline_zynq_seedark.c pipeline_zynq_seedark.h pipeline_zynq.o
	rm -f run_zynq.o
	rm -f hls_target.h hls_target.cpp
