#ifndef STENCIL_H
#define STENCIL_H

#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <hls_stream.h>


using hls::stream;

/** multi-dimension (up-to 4 dimensions) stencil struct
 */
// TODO support 4 dimensions
template <typename T, size_t EXTENT_0, size_t EXTENT_1 = 1, size_t EXTENT_2 = 1, size_t EXTENT_3 = 1>
struct Stencil{
public:
    T value[EXTENT_3][EXTENT_2][EXTENT_1][EXTENT_0];

    /** writer
     */
    inline T& operator()(size_t index_0, size_t index_1 = 0, size_t index_2 = 0, size_t index_3 = 0) {
#pragma HLS INLINE
	assert(index_0 < EXTENT_0 && index_1 < EXTENT_1 && index_2 < EXTENT_2 && index_3 < EXTENT_3);
	return value[index_3][index_2][index_1][index_0];
    }

    /** reader
     */
    inline const T& operator()(size_t index_0, size_t index_1 = 0, size_t index_2 = 0, size_t index_3 = 0) const {
#pragma HLS INLINE
	assert(index_0 < EXTENT_0 && index_1 < EXTENT_1 && index_2 < EXTENT_2 && index_3 < EXTENT_3);
	return value[index_3][index_2][index_1][index_0];
    }

    template <size_t ST_EXTENT_0, size_t ST_EXTENT_1 = 1, size_t ST_EXTENT_2 = 1, size_t ST_EXTENT_3 = 1>
    void write_stencil_at(const Stencil<T, ST_EXTENT_0, ST_EXTENT_1, ST_EXTENT_2, ST_EXTENT_3> &stencil,
			  size_t index_0, size_t index_1 = 0, size_t index_2 = 0, size_t index_3 = 0) {
#pragma HLS INLINE
	assert(index_0 + ST_EXTENT_0 - 1 < EXTENT_0 &&
	       index_1 + ST_EXTENT_1 - 1 < EXTENT_1 &&
	       index_2 + ST_EXTENT_2 - 1 < EXTENT_2 &&
	       index_3 + ST_EXTENT_3 - 1 < EXTENT_3);
	for(size_t st_idx_3 = 0; st_idx_3 < ST_EXTENT_3; st_idx_3++)
#pragma HLS UNROLL
	for(size_t st_idx_2 = 0; st_idx_2 < ST_EXTENT_2; st_idx_2++)
#pragma HLS UNROLL
        for(size_t st_idx_1 = 0; st_idx_1 < ST_EXTENT_1; st_idx_1++)
#pragma HLS UNROLL
        for(size_t st_idx_0 = 0; st_idx_0 < ST_EXTENT_0; st_idx_0++)
#pragma HLS UNROLL
            value[index_3+st_idx_3][index_2+st_idx_2][index_1+st_idx_1][index_0+st_idx_0] = stencil(st_idx_0, st_idx_1, st_idx_2, st_idx_3);
    }


    template <size_t ST_EXTENT_0, size_t ST_EXTENT_1 = 1, size_t ST_EXTENT_2 = 1, size_t ST_EXTENT_3 = 1>
    void read_stencil_at(Stencil<T, ST_EXTENT_0, ST_EXTENT_1, ST_EXTENT_2, ST_EXTENT_3> &stencil,
			 size_t index_0, size_t index_1 = 0, size_t index_2 = 0, size_t index_3 = 0) const {
#pragma HLS INLINE
	assert(index_0 + ST_EXTENT_0 - 1 < EXTENT_0 &&
	       index_1 + ST_EXTENT_1 - 1 < EXTENT_1 &&
	       index_2 + ST_EXTENT_2 - 1 < EXTENT_2 &&
	       index_3 + ST_EXTENT_3 - 1 < EXTENT_3);
	for(size_t st_idx_3 = 0; st_idx_3 < ST_EXTENT_3; st_idx_3++)
#pragma HLS UNROLL
	for(size_t st_idx_2 = 0; st_idx_2 < ST_EXTENT_2; st_idx_2++)
#pragma HLS UNROLL
        for(size_t st_idx_1 = 0; st_idx_1 < ST_EXTENT_1; st_idx_1++)
#pragma HLS UNROLL
	for(size_t st_idx_0 = 0; st_idx_0 < ST_EXTENT_0; st_idx_0++)
#pragma HLS UNROLL
            stencil(st_idx_0, st_idx_1, st_idx_2, st_idx_3) = value[index_3+st_idx_3][index_2+st_idx_2][index_1+st_idx_1][index_0+st_idx_0];
    }
};



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


template <typename T, size_t EXTENT_0, size_t EXTENT_1, size_t EXTENT_2, size_t EXTENT_3>
void buffer_to_stencil(const buffer_t *buffer,
                       Stencil<T, EXTENT_0, EXTENT_1, EXTENT_2, EXTENT_3> &stencil) {
    assert(EXTENT_0 == buffer->extent[0]);
    assert((EXTENT_1 == buffer->extent[1]) || (EXTENT_1 == 1 && buffer->extent[1] == 0));
    assert((EXTENT_2 == buffer->extent[2]) || (EXTENT_2 == 1 && buffer->extent[2] == 0));
    assert((EXTENT_3 == buffer->extent[3]) || (EXTENT_3 == 1 && buffer->extent[3] == 0));
    assert(sizeof(T) == buffer->elem_size);

    for(size_t idx_3 = 0; idx_3 < EXTENT_3; idx_3++)
    for(size_t idx_2 = 0; idx_2 < EXTENT_2; idx_2++)
    for(size_t idx_1 = 0; idx_1 < EXTENT_1; idx_1++)
    for(size_t idx_0 = 0; idx_0 < EXTENT_0; idx_0++) {
        const uint8_t *ptr = buffer->host;
        size_t offset = idx_0 * buffer->stride[0] +
            idx_1 * buffer->stride[1] +
            idx_2 * buffer->stride[2] +
            idx_3 * buffer->stride[3];
        const T *address =  (T *)(ptr + offset * buffer->elem_size);
        stencil(idx_0, idx_1, idx_2, idx_3) = *address;
    }
}

#endif
