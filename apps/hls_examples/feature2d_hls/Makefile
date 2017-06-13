#### Halide flags
HALIDE_BIN_PATH := ../../..
HALIDE_SRC_PATH := ../../..
include ../../support/Makefile.inc

#### HLS flags
include ../hls_support/Makefile.inc

all: out.png

pipeline: pipeline.cpp
	$(CXX) $(CXXFLAGS) -Wall -g $^ $(LIB_HALIDE) -o $@ $(LDFLAGS) -ltinfo 

pipeline_jit: pipeline_jit.cpp
	$(CXX) -O3  $(CXXFLAGS) -Wall -g $^ $(LIB_HALIDE) -rdynamic  -o $@ $(LDFLAGS) -ltinfo $(PNGFLAGS)  `pkg-config --libs opencv`

pipeline_hls.cpp pipeline_native.o pipeline_cuda.o: pipeline
	HL_DEBUG_CODEGEN=0 ./pipeline

#run: run.cpp pipeline_hls.cpp hls_target.cpp pipeline_native.o
#	$(CXX) $(CXXFLAGS) -O1 -DNDEBUG $(HLS_CXXFLAGS) -g -Wall -Werror $^ -o $@ $(IMAGE_IO_FLAGS) $(LDFLAGS)

run: run.cpp pipeline_native.o pipeline_corners.o  
	$(CXX) $(CXXFLAGS) -O1 -DNDEBUG $(HLS_CXXFLAGS) -g -Wall -Werror $^ -o $@ $(IMAGE_IO_FLAGS) $(LDFLAGS) $(PNGFLAGS) `pkg-config --libs opencv`


run_cuda: pipeline_native.o pipeline_cuda.o run_cuda.cpp pipeline_corners.o
	$(CXX) -O3 $(CXXFLAGS) -Wall -Werror $^ -lpthread -ldl  -o $@  $(PNGFLAGS) `pkg-config --libs opencv`

run_integral: run_integral.cpp
	$(CXX) -O3 $(CXXFLAGS) -Wall -Werror $^ -lpthread -ldl -o $@  $(PNGFLAGS) `pkg-config --libs opencv`

gray_integral.png: run_integral
	./run_integral ../../images/gray_small.png

out.png: run
	./run ../../images/gray_small.png ../../images/gray_flip.png
	#./run ../../images/gray_small.png ../../images/gray_rotate90.png
	#./run ../../images/benchmark_1080p_gray.png ../../images/benchmark_1080p_gray_rotate90.png

out_cuda.png: run_cuda
	CUDA_LAUNCH_BLOCKING=1 HL_NUM_THREADS=4 ./run_cuda ../../images/benchmark_1080p_gray.png

clean:
	rm -f pipeline run corners.png
	rm -f pipeline_native.h pipeline_native.o
	rm -f pipeline_hls.h pipeline_hls.cpp
	rm -f hls_target.h hls_target.cpp
