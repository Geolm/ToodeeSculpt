#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

//-----------------------------------------------------------------------------------------------------------------------------
// Simple serializer
//  * does not allocate/deallocate memory
//  * allow to write/read basic values, array, struct (POD)
//  * avoid overflow
//-----------------------------------------------------------------------------------------------------------------------------

typedef struct 
{
	uint8_t* buffer;
	size_t position;	
	size_t buffer_size;
} serializer_context;

//-----------------------------------------------------------------------------------------------------------------------------
static inline void serializer_init(serializer_context* context, void* buffer, size_t buffer_size)
{
    context->buffer = (uint8_t*) buffer;
    context->buffer_size = buffer_size;
    context->position = 0;
}

//-----------------------------------------------------------------------------------------------------------------------------
// go back at the beginning of the buffer, doesn't clear the buffer
static inline void serializer_restart(serializer_context* context)
{
    context->position = 0;
}

//-----------------------------------------------------------------------------------------------------------------------------
// returns how many bytes is left to be written/read in the buffer
static inline size_t serializer_get_leftspace(serializer_context* context)
{
    return context->buffer_size - context->position;
}

//-----------------------------------------------------------------------------------------------------------------------------
static inline size_t serializer_get_position(serializer_context* context)
{
    return context->position;
}

//-----------------------------------------------------------------------------------------------------------------------------
// write functions, return false if there is no enough space to write the data
#define DECLARE_SERIALIZER_WRITE_FUNC(type)                                             \
static inline bool serializer_write_##type(serializer_context* context, type value)     \
{                                                                                       \
    if (context->position + sizeof(type) < context->buffer_size)                        \
    {                                                                                   \
        type* cast = (type*)&context->buffer[context->position];                        \
        *cast = value;                                                                  \
        context->position += sizeof(type);                                              \
        return true;                                                                    \
    }                                                                                   \
    return false;                                                                       \
}

DECLARE_SERIALIZER_WRITE_FUNC(float);
DECLARE_SERIALIZER_WRITE_FUNC(uint8_t);
DECLARE_SERIALIZER_WRITE_FUNC(uint16_t);
DECLARE_SERIALIZER_WRITE_FUNC(uint32_t);
DECLARE_SERIALIZER_WRITE_FUNC(uint64_t);
DECLARE_SERIALIZER_WRITE_FUNC(int8_t);
DECLARE_SERIALIZER_WRITE_FUNC(int16_t);
DECLARE_SERIALIZER_WRITE_FUNC(int64_t);
DECLARE_SERIALIZER_WRITE_FUNC(size_t);

//-----------------------------------------------------------------------------------------------------------------------------
static inline bool serializer_write_blob(serializer_context* context, void* blob, size_t blob_size)
{
    if (serializer_get_leftspace(context) >= blob_size)
    {
        memcpy(&context->buffer[context->position], blob, blob_size);
        context->position += blob_size;
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------------------------------------------------------
static inline bool serializer_write_array(serializer_context* context, void* array, size_t count, size_t element_size)
{
    if (serializer_get_leftspace(context) >= sizeof(size_t) + count * element_size)
    {
        serializer_write_size_t(context, count);
        serializer_write_blob(context, array, count * element_size);
        return true;
    }
    return false;
}

#define serializer_write_struct(context, variable) serializer_write_blob(context, &variable, sizeof(variable))

#endif
