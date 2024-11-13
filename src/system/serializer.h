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

enum serializer_status
{
    serializer_no_error,
    serializer_write_error,
    serializer_read_error
};

typedef struct 
{
	uint8_t* buffer;
	size_t position;	
	size_t buffer_size;
    serializer_status status;
} serializer_context;

//-----------------------------------------------------------------------------------------------------------------------------
static inline void serializer_init(serializer_context* context, void* buffer, size_t buffer_size)
{
    context->buffer = (uint8_t*) buffer;
    context->buffer_size = buffer_size;
    context->position = 0;
    context->status = serializer_no_error;
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
static inline enum serializer_status serializer_get_status(serializer_context* context)
{
    return context->status;
}

//-----------------------------------------------------------------------------------------------------------------------------
// write functions, set the status to error if there is no more space in the buffer
#define DECLARE_SERIALIZER_WRITE_FUNC(type)                                             \
static inline void serializer_write_##type(serializer_context* context, type value)     \
{                                                                                       \
    if (context->position + sizeof(type) < context->buffer_size)                        \
    {                                                                                   \
        type* cast = (type*)&context->buffer[context->position];                        \
        *cast = value;                                                                  \
        context->position += sizeof(type);                                              \
    }                                                                                   \
    else context->status = serializer_write_error;                                      \
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
static inline void serializer_write_blob(serializer_context* context, const void* blob, size_t blob_size)
{
    if (serializer_get_leftspace(context) >= blob_size)
    {
        memcpy(&context->buffer[context->position], blob, blob_size);
        context->position += blob_size;
    }
    else context->status = serializer_write_error;
}

#define serializer_write_struct(context, variable) serializer_write_blob(context, &variable, sizeof(variable))

//-----------------------------------------------------------------------------------------------------------------------------
// read functions
#define DECLARE_SERIALIZER_READ_FUNC(type)                                              \
static inline type serializer_read_##type(serializer_context* context)                   \
{                                                                                       \
    if (context->position + sizeof(type) < context->buffer_size)                        \
    {                                                                                   \
        type* cast = (type*)&context->buffer[context->position];                        \
        context->position += sizeof(type);                                              \
        return *cast;                                                                   \
    }                                                                                   \
    context->status = serializer_read_error;                                            \
    return (type)0;                                                                     \
}

DECLARE_SERIALIZER_READ_FUNC(float);
DECLARE_SERIALIZER_READ_FUNC(uint8_t);
DECLARE_SERIALIZER_READ_FUNC(uint16_t);
DECLARE_SERIALIZER_READ_FUNC(uint32_t);
DECLARE_SERIALIZER_READ_FUNC(uint64_t);
DECLARE_SERIALIZER_READ_FUNC(int8_t);
DECLARE_SERIALIZER_READ_FUNC(int16_t);
DECLARE_SERIALIZER_READ_FUNC(int64_t);
DECLARE_SERIALIZER_READ_FUNC(size_t);

//-----------------------------------------------------------------------------------------------------------------------------
static inline void serializer_read_blob(serializer_context* context, void* blob, size_t blob_size)
{
    if (serializer_get_leftspace(context) >= blob_size)
    {
        memcpy(blob, &context->buffer[context->position], blob_size);
        context->position += blob_size;
    }
    else context->status = serializer_read_error;
}

#endif
