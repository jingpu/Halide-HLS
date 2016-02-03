#### Halide flags
HALIDE_BIN_PATH := ../../..
HALIDE_SRC_PATH := ../../..
include ../../support/Makefile.inc

#### HLS flags
include ../hls_support/Makefile.inc

all: out.png

pipeline: pipeline.cpp
	$(CXX) $(CXXFLAGS) -Wall $^ $(LDFLAGS) $(LIB_HALIDE) -o $@ -lpthread -ldl -lz -ltinfo -g

pipeline_hls.cpp pipeline_native.o: pipeline
	HL_DEBUG_CODEGEN=0 ./pipeline

run: run.cpp pipeline_native.o pipeline_hls.cpp hls_target.cpp
	$(CXX) $(CXXFLAGS) $(HLS_CXXFLAGS) -g -Wall -Werror $^ -lpthread -ldl -o $@  $(PNGFLAGS)

out.png: run
	./run ../../images/left0224.png ../../images/left-remap.png ../../images/right0224.png ../../images/right-remap.png

pipeline_zynq.o: pipeline_zynq.c
	$(CXX) -c -O2 $(CXXFLAGS) -g -Wall -Werror $^ -o $@  $(PNGFLAGS)

run_zynq.o: run_zynq.cpp
	$(CXX) -c $(CXXFLAGS) -g -Wall -Werror $^ -o $@  $(PNGFLAGS)

run_zynq: run_zynq.o pipeline_zynq.o pipeline_native.o
	$(CXX) -Wall -Werror $^ -lpthread -ldl -o $@  $(PNGFLAGS)

out_zynq.png: run_zynq
	./run_zynq ../../images/left0224.png ../../images/left-remap.png ../../images/right0224.png ../../images/right-remap.png

clean:
	rm -f pipeline run out.png
	rm -f pipeline_native.h pipeline_native.o
	rm -f pipeline_hls.h pipeline_hls.cpp
	rm -f hls_target.h hls_target.cpp
