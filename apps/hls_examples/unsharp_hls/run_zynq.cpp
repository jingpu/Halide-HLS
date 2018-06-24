#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include <fcntl.h>
#include <unistd.h>
#include "pipeline_zynq.h"
#include "pipeline_native.h"

#include "BufferMinimal.h"
#include "halide_image_io.h"

using namespace Halide::Tools;
using namespace Halide::Runtime::HLS;

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


    BufferMinimal<uint8_t> input = load_image(argv[1]);
    BufferMinimal<uint8_t> out_native(2400, 3200, 3);
    BufferMinimal<uint8_t> out_zynq(480*5, 640*5, 3);
    
    printf("start.\n");

    pipeline_native(input, out_native);
    save_image(out_native, "out_native.png");
    printf("cpu program results saved.\n");

    pipeline_zynq(input, out_zynq, hwacc, cma);
    save_image(out_zynq, "out_zynq.png");
    printf("accelerator program results saved.\n");

    printf("checking results...\n");
    unsigned fails = 0;
    for (int y = 0; y < out_zynq.height(); y++) {
        for (int x = 0; x < out_zynq.width(); x++) {
            for (int c = 0; c < out_zynq.channels(); c++) {
                if (out_native(x, y, c) != out_zynq(x, y, c)) {
                    printf("out_native(%d, %d, %d) = %d, but out_c(%d, %d, %d) = %d\n",
                           x, y, c, out_native(x, y, c),
                           x, y, c, out_zynq(x, y, c));
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

    close(hwacc);
    close(cma);
    return 0;
}
