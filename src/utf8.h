#ifndef UTF8_H
#define UTF8_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef int8_t i8;
typedef int32_t i32;
typedef uint32_t u32;

#define INVALID 0xFFFD

typedef struct {
    u8 *bytes;
    int pos;
    int len;
} Bytes;

typedef struct {
    u32 *codepoints;
    int pos;
    int len;
} Codepoints;

#define b_fromarr(bytes) b_fromptr(bytes, sizeof(bytes)/sizeof(u8))
Bytes b_fromptr(u8 *arr, size_t n);

u32 next_codepoint(Bytes *bytes);

#endif // UTF8_H
