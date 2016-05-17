#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>
#include "pipeline_native.h"
#include "pipeline_zynq.h"

#include "benchmark.h"
#include "halide_image.h"
#include "halide_image_io.h"


using namespace Halide::Tools;


extern "C" {
#ifndef CMA_BUFFER_T_DEFINED
#define CMA_BUFFER_T_DEFINED
struct mMap;
typedef struct cma_buffer_t {
  unsigned int id; // ID flag for internal use
  unsigned int width; // Width of the image
  unsigned int stride; // Stride between rows, in pixels. This must be >= width
  unsigned int height; // Height of the image
  unsigned int depth; // Byte-depth of the image
  unsigned int phys_addr; // Bus address for DMA
  void* kern_addr; // Kernel virtual address
  struct mMap* cvals;
  unsigned int mmap_offset;
} cma_buffer_t;
#endif
// Zynq runtime API
int halide_zynq_init();
void halide_zynq_free(void *user_context, void *ptr);
int halide_zynq_cma_alloc(struct buffer_t *buf);
int halide_zynq_cma_free(struct buffer_t *buf);
int halide_zynq_subimage(const struct buffer_t* image, struct cma_buffer_t* subimage, void *address_of_subimage_origin, int width, int height);
int halide_zynq_hwacc_launch(struct cma_buffer_t bufs[]);
int halide_zynq_hwacc_sync(int task_id);
}

int main(int argc, char **argv) {
    halide_zynq_init();

    Image<uint8_t> input = load_image(argv[1]);
    fprintf(stderr, "%d %d\n", input.width(), input.height());
    Image<uint8_t> out_native(720, 480, 3);
    Image<uint8_t> out_zynq_img(720, 480, 3);

    // prepare a pinned buffer holding inputs
    buffer_t input_zynq = {0};
    input_zynq.elem_size = 1;
    input_zynq.extent[0] = 1920;
    input_zynq.extent[1] = 1080;
    input_zynq.stride[0] = 1;
    input_zynq.stride[1] = 1920;
    halide_zynq_cma_alloc(&input_zynq);

    // fill data
    for (int y = 0; y < input_zynq.extent[1]; y++)
        for (int x = 0; x < input_zynq.extent[0]; x++)
            input_zynq.host[x + y*input_zynq.stride[1]] = input(x, y);


    // prepare a pinned buffer holding outputs
    buffer_t output_zynq = {0};
    output_zynq.elem_size = 1;
    output_zynq.extent[0] = 3;
    output_zynq.extent[1] = 720;
    output_zynq.extent[2] = 480;
    output_zynq.stride[0] = 1;
    output_zynq.stride[1] = 3;
    output_zynq.stride[2] = 2160;
    halide_zynq_cma_alloc(&output_zynq);

    printf("start.\n");
    pipeline_native(input, out_native);
    save_image(out_native, "out_native.png");
    printf("cpu program results saved.\n");
    //out_native = load_image("out_native.png");
    //printf("cpu program results loaded.\n");

    pipeline_zynq(&input_zynq, &output_zynq);

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
            pipeline_zynq(&input_zynq, &output_zynq);
        });
    printf("accelerator program runtime: %g\n", min_t2 * 1e3);


    // free pinned buffers
    halide_zynq_cma_free(&input_zynq);
    halide_zynq_cma_free(&output_zynq);
    return 0;
}
