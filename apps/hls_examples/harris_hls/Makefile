#### Halide flags
HALIDE_BIN_PATH := ../../..
HALIDE_SRC_PATH := ../../..
include ../../support/Makefile.inc

#### HLS flags
include ../hls_support/Makefile.inc

all: corners.png

pipeline: pipeline.cpp
	$(CXX) $(CXXFLAGS) -Wall $^ $(LDFLAGS) $(LIB_HALIDE) -o $@ -lpthread -ldl -lz -ltinfo -g

pipeline_hls.cpp pipeline_native.o: pipeline
	HL_DEBUG_CODEGEN=0 ./pipeline

run: run.cpp pipeline_native.o pipeline_hls.cpp hls_target.cpp
	$(CXX) $(CXXFLAGS) $(HLS_CXXFLAGS) -g -Wall -Werror $^  -lpthread -ldl -o $@  $(PNGFLAGS)

corners.png: run
	./run gray.png

clean:
	rm -f pipeline run corners.png
	rm -f pipeline_native.h pipeline_native.o
	rm -f pipeline_hls.h pipeline_hls.cpp
	rm -f hls_target.h hls_target.cpp
