#### Halide flags
HALIDE_BIN_PATH := ../../..
HALIDE_SRC_PATH := ../../..
include ../../support/Makefile.inc

#### HLS flags
include ../hls_support/Makefile.inc

all: sum.png

addition: addition.cpp
	$(CXX) $(CXXFLAGS) -Wall $^ $(LDFLAGS) $(LIB_HALIDE) -o $@ -lpthread -lpng -ldl -lz -ltinfo -g

pipeline_hls.cpp pipeline_native.o: addition
	HL_DEBUG_CODEGEN=0 ./addition

run: run.cpp pipeline_native.o pipeline_hls.cpp hls_target.cpp
	$(CXX) $(CXXFLAGS) $(HLS_CXXFLAGS) -g -Wall -Werror $^  -lpthread -ldl -o $@  $(PNGFLAGS)

demosaic.png: run
	./run a.png b.png

clean:
	rm -f addition run out.png demosaic.png
	rm -f pipeline_native.h pipeline_native.o
	rm -f pipeline_hls.h pipeline_hls.cpp
	rm -f hls_target.h hls_target.cpp
