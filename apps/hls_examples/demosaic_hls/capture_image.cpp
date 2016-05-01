/* Shuffle data through the combined VDMA/DMA pipeline, and stream it
 * over the network.
 *
 * Steven Bell <sebell@stanford.edu>
 * 12 November 2015
 */

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <time.h>

#include "pipeline_zynq.h"
//#include "pipeline_native.h"

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
static int halide_grab_image(int fd, kbuf_t* ptr) {
 return ioctl(fd, GRAB_IMAGE, (long unsigned int)ptr);
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

int main(int argc, char* argv[])
{
    // Open the hardware device
    int cma = open("/dev/cmabuffer0", O_RDWR);
    int xilcam = open("/dev/xilcam0", O_RDWR);
    int hwacc = open("/dev/hwacc0", O_RDWR);

    if(cma < 0 || xilcam < 0 || hwacc < 0){
        printf("Failed to open hardware devices: %s\n", strerror(errno));
        close(hwacc);
        close(xilcam);
        close(cma);
        return(-1);
    }


    kbuf_t raw_kbuf;

    int nImages = 1;

    printf("Waiting a second for images to roll in...\n");
    sleep(1);
    for(int i=0; i < nImages; i++) {
        // Grab an image from the camera
        int ok = halide_grab_image(xilcam, &raw_kbuf);
        if(ok < 0){
            printf("Failed to grab image: %s\n", strerror(errno));
            break;
        }

        // check raw image size
        if (raw_kbuf.width != 1920 ||  raw_kbuf.height != 1080 ||
            raw_kbuf.depth != 1 || raw_kbuf.stride != 2048) {
            printf("wrong raw image size: (%d, %d, %d, %d)\n", raw_kbuf.width,  raw_kbuf.height, raw_kbuf.depth, raw_kbuf.stride);
            halide_free_kbuf(cma, &raw_kbuf);
            break;
        }


        // prepare the input buffer duplet for accelerator
        buffer_t raw_buffer = {0};
        setup_buffer_duplet(&raw_buffer, &raw_kbuf, sizeof(uint8_t), cma);

        // copy raw image to a Halide Image object and save to file
        Image<uint8_t> raw_img(1920, 1080);
        for (int y = 0; y < raw_buffer.extent[1]; y++)
            for (int x = 0; x < raw_buffer.extent[0]; x++)
                raw_img(x, y) = raw_buffer.host[x*raw_buffer.stride[0] +
                                                y*raw_buffer.stride[1]];
        save_image(raw_img, "captured_raw.png");

        // prepare the output buffer duplet for accelerator
        kbuf_t output_kbuf;
        buffer_t output_buffer = {0};
        output_kbuf.width = 720;
        output_kbuf.height = 480;
        output_kbuf.depth = 3;
        output_kbuf.stride = 720;

        halide_alloc_kbuf(cma, &output_kbuf);
        setup_buffer_duplet(&output_buffer, &output_kbuf, sizeof(uint8_t), cma);

        // launch accelerator
        pipeline_zynq(&raw_buffer, &output_buffer, hwacc, cma);

        // copy output values to a Halide Image object and save to file
        Image<uint8_t> output_img(720, 480, 3);
        for (int y = 0; y < output_buffer.extent[2]; y++)
            for (int x = 0; x < output_buffer.extent[1]; x++)
                for (int c = 0; c < output_buffer.extent[0]; c++)
                    output_img(x, y, c) = output_buffer.host[c + x*output_buffer.stride[1] +
                                                             y*output_buffer.stride[2]];
        save_image(output_img, "captured_rgb.png");

        // Free the two working buffers
        free_buffer_duplet(&raw_buffer, &raw_kbuf, cma);
        free_buffer_duplet(&output_buffer , &output_kbuf, cma);
    }

    close(hwacc);
    close(xilcam);
    close(cma);
    return(0);
}

