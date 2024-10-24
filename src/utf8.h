#ifndef UTF8_H
#define UTF8_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef int8_t i8;
typedef int32_t i32;
typedef uint32_t u32;

#define INVALID 0xFFFD

typedef struct {
    u8 *c;
} Bytes;


u32 next_codepoint(Bytes *bytes);

#endif // UTF8_H
