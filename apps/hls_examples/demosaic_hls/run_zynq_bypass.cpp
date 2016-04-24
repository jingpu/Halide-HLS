#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <unistd.h>
#include "pipeline_zynq.h"
#include "pipeline_native.h"

#include "benchmark.h"
#include "halide_image.h"
#include "halide_image_io.h"


#ifndef KBUF_T_DEFINED
#define KBUF_T_DEFINED
typedef struct kbuf_t{
 unsigned int id; // ID flag for internal use
 unsigned int width; // Width of the image
 unsigned int stride; // Stride between rows, in pixels. This must be >= width
 unsigned int height; // Height of the image
 unsigned int depth; // Byte-depth of the image
 unsigned int phys_addr; // Bus address for DMA
 void* kern_addr; // Kernel virtual address
 struct mMap* cvals;
 unsigned int mmap_offset;
} kbuf_t;
#endif
#ifndef HALIDE_ATTRIBUTE_ALIGN
  #ifdef _MSC_VER
    #define HALIDE_ATTRIBUTE_ALIGN(x) __declspec(align(x))
  #else
    #define HALIDE_ATTRIBUTE_ALIGN(x) __attribute__((aligned(x)))
  #endif
#endif
#ifndef BUFFER_T_DEFINED
#define BUFFER_T_DEFINED
#include <stdbool.h>
#include <stdint.h>
typedef struct buffer_t {
    uint64_t dev;
    uint8_t* host;
    int32_t extent[4];
    int32_t stride[4];
    int32_t min[4];
    int32_t elem_size;
    HALIDE_ATTRIBUTE_ALIGN(1) bool host_dirty;
    HALIDE_ATTRIBUTE_ALIGN(1) bool dev_dirty;
    HALIDE_ATTRIBUTE_ALIGN(1) uint8_t _padding[10 - sizeof(void *)];
} buffer_t;
#endif

#ifndef _IOCTL_CMDS_H_
#define _IOCTL_CMDS_H_
#define GET_BUFFER 1000 // Get an unused buffer
#define GRAB_IMAGE 1001 // Acquire image from camera
#define FREE_IMAGE 1002 // Release buffer
#define PROCESS_IMAGE 1003 // Push to stencil path
#define PEND_PROCESSED 1004 // Retreive from stencil path
#endif
static int halide_alloc_kbuf(int fd, kbuf_t* ptr) {
 return ioctl(fd, GET_BUFFER, (long unsigned int)ptr);
}
static int halide_free_kbuf(int fd, kbuf_t* ptr) {
 return ioctl(fd, FREE_IMAGE, (long unsigned int)ptr);
}

using namespace Halide::Tools;

int main(int argc, char **argv) {
    // Open the buffer allocation device
    int cma = open("/dev/cmabuffer0", O_RDWR);
    if(cma == -1){
        printf("Failed to open cma provider!\n");
        return(0);
    }

    // open the hardware
    int hwacc = open("/dev/hwacc0", O_RDWR);
    if(hwacc == -1) {
        printf("Failed to open hardware device!\n");
        return(0);
    }

    Image<uint8_t> input = load_image(argv[1]);
    fprintf(stderr, "%d %d\n", input.width(), input.height());
    Image<uint8_t> out_native(720, 480, 3);
    Image<uint8_t> out_zynq_img(720, 480, 3);

    // prepare a pinned buffer holding inputs
    kbuf_t input_kbuf;
    buffer_t input_zynq = {0};
    input_kbuf.width = 1920;
    input_kbuf.height = 1080;
    input_kbuf.depth = 1;
    input_kbuf.stride = 2048;
    int ok_1 = halide_alloc_kbuf(cma, &input_kbuf);
    if (ok_1 < 0) {
        printf("Failed to allocate kernel buffer input_kbuf.\n");
    }

    // setup buffer_t
    input_zynq.extent[0] = 1920;
    input_zynq.extent[1] = 1080;
    input_zynq.extent[2] = 0;
    input_zynq.extent[3] = 0;

    input_zynq.stride[0] = 1;
    input_zynq.stride[1] = 2048;
    input_zynq.stride[2] = 0;
    input_zynq.stride[3] = 0;
    input_zynq.elem_size = sizeof(uint8_t);
    input_zynq.host_dirty = false;
    input_zynq.dev_dirty = false;

    input_zynq.dev = input_kbuf.phys_addr;
    input_zynq.host = (uint8_t*) mmap(NULL, input_kbuf.stride * input_kbuf.height * input_kbuf.depth,
                                      PROT_WRITE, MAP_SHARED, cma, input_kbuf.mmap_offset);

    // fill data
    for (int y = 0; y < input_zynq.extent[1]; y++)
        for (int x = 0; x < input_zynq.extent[0]; x++)
            input_zynq.host[x + y*input_zynq.stride[1]] = input(x, y);


    // prepare a pinned buffer holding outputs
    kbuf_t output_kbuf;
    buffer_t output_zynq = {0};
    output_kbuf.width = 720;
    output_kbuf.height = 480;
    output_kbuf.depth = 3;
    output_kbuf.stride = 720;
    int ok_2 = halide_alloc_kbuf(cma, &output_kbuf);
    if (ok_2 < 0) {
        printf("Failed to allocate kernel buffer output_kbuf.\n");
    }

    // setup buffer_t
    output_zynq.extent[0] = 3;
    output_zynq.extent[1] = 720;
    output_zynq.extent[2] = 480;
    output_zynq.extent[3] = 0;

    output_zynq.stride[0] = 1;
    output_zynq.stride[1] = 3;
    output_zynq.stride[2] = 3*720;
    output_zynq.stride[3] = 0;
    output_zynq.elem_size = sizeof(uint8_t);
    output_zynq.host_dirty = false;
    output_zynq.dev_dirty = false;

    output_zynq.dev = output_kbuf.phys_addr;
    output_zynq.host = (uint8_t*) mmap(NULL, output_kbuf.stride * output_kbuf.height * output_kbuf.depth,
                                       PROT_WRITE, MAP_SHARED, cma, output_kbuf.mmap_offset);

    printf("start.\n");
    pipeline_native(input, out_native);
    save_image(out_native, "out_native.png");
    printf("cpu program results saved.\n");
    //out_native = load_image("out_native.png");
    //printf("cpu program results loaded.\n");

    pipeline_zynq(&input_zynq, &output_zynq, hwacc, cma);

    // copy output values
    for (int y = 0; y < output_zynq.extent[2]; y++)
        for (int x = 0; x < output_zynq.extent[1]; x++)
            for (int c = 0; c < output_zynq.extent[0]; c++)
                out_zynq_img(x, y, c) = output_zynq.host[c + x*output_zynq.stride[1] +
                                                         y*output_zynq.stride[2]];

    save_image(out_zynq_img, "out_zynq_bypass.png");
    printf("accelerator program results saved.\n");

    printf("checking results...\n");

    unsigned fails = 0;
    for (int y = 0; y < out_zynq_img.height(); y++) {
        for (int x = 0; x < out_zynq_img.width(); x++) {
            for (int c = 0; c < 3; c++) {
                if (out_native(x, y, c) != out_zynq_img(x, y, c)) {
                    printf("out_native(%d, %d, %d) = %d, but out_zynq_img(%d, %d, %d) = %d\n",
                           x, y, c, out_native(x, y, c),
                           x, y, c, out_zynq_img(x, y, c));
                    fails++;
                }
            }
	}
    }
    if (!fails) {
        printf("passed.\n");
    } else  {
        printf("%u fails.\n", fails);
    }

    printf("\nstart timing code...\n");
    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t = benchmark(1, 10, [&]() {
            pipeline_native(input, out_native);
        });
    printf("CPU program runtime: %g\n", min_t * 1e3);

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t2 = benchmark(5, 20, [&]() {
            pipeline_zynq(&input_zynq, &output_zynq, hwacc, cma);
        });
    printf("accelerator program runtime: %g\n", min_t2 * 1e3);


    // free pinned buffers
    munmap((void*)input_zynq.host, input_kbuf.stride * input_kbuf.height * input_kbuf.depth);
    halide_free_kbuf(cma, &input_kbuf);
    munmap((void*)output_zynq.host, output_kbuf.stride * output_kbuf.height * output_kbuf.depth);
    halide_free_kbuf(cma, &output_kbuf);

    close(hwacc);
    close(cma);
    return 0;
}
