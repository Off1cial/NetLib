#ifndef COMMON_H
#define COMMON_H    

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

#define NET_USE_ASSERT 1

#define PERROR() fprintf(stderr, "System error: %s (%d)\n", strerror(errno), errno)

static inline void ASSERT(int8_t condition, const char* msg){
#ifdef NET_USE_ASSERT
    if (msg) assert(condition && msg);
    else assert(condition);
#endif

}

#endif
