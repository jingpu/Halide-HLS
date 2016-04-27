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

using namespace Halide::Tools;

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

int setup_buffer_duplet(buffer_t *b, kbuf_t *k, size_t elem_size, int cma) {
    if (k->depth < elem_size || k->depth % elem_size != 0) {
        printf("setup_buffer_duplet: wrong values: k->depth = %u, elem_size = %zu\n", k->depth, elem_size);
    }

    int channels = k->depth / elem_size;
    if (channels == 1) {
        // no channel in buffer_t, so dim 0 is image width, dim 1 is image height
        b->extent[0] = k->width;
        b->extent[1] = k->height;
        b->extent[2] = 0;
        b->extent[3] = 0;

        b->stride[0] = 1;
        b->stride[1] = k->stride;
        b->stride[2] = 0;
        b->stride[3] = 0;
    } else {
        // dim 0 is channel, dim 1 is image width, dim 2 is image height
        b->extent[0] = channels;
        b->extent[1] = k->width;
        b->extent[2] = k->height;
        b->extent[3] = 0;

        b->stride[0] = 1;
        b->stride[1] = channels;
        b->stride[2] = channels * k->stride;
        b->stride[3] = 0;
    }

    // other fields of buffer_t
    b->elem_size = elem_size;
    b->host_dirty = false;
    b->dev_dirty = false;

    b->dev = k->phys_addr;
    b->host = (uint8_t*) mmap(NULL, k->stride * k->height * k->depth,
                              PROT_WRITE, MAP_SHARED, cma, k->mmap_offset);
    return 0;
}


int free_buffer_duplet(buffer_t *b, kbuf_t *k, int cma) {
    munmap((void*)b->host, k->stride * k->height * k->depth);
    return halide_free_kbuf(cma, k);
}


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

    Image<uint8_t> input1 = load_image(argv[1]);
    Image<uint8_t> input2 = load_image(argv[2]);
    Image<uint8_t> out_native(720, 480, 3);
    Image<uint8_t> out_zynq_img(720, 480, 3);

    // prepare a pinned buffer holding inputs
    kbuf_t input1_kbuf;
    buffer_t input1_zynq = {0};
    input1_kbuf.width = 1920;
    input1_kbuf.height = 1080;
    input1_kbuf.depth = 1;
    input1_kbuf.stride = 2048;

    halide_alloc_kbuf(cma, &input1_kbuf);
    setup_buffer_duplet(&input1_zynq, &input1_kbuf, sizeof(uint8_t), cma);

    kbuf_t input2_kbuf;
    buffer_t input2_zynq = {0};
    input2_kbuf.width = 1920;
    input2_kbuf.height = 1080;
    input2_kbuf.depth = 1;
    input2_kbuf.stride = 2048;

    halide_alloc_kbuf(cma, &input2_kbuf);
    setup_buffer_duplet(&input2_zynq, &input2_kbuf, sizeof(uint8_t), cma);

    // fill data
    for (int y = 0; y < input1_zynq.extent[1]; y++)
        for (int x = 0; x < input1_zynq.extent[0]; x++)
            input1_zynq.host[x + y*input1_zynq.stride[1]] = input1(x, y);

    for (int y = 0; y < input2_zynq.extent[1]; y++)
        for (int x = 0; x < input2_zynq.extent[0]; x++)
            input2_zynq.host[x + y*input2_zynq.stride[1]] = input2(x, y);

    // prepare a pinned buffer holding outputs
    kbuf_t output_kbuf;
    buffer_t output_zynq = {0};
    output_kbuf.width = 720;
    output_kbuf.height = 480;
    output_kbuf.depth = 3;
    output_kbuf.stride = 720;

    halide_alloc_kbuf(cma, &output_kbuf);
    setup_buffer_duplet(&output_zynq, &output_kbuf, sizeof(uint8_t), cma);

    printf("start.\n");
    pipeline_native(input1, input2, out_native);
    save_image(out_native, "out_native.png");
    printf("cpu program results saved.\n");
    //out_native = load_image("out_native.png");
    //printf("cpu program results loaded.\n");

    pipeline_zynq(&input1_zynq, &input2_zynq, &output_zynq, hwacc, cma);

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
            pipeline_native(input1, input2, out_native);
        });
    printf("CPU program runtime: %g\n", min_t * 1e3);

    // Timing code. Timing doesn't include copying the input data to
    // the gpu or copying the output back.
    double min_t2 = benchmark(5, 20, [&]() {
            pipeline_zynq(&input1_zynq, &input2_zynq, &output_zynq, hwacc, cma);
        });
    printf("accelerator program runtime: %g\n", min_t2 * 1e3);


    // free pinned buffers
    free_buffer_duplet(&input1_zynq, &input1_kbuf, cma);
    free_buffer_duplet(&input2_zynq, &input2_kbuf, cma);
    free_buffer_duplet(&output_zynq, &output_kbuf, cma);

    close(hwacc);
    close(cma);
    return 0;
}
