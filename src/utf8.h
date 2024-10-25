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

#ifdef TAM_UTF8_IMPLEMENTATION

const u8 MASK_1 = 0x7F; // 01 11 11 11
const u8 MASK_2 = 0x3F; // 00 11 11 11
const u8 MASK_3 = 0x1F; // 00 01 11 11
const u8 MASK_4 = 0x0F; // 00 00 11 11
const u8 MASK_5 = 0x07; // 00 00 01 11

bool starts_with_10(const u8 c) {
    return c >= 0x80 && c <= 0xBF;
}

bool starts_with_110(const u8 c) {
    return c >= 0xC0 && c <= 0xDF; 
}

bool starts_with_1110(const u8 c) {
    return c >= 0xE0 && c <= 0xEF;
}

bool starts_with_11110(const u8 c) {
    return c >= 0xF0 && c <= 0xF7;
}

bool in_range_2(const u32 c) {
    return c >= 0x80 && c <= 0x7FF;
}

bool in_range_3(const u32 c) {
    return c >= 0x800 && c <= 0xFFFF;
}

bool in_range_4(const u32 c) {
    return c >= 0x010000 && c <= 0x10FFFF;
}

bool is_continuation(const u8 c) {
    return c >= 0x80 && c <= 0xBF;
}

bool is_utf16_surrogate(const u32 c) {
    return c >= 0xD800 && c <= 0xDFFF;
}

Bytes b_fromptr(u8 *ptr, size_t n) {
    return (Bytes){.bytes = ptr, .pos = 0, .len = n};
}

u8 b_peek(Bytes *bytes) {
    if (bytes->pos >= 0 && bytes->pos < bytes->len) {
        return bytes->bytes[bytes->pos];
    } else {
        return 0x0;
    }
}

u8 b_read(Bytes *bytes) {
    u8 byte = b_peek(bytes);
    bytes->pos++;
    return byte;
}

void b_rewind(Bytes *bytes, int n) {
    bytes->pos -= n;
    if (bytes->pos < 0) bytes->pos = 0;
}

u32 next_codepoint(Bytes *bytes) {
    // get first byte
    u32 b0 = b_read(bytes);

    // check for range 1: ascii
    if (b0 < 128) {
        return b0 & MASK_1;
    }

    // check for range 2:
    // bytes: 110xxxyy 10yyzzzz
    if (starts_with_110(b0)) {
        u32 b1 = b_read(bytes);
        if (starts_with_10(b1)) {
            // in range 2
            u32 codepoint = (b1 & MASK_2) + ((b0 & MASK_3) << 6);
            if (in_range_2(codepoint)) {
                return codepoint;
            } else {
                return INVALID;
            }
        } else {
            b_rewind(bytes, 1);
        }
    }

    // check for range 3:
    // bytes: 1110wwww 10xxxxyy 10yyzzzz
    if (starts_with_1110(b0)) {
        u32 b1 = b_read(bytes);
        if (starts_with_10(b1)) {
            u32 b2 = b_read(bytes);
            if (starts_with_10(b2)) {
                u32 codepoint = (b2 & MASK_2) + 
                                ((b1 & MASK_2) << 6) +
                                ((b0 & MASK_4) << 12);
                if (in_range_3(codepoint) && !is_utf16_surrogate(codepoint)) {
                    return codepoint;
                } else {
                    return INVALID;
                }

            } else {
                b_rewind(bytes, 1);
            }
        } else {
            b_rewind(bytes, 1);
        }
    }

    // check for range 4:
    // bytes: 11110uvv 10vvwwww 10xxxxyy 10yyzzzz
    if (starts_with_11110(b0)) {
        u32 b1 = b_read(bytes);
        if (starts_with_10(b1)) {
            u32 b2 = b_read(bytes);
            if (starts_with_10(b2)) {
                u32 b3 = b_read(bytes);
                if (starts_with_10(b3)) {
                    u32 codepoint = (b3 & MASK_2) + 
                                    ((b2 & MASK_2) << 6) +
                                    ((b1 & MASK_2) << 12) +
                                    ((b0 & MASK_5) << 18);
                    if (in_range_4(codepoint)) {
                        return codepoint;
                    } else {
                        return INVALID;
                    }
                } else {
                    b_rewind(bytes, 1);
                }
            } else {
                b_rewind(bytes, 1);
            }
        } else {
            b_rewind(bytes, 1);
        }
    }

    return INVALID;
}

#endif // TAM_UTF8_IMPLEMENTATION

#endif // UTF8_H
