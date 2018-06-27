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

pipeline_hls.cpp pipeline_native.o pipeline_zynq.o: pipeline
	HL_DEBUG_CODEGEN=0 ./pipeline

run: run.cpp pipeline_native.o pipeline_hls.cpp hls_target.cpp
	$(CXX) $(CXXFLAGS) -O1 -DNDEBUG $(HLS_CXXFLAGS) -g -Wall -Werror $^ -lpthread -ldl $(PNGFLAGS) -o $@ $(IMAGE_IO_FLAGS) $(LDFLAGS)


out.png: run
	./run ../../images/left0224.png ../../images/left-remap.png ../../images/right0224.png ../../images/right-remap.png

$(HLS_LOG): ../hls_support/run_hls.tcl pipeline_hls.cpp run.cpp
	RUN_PATH=$(realpath ./) \
	RUN_ARGS="$(realpath ./../../images/left0224.png) $(realpath ./../../images/left-remap.png) $(realpath ./../../images/right0224.png) $(realpath ./../../images/right-remap.png)" \
	vivado_hls -f $< -l $(HLS_LOG)

#pipeline_zynq.o: pipeline_zynq.c
#	$(CXX) -c -O2 $(CXXFLAGS) -g -Wall -Werror $^ -o $@  $(PNGFLAGS)

run_zynq: pipeline_zynq.o pipeline_native.o run_zynq.cpp
	$(CXX) -O3 $(CXXFLAGS) $(HLS_IFLAG) -Wall -Werror $^ -lpthread -ldl -o $@  $(IMAGE_IO_FLAGS)

run_cuda: pipeline_native.o pipeline_cuda.o run_cuda.cpp
	$(CXX) -O3 $(CXXFLAGS) -Wall -Werror $^ -lpthread -ldl -o $@  $(PNGFLAGS) `pkg-config --libs opencv`

out_zynq.png: run_zynq
	#HL_NUM_THREADS=3 ./run_zynq ../../images/left0224.png ../../images/left-remap.png ../../images/right0224.png ../../images/right-remap.png
	HL_NUM_THREADS=3 ./run_zynq ../../images/benchmark_8mp_rgb.png ../../images/benchmark_8mp_rgb.png ../../images/benchmark_8mp_rgb.png ../../images/benchmark_8mp_rgb.png

out_cuda.png: run_cuda
	CUDA_LAUNCH_BLOCKING=1 HL_NUM_THREADS=4 ./run_cuda ../../images/benchmark_8mp_rgb.png ../../images/benchmark_8mp_rgb.png ../../images/benchmark_8mp_rgb.png ../../images/benchmark_8mp_rgb.png

clean:
	rm -f pipeline run run_zynq run_cuda
	rm -f pipeline_native.h pipeline_native.o
	rm -f pipeline_hls.h pipeline_hls.cpp
	rm -f hls_target.h hls_target.cpp
	rm -f *.png
	rm -f *.h
	rm -f *.o
	rm -f *.html
