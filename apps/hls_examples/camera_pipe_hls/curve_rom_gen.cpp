#include <stdio.h>
#include <stdint.h>
#include "halide_math.h"

void compute_curve(uint8_t _curve_2[65536]) {
    for (int _curve_2_s0_x = 0; _curve_2_s0_x < 65536; _curve_2_s0_x++)
        {
            //int32_t _609 = _curve_2_s0_x + 32768;
            float _610 = (float)(_curve_2_s0_x - 32768);
            float _611 = _610 * float_from_bits(981467136 /* 0.000976562 */);
            float _612 = min(_611, float_from_bits(1065353216 /* 1 */));
            float _613 = max(_612, float_from_bits(0 /* 0 */));
            float _614 = pow_f32(_613, float_from_bits(1056964608 /* 0.5 */));
            float _615 = float_from_bits(1065353216 /* 1 */) - _614;
            float _616 = _615 * float_from_bits(1062474700 /* 0.828427 */);
            float _617 = _616 + float_from_bits(1058403866 /* 0.585786 */);
            float _618 = _615 * _617;
            float _619 = float_from_bits(1065353216 /* 1 */) - _618;
            float _620 = _614 * float_from_bits(1062474700 /* 0.828427 */);
            float _621 = _620 + float_from_bits(1058403866 /* 0.585786 */);
            float _622 = _614 * _621;
            bool _623 = float_from_bits(1056964608 /* 0.5 */) < _614;
            float _624 = (float)(_623 ? _619 : _622);
            float _625 = _624 * float_from_bits(1132462080 /* 256 */);
            float _626 = min(_625, float_from_bits(1132396544 /* 255 */));
            float _627 = max(_626, float_from_bits(0 /* 0 */));
            uint8_t _628 = (uint8_t)(_627);
            _curve_2[_curve_2_s0_x] = _628;
        } // for _curve_2_s0_x
}


int main(int argc, char **argv) {
    uint8_t data[65536];
    compute_curve(data);

    FILE *fp = fopen("curve_rom.h","w");

    fprintf(fp, "const static uint8_t _curve_2[65536] = {\n");
    for (int i = 0; i < 65536/16; i++) {
        for (int j = 0; j < 16; j++) {
            fprintf(fp, " %u,", data[j + i*16]);
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "};\n");
    fclose(fp);
    return 0;
}
