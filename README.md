Halide to CPU/FPGA
============

The current compiler is based on Halide release 2017/05/03 (https://github.com/halide/Halide/releases).

Intructions for building examples can be found at the wiki page:
https://github.com/jingpu/Halide-HLS/wiki

A paper is available at https://arxiv.org/abs/1610.09405

If you want to build the compiler in other settings, please refer to the original readme:
https://github.com/jingpu/Halide-HLS/blob/HLS/README.orig.md

For more detail about what Halide is, see http://halide-lang.org.

Build Status
------------

| Linux                        |
|------------------------------|
| [![linux build status][1]][2]|

[1]: https://travis-ci.org/jingpu/Halide-HLS.svg?branch=HLS
[2]: https://travis-ci.org/jingpu/Halide-HLS


Updates
------------
- 2017/06/14 merge Halide nightly 2017/06/13 ( d55ebd5b806110cd77e7c4f2616ddb9ffbf2e99e ), picking up aotcpp_generators tests.
- 2017/06/13 merge Halide release 2017/05/03.