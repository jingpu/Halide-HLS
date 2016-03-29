#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <math.h>

#include "pipeline_hls.h"
#include "pipeline_native.h"

#include "halide_image.h"
#include "halide_image_io.h"

using namespace Halide::Tools;


class my_load_image {
public:
    my_load_image(const std::string &f) : filename(f) {}
    template<typename ImageType>
    inline operator ImageType() {
        ImageType im;
        (void) load<ImageType, Internal::CheckFail>(filename, &im);
        // shuffle data
        ImageType res(im.channels(), im.width(), im.height());
        for(int x = 0; x < im.width(); x++)
            for(int y = 0; y < im.height(); y++)
                for(int c = 0; c < im.channels(); c++)
                    res(c, x, y) = im(x, y, c);
        return res;
    }
private:
  const std::string filename;
};


template<typename ImageType>
void my_save_image(ImageType &im, const std::string &filename) {
    int width = im.extent(1);
    int height = im.extent(2);
    int channels = im.extent(0);
    ImageType shuffled(width, height, channels);
    for(int x = 0; x < width; x++)
        for(int y = 0; y < height; y++)
            for(int c = 0; c < channels; c++)
                shuffled(x, y, c) = im(c, x, y);
    (void) save<ImageType, Internal::CheckFail>(shuffled, filename);
}


int main(int argc, char **argv) {

    if (argc < 3) {
        printf("Usage: ./run input.png output.png\n");
        return 0;
    }

    Image<uint8_t> input = my_load_image(argv[1]);
    Image<uint8_t> out_native(3, input.extent(1), input.extent(2));
    Image<uint8_t> out_hls(3, 512, 512);

    printf("start.\n");

    pipeline_native(input, out_native);
    my_save_image(out_native, argv[2]);

    printf("finish running native code\n");
    pipeline_hls(input, out_hls);

    printf("finish running HLS code\n");

    bool pass = true;
    for (int y = 0; y < out_hls.height(); y++) {
        for (int x = 0; x < out_hls.width(); x++) {
            for (int c = 0; c < out_hls.channels(); c++) {
                if (out_native(x, y, c) != out_hls(x, y, c)) {
                    printf("out_native(%d, %d, %d) = %d, but out_c(%d, %d, %d) = %d\n",
                           x, y, c, out_native(x, y, c),
                           x, y, c, out_hls(x, y, c));
                    pass = false;
                }
          }
	}
    }
    if (pass) {
        printf("passed.\n");
        return 0;
    } else  {
        printf("failed.\n");
        return 1;
    }
    return 0;
}
