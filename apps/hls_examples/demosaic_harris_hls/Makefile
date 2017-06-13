#### Halide flags
HALIDE_BIN_PATH := ../../..
HALIDE_SRC_PATH := ../../..
include ../../support/Makefile.inc

#### HLS flags
include ../hls_support/Makefile.inc
HLS_LOG = vivado_hls.log

.PHONY: all run_hls
all: run
run_hls: $(HLS_LOG)

pipeline: pipeline.cpp
	$(CXX) $(CXXFLAGS) -Wall -g $^ $(LIB_HALIDE) -o $@ $(LDFLAGS) -ltinfo

pipeline_hls.cpp pipeline_native.o pipeline_zynq.o: pipeline
	HL_DEBUG_CODEGEN=0 ./pipeline

run: run.cpp pipeline_hls.cpp hls_target.cpp pipeline_native.o
	$(CXX) $(CXXFLAGS) -O1 -DNDEBUG $(HLS_CXXFLAGS) -g -Wall -Werror $^ -o $@ $(IMAGE_IO_FLAGS) $(LDFLAGS)

run_downsample: run.cpp pipeline_hls_downsample.cpp hls_target_downsample.cpp pipeline_native.o
	$(CXX) $(CXXFLAGS) -O1 -DNDEBUG $(HLS_CXXFLAGS) -g -Wall -Werror $^ -o $@ $(IMAGE_IO_FLAGS) $(LDFLAGS)

out.png: run
	./run ../../images/zynq_raw.png

out_downsample: run_downsample
	./run_downsample ../../images/zynq_raw.png

$(HLS_LOG): ../hls_support/run_hls.tcl pipeline_hls.cpp run.cpp
	RUN_PATH=$(realpath ./) \
	RUN_ARGS=$(realpath ./../../images/zynq_raw.png) \
	vivado_hls -f $< -l $(HLS_LOG)

#pipeline_zynq.o: pipeline_zynq.c
#	$(CXX) -c -O2 $(CXXFLAGS) -g -Wall -Werror $^ -o $@

pipeline_zynq_bypass.o: pipeline_zynq_bypass.c
	$(CXX) -c -O2 $(CXXFLAGS) -g -Wall -Werror $^ -o $@

HalideRuntimeZynq.o: ../hls_support/HalideRuntimeZynq.cpp
	$(CXX) -c -O2 $(CXXFLAGS) -g -Wall -Werror $^ -o $@

run_zynq: run_zynq.cpp pipeline_zynq.o pipeline_native.o  #HalideRuntimeZynq.o
	$(CXX) $(CXXFLAGS) -Wall -Werror $^ -lpthread -ldl -o $@  $(PNGFLAGS)

run_zynq_bypass: run_zynq_bypass.cpp pipeline_native.o pipeline_zynq_bypass.o HalideRuntimeZynq.o
	$(CXX) $(CXXFLAGS) -Wall -Werror $^ -lpthread -ldl -o $@  $(PNGFLAGS)

out_zynq.png: run_zynq
	HL_NUM_THREADS=3 ./run_zynq ../../images/zynq_raw.png

out_zynq_bypass.png: run_zynq_bypass
	./run_zynq_bypass ../../images/zynq_raw.png

clean:
	rm -f pipeline run out.png demosaic.png
	rm -f pipeline_native.h pipeline_native.o
	rm -f pipeline_hls.h pipeline_hls.cpp
	rm -f hls_target.h hls_target.cpp
