#include "keygen.h"

#include <string.h>

static const char alphabet[33] = "23456789abcdefghjkmnpqrstuvwxyz?";

/* substitution table at original virtual address 0x402053. */
static const unsigned char substitution[256] = {
    0x08, 0x0f, 0x1a, 0x1b, 0x1e, 0x06, 0x1d, 0x0e, 0x11, 0x0c, 0x13, 0x14, 0x18, 0x09, 0x0b, 0x15,
    0x1f, 0x05, 0x07, 0x1c, 0x12, 0x0a, 0x04, 0x00, 0x02, 0x01, 0x0d, 0x10, 0x03, 0x17, 0x19, 0x16,
    0x4e, 0x71, 0x66, 0x2b, 0x59, 0x61, 0x48, 0x5e, 0x31, 0x25, 0x39, 0x3b, 0x5f, 0x23, 0x7b, 0x6b,
    0x45, 0x33, 0x63, 0x4c, 0x65, 0x26, 0x60, 0x62, 0x2e, 0x79, 0x64, 0x4a, 0x5a, 0x55, 0x40, 0x24,
    0x21, 0x34, 0x4d, 0x7a, 0x53, 0x47, 0x3f, 0x29, 0x28, 0x30, 0x78, 0x6f, 0x58, 0x38, 0x43, 0x73,
    0x49, 0x69, 0x68, 0x2c, 0x52, 0x5c, 0x2a, 0x32, 0x6a, 0x7d, 0x22, 0x2f, 0x51, 0x2d, 0x44, 0x46,
    0x75, 0x77, 0x5b, 0x72, 0x36, 0x50, 0x7e, 0x20, 0x74, 0x3a, 0x42, 0x57, 0x70, 0x4b, 0x5d, 0x67,
    0x4f, 0x3d, 0x6c, 0x3e, 0x76, 0x37, 0x35, 0x54, 0x6d, 0x41, 0x6e, 0x3c, 0x7c, 0x56, 0x27, 0x7f,
    0xee, 0xe0, 0xad, 0xac, 0x8d, 0xcb, 0xa8, 0x83, 0xdc, 0xca, 0xbe, 0xcc, 0xb0, 0x84, 0xeb, 0xa6,
    0xb9, 0x8a, 0xb1, 0xfb, 0x82, 0xdd, 0xef, 0xfc, 0xf7, 0xb5, 0x99, 0x9a, 0x86, 0x96, 0x87, 0xd4,
    0xa0, 0xd5, 0xea, 0xd3, 0xe8, 0xde, 0xa7, 0xd1, 0xf9, 0x80, 0xba, 0xaf, 0xf8, 0xd7, 0xbb, 0xf1,
    0xb8, 0xc8, 0xf5, 0xab, 0xd8, 0xbc, 0xdb, 0xaa, 0xa5, 0xd2, 0xcd, 0xec, 0xfe, 0x8f, 0xa9, 0xf6,
    0x9b, 0x92, 0xc4, 0x8b, 0xf0, 0x91, 0xe2, 0xc0, 0xf4, 0xae, 0xc6, 0xff, 0xe1, 0x9c, 0xce, 0xe7,
    0xfd, 0xe3, 0xc5, 0x9f, 0xcf, 0x95, 0x98, 0xb2, 0xc1, 0x8c, 0xda, 0xe4, 0xd9, 0xc7, 0xb3, 0x81,
    0xe5, 0x90, 0x88, 0x85, 0xc2, 0xfa, 0x8e, 0xe9, 0xb7, 0xb4, 0x9d, 0xa3, 0xa2, 0xa1, 0xbf, 0xa4,
    0x94, 0x89, 0x9e, 0xdf, 0xd0, 0xc9, 0xf2, 0x97, 0x93, 0xe6, 0xbd, 0xf3, 0xb6, 0xed, 0xd6, 0xc3};

static uint32_t rng_state = 0xdeadbeefUL;

void kg_reset_rng(void)
{
    rng_state = 0xdeadbeefUL;
}

uint32_t kg_next_random(uint32_t tick, uint32_t upper_bound)
{
    uint64_t product;
    uint64_t difference;
    uint32_t low, high, carry, borrow, delta;

    if (!upper_bound)
        return 1;

    /* mul, sbb, cmc, adc at 0x401540. */
    product = (uint64_t)tick * (uint64_t)rng_state;
    low = (uint32_t)product;
    high = (uint32_t)(product >> 32);
    carry = high != 0;
    difference = (uint64_t)low + carry;
    borrow = (uint64_t)high < difference;
    delta = high - low - carry; /* wrap to 32 bits, as in x86 */
    rng_state += delta + (borrow ? 0UL : 1UL);

    /* remainder of delta / upper_bound, then increment once. */
    return (delta % upper_bound) + 1UL;
}

void kg_make_serial(uint32_t random, char serial[17], char authorization[8])
{
    static const unsigned char defaults[14] = {0, 0, 0, 0, 3, 0, 12, 0, 0, 0, 0, 17, 0, 7};
    unsigned char index[14];
    char raw_serial[14];
    char raw_auth[6];
    uint32_t value, seed, state;
    unsigned int i;
    int j;

    value = random + 0x0e000000UL - 0x2076UL;
    value = value + (17UL - (value % 17UL)) + 0x2076UL;

    memcpy(index, defaults, sizeof(index));
    index[0] = (unsigned char)(value & 31);
    index[7] = (unsigned char)((value >> 5) & 31);
    index[12] = (unsigned char)((value >> 10) & 31);
    index[2] = (unsigned char)((value >> 15) & 31);
    index[5] = (unsigned char)((value >> 20) & 31);

    state = 0xd5bfea1cUL;
    for (i = 0; i < 14; ++i)
    {
        unsigned int letter = index[i] ^ ((state >> 27) & 31UL);
        raw_serial[i] = alphabet[letter];
        state ^= (uint32_t)letter;
        for (j = 5; j <= 30; j += 5)
        {
            seed = state << j;
            state ^= seed;
        }
    }

    for (i = 0; i < 6; ++i)
    {
        raw_auth[i] = alphabet[(state >> (5 * i)) & 31UL];
    }

    memcpy(serial, raw_serial, 4);
    serial[4] = '-';
    memcpy(serial + 5, raw_serial + 4, 5);
    serial[10] = '-';
    memcpy(serial + 11, raw_serial + 9, 5);
    serial[16] = '\0';

    memcpy(authorization, raw_auth, 3);
    authorization[3] = '-';
    memcpy(authorization + 4, raw_auth + 3, 3);
    authorization[7] = '\0';
}

/* original routine reads fixed offsets even when the fields are empty. */
int kg_make_activation_raw(const char serial[17], const char authorization[8], const char registration[7], char activation[16])
{
    unsigned char value[15];
    uint32_t code, place;
    int32_t rounds;
    unsigned int i;
    int j;

    if (!serial || !authorization || !registration || !activation ||
        strlen(registration) != 6)
    {
        return 0;
    }

    for (i = 0; i < 5; ++i)
    {
        value[i] = (unsigned char)authorization[i + 2];
    }
    for (i = 0; i < 10; ++i)
    {
        value[i + 5] = (unsigned char)serial[i + 6];
    }

    for (i = 0; i < 15; ++i)
    {
        value[i] = substitution[value[i]];
    }

    code = 0;
    place = 1;
    for (j = 5; j >= 0; --j)
    {
        /* deliberately use the original byte-to-number conversion. */
        code += ((uint32_t)(unsigned char)registration[j] - 0x30UL) * place;
        place *= 10UL;
    }
    rounds = ((int32_t)code) % 0x481L;
    if (rounds == 0)
    {
        rounds = 1;
    }

    /* original substitution loop always executes at least once. */
    j = 0;
    do
    {
        for (i = 0; i < 15; ++i)
        {
            value[i] = substitution[value[i]];
        }
        ++j;
    } while (j < rounds);

    for (i = 0; i < 15; ++i)
    {
        unsigned char byte = value[i];
        /* x86 signed jge/jle against '1' and 'Y'. */
        while ((signed char)byte < '1')
        {
            byte = (unsigned char)(byte + 0x17);
        }
        while ((signed char)byte > 'Y')
        {
            byte = (unsigned char)(byte - 0x17);
        }
        if (byte == '0') byte = '2';
        if (byte == 'O') byte = 'P';
        activation[i] = (char)byte;
    }
    activation[15] = '\0';
    return 1;
}

/* strict version remains available for callers outside the gui. */
int kg_make_activation(const char *serial, const char *authorization, const char *registration, char activation[16])
{
    if (!serial || !authorization || !registration || !activation || strlen(serial) != 16 || strlen(authorization) != 7 || strlen(registration) != 6)
        return 0;

    return kg_make_activation_raw(serial, authorization, registration, activation);
}
