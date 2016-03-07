#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <limits>

// Benchmark the operation 'op'. The number of iterations refers to
// how many times the operation is run for each time measurement, the
// result is the minimum over a number of samples runs. The result is the
// amount of time in seconds for one iteration.
#ifdef _WIN32

union _LARGE_INTEGER;
typedef union _LARGE_INTEGER LARGE_INTEGER;
extern "C" int __stdcall QueryPerformanceCounter(LARGE_INTEGER*);
extern "C" int __stdcall QueryPerformanceFrequency(LARGE_INTEGER*);

template <typename F>
double benchmark(int samples, int iterations, F op) {
    int64_t freq;
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);

    double best = std::numeric_limits<double>::infinity();
    for (int i = 0; i < samples; i++) {
        int64_t t1;
        QueryPerformanceCounter((LARGE_INTEGER*)&t1);
        for (int j = 0; j < iterations; j++) {
            op();
        }
        int64_t t2;
        QueryPerformanceCounter((LARGE_INTEGER*)&t2);
        double dt = (t2 - t1) / static_cast<double>(freq);
        if (dt < best) best = dt;
    }
    return best / iterations;
}

#else

#include <chrono>

template <typename F>
double benchmark(int samples, int iterations, F op) {
    double best = std::numeric_limits<double>::infinity();
    for (int i = 0; i < samples; i++) {
        auto t1 = std::chrono::high_resolution_clock::now();
        for (int j = 0; j < iterations; j++) {
            op();
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        double dt = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() / 1e6;
        if (dt < best) best = dt;
    }
    return best / iterations;
}

#include <sys/ioctl.h>
#define READ_TIMER 1010 // Retreive hw timer count

double zynq_gettime_sec(int fd) {
  struct {
    unsigned long counter0;
    unsigned long counter1;
  } count;
  ioctl(fd, READ_TIMER, (long unsigned int)(&count));

  const double FREQ_MHZ = 125.0;
  const double K = 4294967296.0;  // 2^32
  return count.counter0 / FREQ_MHZ / 1e6
    + count.counter1 * (K / FREQ_MHZ / 1e6);
}

template <typename F>
double benchmark_zynq(int fd, int samples, int iterations, F op) {
    double best = std::numeric_limits<double>::infinity();
    for (int i = 0; i < samples; i++) {
        double t1 = zynq_gettime_sec(fd);
        for (int j = 0; j < iterations; j++) {
            op();
        }
        double t2 = zynq_gettime_sec(fd);
        double dt = t2 - t1;
        if (dt < best) best = dt;
    }
    return best / iterations;
}


#endif

#endif
