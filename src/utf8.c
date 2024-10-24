#include "utf8.h"

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

u32 next_codepoint(Bytes *bytes) {
#define NEXT_BYTE() (*(bytes->c)++)
#define REWIND(n) (bytes->c -= n)
    // get first byte
    u32 b0 = NEXT_BYTE();

    // check for range 1: ascii
    if (b0 < 128) {
        return b0 & MASK_1;
    }

    // check for range 2:
    // bytes: 110xxxyy 10yyzzzz
    if (starts_with_110(b0)) {
        u32 b1 = NEXT_BYTE();
        if (starts_with_10(b1)) {
            // in range 2
            u32 codepoint = (b1 & MASK_2) + ((b0 & MASK_3) << 6);
            if (in_range_2(codepoint)) {
                return codepoint;
            } else {
                return INVALID;
            }
        } else {
            REWIND(1);
        }
    }

    // check for range 3:
    // bytes: 1110wwww 10xxxxyy 10yyzzzz
    if (starts_with_1110(b0)) {
        u32 b1 = NEXT_BYTE();
        if (starts_with_10(b1)) {
            u32 b2 = NEXT_BYTE();
            if (starts_with_10(b2)) {
                u32 codepoint = (b2 & MASK_2) + 
                                ((b1 & MASK_2) << 6) +
                                ((b0 & MASK_4) << 12);
                if (in_range_3(codepoint)) {
                    return codepoint;
                } else {
                    return INVALID;
                }

            } else {
                REWIND(1);
            }
        } else {
            REWIND(1);
        }
    }

    // check for range 4:
    // bytes: 11110uvv 10vvwwww 10xxxxyy 10yyzzzz
    if (starts_with_11110(b0)) {
        u32 b1 = NEXT_BYTE();
        if (starts_with_10(b1)) {
            u32 b2 = NEXT_BYTE();
            if (starts_with_10(b2)) {
                u32 b3 = NEXT_BYTE();
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
                    REWIND(1);
                }
            } else {
                REWIND(1);
            }
        } else {
            REWIND(1);
        }
    }

    return INVALID;
#undef NEXT_BYTE
#undef REWIND
}
