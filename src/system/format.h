#ifndef __FORMAT__H__
#define __FORMAT__H__

#ifdef __cplusplus
extern "C" {
#endif

// this function is not threadsafe as it uses the only one buffer
const char* format(const char* string, ...);

#ifdef __cplusplus
}
#endif


#endif