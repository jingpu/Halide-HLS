/** \file
 * Defines a BufferMinimal type that wraps from buffer_t and adds
 * minimal functionality, and methods for more conveniently
 * iterating over the samples in a halide_buffer_t outside of
 * Halide code. */


#ifndef HALIDE_RUNTIME_HLS_BUFFER_MINIMAL_H
#define HALIDE_RUNTIME_HLS_BUFFER_MINIMAL_H

#include <stdint.h>
#include <string.h>

#include "HalideRuntime.h"

namespace Halide {
namespace Runtime {
namespace HLS {

/** A templated Buffer class that wraps halide_buffer_t and adds
 * minimal functionality. It provides a minimal set of methods
 * of Halide::Runtime::Buffer, in order to allow to be built with
 * legacy C++ compiler without the support of C++11 feature.
 */
template<typename T, int D = 3>
class BufferMinimal {
    /** The underlying buffer_t */
    halide_buffer_t buf = {0};

    /** Some in-class storage for shape of the dimensions. */
    halide_dimension_t shape[D];

    void free_shape_storage() {
        if (buf.dim != shape) {
            delete[] buf.dim;
            buf.dim = nullptr;
        }
    }

    void make_shape_storage() {
        if (buf.dimensions <= D) {
            buf.dim = shape;
        } else {
            buf.dim = new halide_dimension_t[buf.dimensions];
        }
    }

    void copy_shape_from(const halide_buffer_t &other) {
        // All callers of this ensure that buf.dimensions == other.dimensions.
        make_shape_storage();
        for (int i = 0; i < buf.dimensions; i++) {
            buf.dim[i] = other.dim[i];
        }
    }

    int get_dimension_from_args(int first, int second, int third) {
        if (second == 0) {
            return 1;
        } else if (third == 0) {
            return 2;
        } else {
            return 3;
        }
    }

    void initialize_shape(int first, int second, int third) {
        int extents[3] = {first, second, third};
        for (int i = 0; i < buf.dimensions; i++) {
            buf.dim[i].min = 0;
            buf.dim[i].extent = extents[i];
            if (i == 0) {
                buf.dim[i].stride = 1;
            } else {
                buf.dim[i].stride = buf.dim[i-1].stride * buf.dim[i-1].extent;
            }
        }
    }

    void allocate() {
        // Conservatively align images to 128 bytes. This is enough
        // alignment for all the platforms we might use.
        size_t size = size_in_bytes();
        const size_t alignment = 128;
        size = (size + alignment - 1) & ~(alignment - 1);
        uint8_t *unaligned_ptr = (uint8_t *)malloc(size + alignment - 1);
        buf.host = (uint8_t *)((size_t)(unaligned_ptr + alignment - 1) & ~(alignment - 1));
    }

    void deallocate() {
        // Memory Leak here.
    }

    size_t offset_of(int first, int second, int third) {
        int indices[3] = {first, second, third};
        size_t offset = 0;
        for (int i = 0; i < buf.dimensions; i++) {
            offset += buf.dim[i].stride * (indices[i] - buf.dim[i].min);
        }
        return offset;
    }

public:
    /** Get the Halide type of T. Callers should not use the result if
     * has_static_halide_type() returns false. */
    static halide_type_t static_halide_type() {
        return halide_type_of<T>();
    }

    BufferMinimal() {
        memset(&buf, 0, sizeof(halide_buffer_t));
        buf.type = static_halide_type();
        make_shape_storage();
    }

    /** Allocate a new image of the given size. Pass zeroes to make a
     * buffer suitable for bounds query calls. */
    BufferMinimal(int first, int second = 0, int third = 0) {
        buf.type = static_halide_type();
        buf.dimensions = get_dimension_from_args(first, second, third);
        this->make_shape_storage();
        this->initialize_shape(first, second, third);
        allocate();
    }

    /** Destructor. Will release any underlying owned allocation if
     * this is the last reference to it. Will assert fail if there are
     * weak references to this Buffer outstanding. */
    ~BufferMinimal() {
        free_shape_storage();
        deallocate();
    }

    /** Copy constructor. Does not copy underlying data. */
    BufferMinimal(const BufferMinimal<T, D> &other) : buf(other.buf) {
        //other.incref();
        //dev_ref_count = other.dev_ref_count;
        copy_shape_from(other.buf);
    }
    /** Standard assignment operator */
    BufferMinimal<T, D> &operator=(const BufferMinimal<T, D> &other) {
        if (this == &other) {
            return *this;
        }
        //other.incref();
        //decref();
        //dev_ref_count = other.dev_ref_count;
        //alloc = other.alloc;
        free_shape_storage();
        buf = other.buf;
        copy_shape_from(other.buf);
        return *this;
    }

    /** Get a pointer to the address of the min coordinate. */
    // @{
    T *data() {
        return (T *)(this->buf.host);
    }

    const T *data() const {
        return (const T *)(this->buf.host);
    }
    // @}

    /** Getter and setter for element in the buffer. */
    const T &operator()(int first, int second = 0, int third = 0) const {
        return *(((T *)buf.host) + offset_of(first, second, third));
    }

    T &operator()(int first, int second = 0, int third = 0) {
        return *(((T *)buf.host) + offset_of(first, second, third));
    }

    /** Get the dimensionality of the buffer. */
    int dimensions() const {
        return buf.dimensions;
    }

    /** Get the type of the elements. */
    halide_type_t type() const {
        return buf.type;
    }

    typedef T ElemType;

    /** Read-only access to the shape */
    class Dimension {
        const halide_dimension_t &d;
    public:
        /** The lowest coordinate in this dimension */
        inline int min() const {
            return d.min;
        }

        /** The number of elements in memory you have to step over to
         * increment this coordinate by one. */
        inline int stride() const {
            return d.stride;
        }

        /** The extent of the image along this dimension */
        inline int extent() const {
            return d.extent;
        }

        /** The highest coordinate in this dimension */
        inline int max() const {
            return min() + extent() - 1;
        }

        /** An iterator class, so that you can iterate over
         * coordinates in a dimensions using a range-based for loop. */
        struct iterator {
            int val;
            int operator*() const {return val;}
            bool operator!=(const iterator &other) const {return val != other.val;}
            iterator &operator++() {val++; return *this;}
        };

        /** An iterator that points to the min coordinate */
        inline iterator begin() const {
            return {min()};
        }

        /** An iterator that points to one past the max coordinate */
        inline iterator end() const {
            return {min() + extent()};
        }

        Dimension(const halide_dimension_t &dim) : d(dim) {};
    };

    /** Access the shape of the buffer */
    inline Dimension dim(int i) const {
        return Dimension(buf.dim[i]);
    }

    /** Conventional names for the first three dimensions. */
    int width() const {
        return (dimensions() > 0) ? dim(0).extent() : 1;
    }
    int height() const {
        return (dimensions() > 1) ? dim(1).extent() : 1;
    }
    int channels() const {
        return (dimensions() > 2) ? dim(2).extent() : 1;
    }

    /** A pointer to the element with the lowest address. If all
     * strides are positive, equal to the host pointer. */
    T *begin() const {
        ptrdiff_t index = 0;
        for (int i = 0; i < dimensions(); i++) {
            if (dim(i).stride() < 0) {
                index += dim(i).stride() * (dim(i).extent() - 1);
            }
        }
        return (T *)(buf.host + index * type().bytes());
    }

    /** A pointer to one beyond the element with the highest address. */
    T *end() const {
        ptrdiff_t index = 0;
        for (int i = 0; i < dimensions(); i++) {
            if (dim(i).stride() > 0) {
                index += dim(i).stride() * (dim(i).extent() - 1);
            }
        }
        index += 1;
        return (T *)(buf.host + index * type().bytes());
    }

    /** The total number of bytes spanned by the data in memory. */
    size_t size_in_bytes() const {
        return (size_t)((const uint8_t *)end() - (const uint8_t *)begin());
    }

    /** Get a pointer to the raw buffer_t this wraps. */
    // @{
    halide_buffer_t *raw_buffer() {
        return &buf;
    }

    const halide_buffer_t *raw_buffer() const {
        return &buf;
    }
    // @}

    /** Provide a cast operator to halide_buffer_t *, so that
     * instances can be passed directly to Halide filters. */
    operator halide_buffer_t *() {
        return &buf;
    }

    /** Methods for managing any GPU allocation. */
    // @{
    void set_host_dirty(bool v = true) {
        buf.set_host_dirty(v);
    }

    bool device_dirty() const {
        return buf.device_dirty();
    }

    bool host_dirty() const {
        return buf.host_dirty();
    }

    void set_device_dirty(bool v = true) {
        buf.set_device_dirty(v);
    }

    int copy_to_host(void *ctx = nullptr) {
        if (device_dirty()) {
            return halide_copy_to_host(ctx, &buf);
        }
        return 0;
    }

    int copy_to_device(const struct halide_device_interface_t *device_interface, void *ctx = nullptr) {
        if (host_dirty()) {
            return halide_copy_to_device(ctx, &buf, device_interface);
        }
        return 0;
    }

    int device_sync(void *ctx = nullptr) {
        return halide_device_sync(ctx, &buf);
    }

    bool has_device_allocation() const {
        return buf.device != 0;
    }
    // @}

};

}  // namespace HLS
}  // namespace Runtime
}  // namespace Halide

#endif  // HALIDE_RUNTIME_HLS_BUFFER_MINIMAL_H
