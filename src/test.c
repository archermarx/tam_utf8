//
// UTF-8 tests
//
// TODO: check memory safety - cannot advance past end of byte seq
//

// these bytes should never appear in any conforming UTF-8 seq.
const u8 invalid_bytes[] = {
    0xC0, 0xC1, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9,
    0xFA, 0xFB, 0xFC, 0xFB, 0xFE, 0xFF,
};

int check_codepoint(Bytes *bytes, u32 expected, char *msg) {
    u8 *start = bytes->c;
    u32 c = next_codepoint(bytes);
    if (c != expected) {
        printf("Failure: byte sequence");
        for (u8 *c = start; c != bytes->c; c++) {
            printf(" 0x%X", *c);
        }
        printf(" should decode to U+%X. Got U+%X instead\n", expected, c);
        if (msg)
            printf("msg = %s\n", msg);
        return 1;
    }
    return 0;
}

#define LENGTH(x) (sizeof(x) / sizeof(x[0]))

#define ASSERT_CODEPOINT_MSG(b, expected, msg) \
    (num_failed += check_codepoint(&b, expected, msg))

#define ASSERT_CODEPOINT(b, expected) ASSERT_CODEPOINT_MSG(b, expected, NULL)

#define ASSERT_INVALID(seq) ASSERT_CODEPOINT(seq, INVALID)

#define ASSERT_CODEPOINTS(bytes, codepoints, msg) do {      \
        Bytes b = {.c = bytes};                             \
        for (size_t i = 0; i < LENGTH(codepoints); i++) {   \
            ASSERT_CODEPOINT_MSG(b, codepoints[i], msg);    \
        }                                                   \
    } while (0)

#define ASSERT_ALL_INVALID(bytes, msg) do {             \
    Bytes b = {.c = bytes};                             \
    for (size_t i = 0; i < LENGTH(bytes); i++) {        \
        ASSERT_CODEPOINT_MSG(b, INVALID, msg);          \
    }} while (0)

int test_basic() {
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
}


int test_decode() {
    int num_failed = 0;
    // tests from Markus Kuhn's UTF-8 stress test
    // https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
    // 
    // 1. κόσμε (valid utf-8) 
    {
        u8 bytes[] = {0xce, 0xba, 0xcf, 0x8c, 0xcf, 0x83, 0xce, 0xbc, 0xce, 0xb5, 0x0};
        u32 codepoints[] = {0x3BA, 0x3CC, 0x3C3, 0x3BC, 0x3B5, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "κόσμε");
    }
    // 2.1. first 1,2,3,4-byte sequences
    {
        u8 bytes[] = {0x0, 0xc2, 0x80, 0xe0, 0xa0, 0x80, 0xf0, 0x90, 0x80, 0x80, 0x0};
        u32 codepoints[] = {0x0, 0x80, 0x800, 0x10000, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "first 1,2,3,4-byte sequences");
    }
    // 2.2. last 1,2,3,4-byte sequences 
    {
        u8 bytes[] = {0x7f, 0xdf, 0xbf, 0xef, 0xbf, 0xbf, 0xf4, 0x8f, 0xbf, 0xbf, 0x0};
        u32 codepoints[] = {0x7F, 0x7FF, 0xFFFF, 0x10FFFF, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "last 1,2,3,4-byte sequences");
    }
    // 2.3. other boundary conditions
    {
        u8 bytes[] = {0xed, 0x9f, 0xbf, 0xee, 0x80, 0x80, 0x0};
        u32 codepoints[] = {0xD7FF, 0xE000, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "other boundary conditions");
    }
    // 3: Malformed sequences
    // 3.1: unexpected continuation bytes
    // 3.1.1-3.1.3: first and last continuation bytes and two continuation bytes
    {
        u8 bytes[] = {0x80, 0xbf, 0x0};
        u32 codepoints[] = {INVALID, INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "first and last continuations");
    }
    // 3.1.3: 3 continuation bytes
    {
        u8 bytes[] = {0x80, 0xbf, 0x80, 0x0};
        u32 codepoints[] = {INVALID, INVALID, INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "3 continuation bytes");
    }
    // 3.1.4: 4 continuation bytes
    {
        u8 bytes[] = {0x80, 0xbf, 0x80, 0xbf};
        ASSERT_ALL_INVALID(bytes, "4 continuation bytes");
    }
    // 3.1.5: 5 continuation bytes
    {
        u8 bytes[] = {0x80, 0xbf, 0x80, 0xbf, 0x80};
        ASSERT_ALL_INVALID(bytes, "5 continuation bytes");
    }
    // 3.1.6: 6 continuation bytes
    {
        u8 bytes[] = {0x80, 0xbf, 0x80, 0xbf, 0x80, 0xbf};
        ASSERT_ALL_INVALID(bytes, "6 continuation bytes");
    }
    // 3.1.8: all continuation bytes
    {
        u8 bytes[] = {
            0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
            0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
            0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
            0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
            0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
            0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
            0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
            0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf
        }; 
        ASSERT_ALL_INVALID(bytes, "all continuation bytes in a row");
    }
    // 3.2: all lonely start characters
    // 3.2.1: All 32 first bytes of a two-byte sequence, each followed by a space character
    {
        u8 bytes[] = {
            0xc0, 0x20, 0xc1, 0x20, 0xc2, 0x20, 0xc3, 0x20,
            0xc4, 0x20, 0xc5, 0x20, 0xc6, 0x20, 0xc7, 0x20, 
            0xc8, 0x20, 0xc9, 0x20, 0xca, 0x20, 0xcb, 0x20, 
            0xcc, 0x20, 0xcd, 0x20, 0xce, 0x20, 0xcf, 0x20, 
            0xd0, 0x20, 0xd1, 0x20, 0xd2, 0x20, 0xd3, 0x20,
            0xd4, 0x20, 0xd5, 0x20, 0xd6, 0x20, 0xd7, 0x20,
            0xd8, 0x20, 0xd9, 0x20, 0xda, 0x20, 0xdb, 0x20,
            0xdc, 0x20, 0xdd, 0x20, 0xde, 0x20, 0xdf, 0x20
        };
        u32 codepoints[] = {
            INVALID, 0x20, INVALID, 0x20, INVALID, 0x20, INVALID, 0x20,
            INVALID, 0x20, INVALID, 0x20, INVALID, 0x20, INVALID, 0x20,
            INVALID, 0x20, INVALID, 0x20, INVALID, 0x20, INVALID, 0x20,
            INVALID, 0x20, INVALID, 0x20, INVALID, 0x20, INVALID, 0x20,
            INVALID, 0x20, INVALID, 0x20, INVALID, 0x20, INVALID, 0x20,
            INVALID, 0x20, INVALID, 0x20, INVALID, 0x20, INVALID, 0x20,
            INVALID, 0x20, INVALID, 0x20, INVALID, 0x20, INVALID, 0x20,
            INVALID, 0x20, INVALID, 0x20, INVALID, 0x20, INVALID, 0x20
        };
        ASSERT_CODEPOINTS(bytes, codepoints, "two-byte start chars with spaces");
    }
    // 3.2.2: all 16 first bytes of a three-byte sequence, each followed by a space character
    {
        u8 bytes[] = {
            0xe0, 0x20, 0xe1, 0x20, 0xe2, 0x20, 0xe3, 0x20,
            0xe4, 0x20, 0xe5, 0x20, 0xe6, 0x20, 0xe7, 0x20,
            0xe8, 0x20, 0xe9, 0x20, 0xea, 0x20, 0xeb, 0x20,
            0xec, 0x20, 0xed, 0x20, 0xee, 0x20, 0xef, 0x20                                          
        };
        u32 codepoints[] = {
            INVALID, 0x20, INVALID, 0x20, INVALID, 0x20, INVALID, 0x20,
            INVALID, 0x20, INVALID, 0x20, INVALID, 0x20, INVALID, 0x20,
            INVALID, 0x20, INVALID, 0x20, INVALID, 0x20, INVALID, 0x20,
            INVALID, 0x20, INVALID, 0x20, INVALID, 0x20, INVALID, 0x20,
        };
        ASSERT_CODEPOINTS(bytes, codepoints, "three-byte start chars with spaces");
    }
    // 3.2.3: all 8 first bytes of a four-byte sequence, each followed by a space character
    {
        u8 bytes[] = {
            0xf0, 0x20, 0xf1, 0x20, 0xf2, 0x20, 0xf3, 0x20,
            0xf4, 0x20, 0xf5, 0x20, 0xf6, 0x20, 0xf7, 0x20   
        };
        u32 codepoints[] = {
            INVALID, 0x20, INVALID, 0x20, INVALID, 0x20, INVALID, 0x20,
            INVALID, 0x20, INVALID, 0x20, INVALID, 0x20, INVALID, 0x20,
        };
        ASSERT_CODEPOINTS(bytes, codepoints, "four byte start chars with spaces");
    }
    // 3.3: Sequences with last continuation byte missing
    // Should return single invalid codepoint
    // 3.3.1: 2-byte sequence with last byte missing (first)
    // 3.3.6: 2-byte sequence with last byte missing (last)
    {
        u8 b1[] = {0xc0, 0x00};
        u8 b2[] = {0xdf, 0x00};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(b1, codepoints, "2-byte sequence missing continuation (1)");
        ASSERT_CODEPOINTS(b2, codepoints, "2-byte sequence missing continuation (2)");
    }
    // 3.3.2: 3-byte sequence with last byte missing (first)
    // 3.3.7: 3-byte sequence with last byte missing (lat)
    {
        u8 b1[] = {0xe0, 0x80, 0x00};
        u8 b2[] = {0xef, 0xbf, 0x00};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(b1, codepoints, "3-byte sequence missing continuation (1)");
        ASSERT_CODEPOINTS(b2, codepoints, "3-byte sequence missing continuation (2)");
    }
    // 3.3.3: 4-byte sequence with last byte missing (first)
    // 3.3.8: 4-byte sequence with last byte missing (last)
    {
        u8 b1[] = {0xf0, 0x80, 0x80, 0x00};
        u8 b2[] = {0xf7, 0xbf, 0xbf, 0x00};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(b1, codepoints, "4-byte sequence missing continuation (1)");
        ASSERT_CODEPOINTS(b2, codepoints, "4-byte sequence missing continuation (2)");
    }
    // 3.3.4: 5-byte sequence with last byte missing (first)
    // 3.3.9: 5-byte sequence with last byte missing (last)
    // NOTE: five-byte sequences are not supported, so these should emit individual INVALID bytes
    {
        u8 b1[] = {0xf8, 0x80, 0x80, 0x80, 0x00};
        u8 b2[] = {0xfb, 0xbf, 0xbf, 0xbf, 0x00};
        u32 codepoints[] = {INVALID, INVALID, INVALID, INVALID, 0x0};
        ASSERT_CODEPOINTS(b1, codepoints, "5-byte sequence missing continuation (1)");
        ASSERT_CODEPOINTS(b2, codepoints, "5-byte sequence missing continuation (2)");
    }
    // 3.3.4: 6-byte sequence with last byte missing (first)
    // 3.3.9: 6-byte sequence with last byte missing (last)
    // NOTE: five-byte sequences are not supported, so these should emit individual INVALID bytes
    {
        u8 b1[] = {0xfc, 0x80, 0x80, 0x80, 0x80, 0x00};
        u8 b2[] = {0xfd, 0xbf, 0xbf, 0xbf, 0xbf, 0x00};
        u32 codepoints[] = {INVALID, INVALID, INVALID, INVALID, INVALID, 0x0};
        ASSERT_CODEPOINTS(b1, codepoints, "6-byte sequence missing continuation (1)");
        ASSERT_CODEPOINTS(b2, codepoints, "6-byte sequence missing continuation (2)");
    }
    // 3.4: concatenation of incomplete sequences
    // all incomplete 2,3,and 4-byte sequences concatenated
    // should see six INVALIDs
    {
        u8 bytes[] = {
            0xc0, 
            0xe0, 0x80,
            0xf0, 0x80, 0x80,
            0xdf,
            0xef, 0xbf,
            0xf7, 0xbf, 0xbf,
            0x00
        };
        u32 codepoints[] = {INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "concatenation of incomplete sequences");
    }
    // 3.5: impossible bytes
    // these bytes should never appear in a valid utf-8 string
    {
        u8 bytes[] = {0xfe, 0xff, 0xfe, 0xfe, 0xff, 0xff};
        ASSERT_ALL_INVALID(bytes, "impossible bytes");
    }
    // 4 - Overlong sequences
    // 4.1 - Overlong ASCII characters
    // 4.1.1 - two bytes
    {
        u8 bytes[] = {0xc0, 0xaf, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "overlong ascii, two bytes");
    }
    // 4.1.2 - three bytes
    {
        u8 bytes[] = {0xe0, 0x80, 0xaf, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "overlong ascii, three bytes");
    }
    // 4.1.3 - four bytes
    {
        u8 bytes[] = {0xf0, 0x80, 0x80, 0xaf, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "overlong ascii, four bytes");
    }
    // 4.2 - maximum overlong sequences
    // checks boundaries between overlong chars and valid chars
    // 4.2.1 - two bytes
    {
        u8 bytes[] = {0xc1, 0xbf, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "maximum overlong, two bytes");
    }
    // 4.2.2 - three bytes
    {
        u8 bytes[] = {0xe0, 0x9f, 0xbf, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "maximum overlong, three bytes");
    }
    // 4.2.3 - four bytes
    {
        u8 bytes[] = {0xf0, 0x8f, 0xbf, 0xbf, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "maximum overlong, four bytes");
    }
    // 4.3 - overlong null terminators
    // 4.3.1 - two bytes
    {
        u8 bytes[] = {0xc0, 0x80, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "overlong null, two bytes");
    }
    // 4.3.1 - three bytes
    {
        u8 bytes[] = {0xe0, 0x80, 0x80, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "overlong null, three bytes");
    }
    // 4.3.1 - four bytes
    {
        u8 bytes[] = {0xf0, 0x80, 0x80, 0x80, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "overlong null, four bytes");
    }
    // 5 - UTF-16 surrogates
    // 5.1 - Single UTF-16 surrogates
    // 5.1.1
    {
        u8 bytes[] = {0xed, 0xa0, 0x80, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.1.1 - utf16 surrogate");
    }
    // 5.1.2
    {
        u8 bytes[] = {0xed, 0xad, 0xbf, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.1.2 - utf16 surrogate");
    }
    // 5.1.3
    {
        u8 bytes[] = {0xed, 0xae, 0x90, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.1.3 - utf16 surrogate");
    }
    // 5.1.4
    {
        u8 bytes[] = {0xed, 0xaf, 0xbf, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.1.4 - utf16 surrogate");
    }
    // 5.1.5
    {
        u8 bytes[] = {0xed, 0xb0, 0x80, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.1.5 - utf16 surrogate");
    }
    // 5.1.6
    {
        u8 bytes[] = {0xed, 0xbe, 0x80, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.1.6 - utf16 surrogate");
    }
    // 5.1.7
    {
        u8 bytes[] = {0xed, 0xbf, 0xbf, 0x0};
        u32 codepoints[] = {INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.1.7 - utf16 surrogate");
    }
    // 5.2 - Paired UTF-16 surrogates
    // 5.2.1
    {
        u8 bytes[] = {0xed, 0xa0, 0x80, 0xed, 0xb0, 0x80, 0x0};
        u32 codepoints[] = {INVALID, INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.2.1 - paired utf16 surrogates");
    }
    // 5.2.2
    {
        u8 bytes[] = {0xed, 0xa0, 0x80, 0xed, 0xbf, 0xbf, 0x0};
        u32 codepoints[] = {INVALID, INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.2.2 - paired utf16 surrogates");
    }
    // 5.2.3
    {
        u8 bytes[] = {0xed, 0xad, 0xbf, 0xed, 0xb0, 0x80, 0x0};
        u32 codepoints[] = {INVALID, INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.2.3 - paired utf16 surrogates");
    }
    // 5.2.4
    {
        u8 bytes[] = {0xed, 0xad, 0xbf, 0xed, 0xbf, 0xbf, 0x0};
        u32 codepoints[] = {INVALID, INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.2.4 - paired utf16 surrogates");
    }
    // 5.2.5
    {
        u8 bytes[] = {0xed, 0xae, 0x80, 0xed, 0xb0, 0x80, 0x0};
        u32 codepoints[] = {INVALID, INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.2.5 - paired utf16 surrogates");
    }
    // 5.2.6
    {
        u8 bytes[] = {0xed, 0xae, 0x80, 0xed, 0xbf, 0xbf, 0x0};
        u32 codepoints[] = {INVALID, INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.2.6 - paired utf16 surrogates");
    }
    // 5.2.7
    {
        u8 bytes[] = {0xed, 0xaf, 0xbf, 0xed, 0xb0, 0x80, 0x0};
        u32 codepoints[] = {INVALID, INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.2.7 - paired utf16 surrogates");
    }
    // 5.2.8
    {
        u8 bytes[] = {0xed, 0xaf, 0xbf, 0xed, 0xbf, 0xbf, 0x0};
        u32 codepoints[] = {INVALID, INVALID, 0x0};
        ASSERT_CODEPOINTS(bytes, codepoints, "5.2.8 - paired utf16 surrogates");
    }

    return num_failed;
}

int run_tests() {
    int failed0 = test_basic();
    int failed1 = test_decode();
    return failed0 + failed1;
}

#undef ASSERT_CODEPOINT
#undef ASSERT_INVALID
