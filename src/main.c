#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef int8_t i8;
typedef int32_t i32;
typedef uint32_t u32;

typedef union {
    u32 val;
    struct {
        u8 bytes[4];
    };
} rune;

u32 bytes_to_u32(const u8 *bytes, u8 num_bytes) {
    u32 num = 0;
    for (int i = 0; i < num_bytes; i++) {
        printf("%X ", bytes[i]);
        num += bytes[i] << (i * 8); 
    }
    printf("\n");
    return num;
}

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

const u32 REPLACEMENT_CHAR = 0xFFFD;

typedef struct {
    u8 *c;
} Bytes;

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
                return REPLACEMENT_CHAR;
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
                    return REPLACEMENT_CHAR;
                }

            } else {
                REWIND(2);
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
                        return REPLACEMENT_CHAR;
                    }
                } else {
                    REWIND(3);
                }
            } else {
                REWIND(2);
            }
        } else {
            REWIND(1);
        }
    }

    return REPLACEMENT_CHAR;
}

// these bytes should never appear in any conforming UTF-8 seq.
const u8 invalid_bytes[] = {
    0xC0, 0xC1, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9,
    0xFA, 0xFB, 0xFC, 0xFB, 0xFE, 0xFF,
};

int check_codepoint(Bytes *bytes, u32 expected) {
    u8 *start = bytes->c;
    u32 c = next_codepoint(bytes);
    if (c != expected) {
        printf("Failure: byte sequence");
        for (u8 *c = start; c != bytes->c; c++) {
            printf(" 0x%X", *c);
        }
        printf(" should decoded to U+%X. Got U+%X instead\n", expected, c);
        return 1;
    }
    return 0;
}

int run_tests() {
#define ASSERT_CODEPOINT(b, expected) \
    (num_failed += check_codepoint(&b, expected))

#define ASSERT_INVALID(seq) ASSERT_CODEPOINT(seq, REPLACEMENT_CHAR)
    // check that invalid bytes are always rejected
    int num_failed = 0;
    int num_invalid_bytes = sizeof(invalid_bytes) / sizeof(u8);
    u8 single_bytes[] = {0x00};
    u8 double_bytes[] = {0xC2, 0x00};
    Bytes b;

    for (int i = 0; i < num_invalid_bytes; i++) {
        single_bytes[0] = invalid_bytes[i];
        b.c = single_bytes;
        ASSERT_INVALID(b);
       
        double_bytes[1] = invalid_bytes[i];
        b.c = double_bytes;
        ASSERT_INVALID(b);
    }

    u8 fur[] = {'f', 0xFC, 'r'};
    Bytes fur_bytes = {.c = fur};
    ASSERT_CODEPOINT(fur_bytes, 'f');
    ASSERT_INVALID(fur_bytes);
    ASSERT_CODEPOINT(fur_bytes, 'r');

    u8 ascii[] = {'a', 's', 'c', 'i', 'i', 0x00, 0xC0};
    Bytes ascii_bytes = {.c = ascii};
    ASSERT_CODEPOINT(ascii_bytes, 'a');
    ASSERT_CODEPOINT(ascii_bytes, 's');
    ASSERT_CODEPOINT(ascii_bytes, 'c');
    ASSERT_CODEPOINT(ascii_bytes, 'i');
    ASSERT_CODEPOINT(ascii_bytes, 'i');
    ASSERT_CODEPOINT(ascii_bytes, 0x0);
    ASSERT_INVALID(ascii_bytes);

    // check that encoded codepoints are minimal
    u8 overlong_1[] = {0xC0, 0x80};
    u8 overlong_2[] = {0xE0, 0x80, 0x80};
    u8 overlong_3[] = {0xF0, 0x80, 0x80, 0x80};
    
    b.c = overlong_1;
    ASSERT_INVALID(b);
    b.c = overlong_2;
    ASSERT_INVALID(b);
    b.c = overlong_3;
    ASSERT_INVALID(b);

    // check some two-byte sequences
    u8 two_bytes[] = {
        0xC2, 0xA9,
        0xC2, 0xAA,
        0xCA, 0x81,
        0x00
    };
    b.c = two_bytes;
    ASSERT_CODEPOINT(b, 0xA9);
    ASSERT_CODEPOINT(b, 0xAA);
    ASSERT_CODEPOINT(b, 0x281);
    ASSERT_CODEPOINT(b, 0x0);

    // check some three-byte sequences
    u8 three_bytes[] = {
        0xE0,0xA4,0xA6, // द
        0xE0,0xA5,0x9B, // ज़
        0xE1,0x87,0xB6, // ᇶ
        0xE2,0x80,0xB1, // ‱
        0xE3,0x8A,0x92, // ㊒
        0x0
    };
    b.c = three_bytes;
    ASSERT_CODEPOINT(b, 0x926);
    ASSERT_CODEPOINT(b, 0x95B);
    ASSERT_CODEPOINT(b, 0x11F6);
    ASSERT_CODEPOINT(b, 0x2031);
    ASSERT_CODEPOINT(b, 0x3292);
    ASSERT_CODEPOINT(b, 0x0);

    // check some four-byte sequences
    u8 four_bytes[] = {
        0xF0, 0x90, 0xBA, 0x80, // 𐺀
        0xF0, 0x91, 0x85, 0x83, // 𑅃
        0xF0, 0x92, 0x84, 0xA1, // 𒄡
        0xF0, 0x92, 0x95, 0x83, // 𒕃
        0x0
    };
    b.c = four_bytes;
    ASSERT_CODEPOINT(b,0x10E80);
    ASSERT_CODEPOINT(b,0x11143);
    ASSERT_CODEPOINT(b,0x12121);
    ASSERT_CODEPOINT(b,0x12543);
    ASSERT_CODEPOINT(b,0x0);

    return num_failed;
#undef ASSERT_CODEPOINT
#undef ASSERT_INVALID
}

int main() {
    int num_failed = run_tests();
    if (num_failed == 0) {
        printf("All tests passed!\n");
    } else {
        printf("%d tests failed\n", num_failed);
    }
}
