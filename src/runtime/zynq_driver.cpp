#include "HalideRuntimeZynq.h"

#ifndef _IOCTL_CMDS_H_
#define _IOCTL_CMDS_H_

// TODO: switch these out for "proper" mostly-system-unique ioctl numbers
#define GET_BUFFER 1000 // Get an unused buffer
#define GRAB_IMAGE 1001 // Acquire image from camera
#define FREE_IMAGE 1002 // Release buffer
#define PROCESS_IMAGE 1003 // Push to stencil path
#define PEND_PROCESSED 1004 // Retreive from stencil path

#endif

extern "C" int slice_buffer(Buffer* src, Buffer* des, unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
    /*
 if (width == 0 || height == 0) {
  printf("slice_buffer failed: width and height of slide should be non-zero.\n");
  return -1;
 }
 if (x + width > src->width || y + height > src->height) {
  printf("slice_buffer failed: slice is out of range.\n");
  return -1;
 }
    */
 *des = *src; // copy depth, stride, data, etc.
 des->width = width;
 des->height = height;
 des->phys_addr += src->depth * (y * src->stride + x);
 des->mmap_offset += src->depth * (y * src->stride + x);
 return 0;
}
