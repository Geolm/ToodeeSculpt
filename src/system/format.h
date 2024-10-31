#ifndef __FORMAT__H__
#define __FORMAT__H__

// this function is not threadsafe as it uses the only one buffer
#ifdef __cplusplus
extern "C" {
#endif

const char* format(const char* string, ...);

#ifdef __cplusplus
}
#endif


#endif