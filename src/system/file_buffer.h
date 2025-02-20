#ifndef __FILE_BUFFER_H__
#define __FILE_BUFFER_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* read_file(const char* filename, size_t* file_size);

#ifdef __cplusplus
}
#endif

#endif