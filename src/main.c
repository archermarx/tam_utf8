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

u32 next_codepoint(const u8 *c) {

    // check for range 1: ascii
    if (*c < 128) {
        u8 byte = *c++;
        return byte & MASK_1;
    }

    // check for range 2:
    // bytes: 110xxxyy 10yyzzzz
    if (starts_with_110(*c)) {
        u32 b0 = *c++; 
        if (starts_with_10(*c)) {
            // in range 2
            u32 b1 = *c++;
            printf("%02X %02X\n", b0, b1);
            return (b1 & MASK_2) + ((b0 & MASK_3) << 6);
        } else {
            // not in range 2, need to backtrack
            c--;
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    const u8* input = "";
    if (argc > 1) {
        input = argv[1];
    }

    printf("input was `%s`\n", input);

    printf("0x%04X\n", next_codepoint(input));
}
