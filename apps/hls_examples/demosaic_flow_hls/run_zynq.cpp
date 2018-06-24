#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include <fcntl.h>
#include <unistd.h>
#include "pipeline_zynq.h"
#include "pipeline_native.h"

#include "HalideBuffer.h"
#include "halide_image_io.h"
#include "HalideRuntime.h"

using namespace Halide::Tools;
using namespace Halide::Runtime;

extern "C" int halide_zynq_init();

int main(int argc, char **argv) {
    int fd_cma, fd_hwacc;

    fd_cma = open("/dev/cmabuffer0", O_RDWR, 0644);
      if(fd_cma == -1) {
         printf("Failed to open cma provider!\n");
         fd_cma = fd_hwacc = 0;
         return -2;
    }
    fd_hwacc = open("/dev/hwacc0", O_RDWR, 0644);
    if(fd_hwacc == -1) {
       printf("Failed to open hwacc device!\n");
       close(fd_cma);
       fd_cma = fd_hwacc = 0;
       return -2;
    }

    Buffer<uint8_t> input1 = load_image(argv[1]);
    Buffer<uint8_t> input2 = load_image(argv[2]);
    Buffer<uint8_t> out_native(720, 480, 3);
    Buffer<uint8_t> out_zynq(720, 480, 3);

    printf("start.\n");

    pipeline_native(input1, input2, out_native);
    save_image(out_native, "out_native.png");
    printf("cpu program results saved.\n");

    pipeline_zynq(input1, input2, out_zynq, fd_hwacc, fd_cma);
    save_image(out_zynq, "out_zynq.png");
    printf("accelerator program results saved.\n");

    printf("checking results...\n");

    unsigned fails = 0;
    for (int y = 0; y < out_zynq.height(); y++) {
        for (int x = 0; x < out_zynq.width(); x++) {
            for (int c = 0; c < out_zynq.channels(); c++) {
                if (out_native(x, y, c) != out_zynq(x, y, c)) {
                    printf("out_native(%d, %d, %d) = %d, but out_zynq(%d, %d, %d) = %d\n",
                           x, y, c, out_native(x, y, c),
                           x, y, c, out_zynq(x, y, c));
                    fails++;
                    goto test;
                }
            }
	}
    }
test:
    if (!fails) {
        printf("passed.\n");
    } else  {
        printf("%u fails.\n", fails);
    }

    return 0;
}
