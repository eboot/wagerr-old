/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_SCALAR_REPR_IMPL_H_
#define _SECP256K1_SCALAR_REPR_IMPL_H_

/* Limbs of the secp256k1 order. */
#define SECP256K1_N_0 ((uint32_t)0xD0364141UL)
#define SECP256K1_N_1 ((uint32_t)0xBFD25E8CUL)
#define SECP256K1_N_2 ((uint32_t)0xAF48A03BUL)
#define SECP256K1_N_3 ((uint32_t)0xBAAEDCE6UL)
#define SECP256K1_N_4 ((uint32_t)0xFFFFFFFEUL)
#define SECP256K1_N_5 ((uint32_t)0xFFFFFFFFUL)
#define SECP256K1_N_6 ((uint32_t)0xFFFFFFFFUL)
#define SECP256K1_N_7 ((uint32_t)0xFFFFFFFFUL)

/* Limbs of 2^256 minus the secp256k1 order. */
#define SECP256K1_N_C_0 (~SECP256K1_N_0 + 1)
#define SECP256K1_N_C_1 (~SECP256K1_N_1)
#define SECP256K1_N_C_2 (~SECP256K1_N_2)
#define SECP256K1_N_C_3 (~SECP256K1_N_3)
#define SECP256K1_N_C_4 (1)

/* Limbs of half the secp256k1 order. */
#define SECP256K1_N_H_0 ((uint32_t)0x681B20A0UL)
#define SECP256K1_N_H_1 ((uint32_t)0xDFE92F46UL)
#define SECP256K1_N_H_2 ((uint32_t)0x57A4501DUL)
#define SECP256K1_N_H_3 ((uint32_t)0x5D576E73UL)
#define SECP256K1_N_H_4 ((uint32_t)0xFFFFFFFFUL)
#define SECP256K1_N_H_5 ((uint32_t)0xFFFFFFFFUL)
#define SECP256K1_N_H_6 ((uint32_t)0xFFFFFFFFUL)
#define SECP256K1_N_H_7 ((uint32_t)0x7FFFFFFFUL)

SECP256K1_INLINE static void secp256k1_scalar_clear(secp256k1_scalar_t *r) {
    r->d[0] = 0;
    r->d[1] = 0;
    r->d[2] = 0;
    r->d[3] = 0;
    r->d[4] = 0;
    r->d[5] = 0;
    r->d[6] = 0;
    r->d[7] = 0;
}

SECP256K1_INLINE static int secp256k1_scalar_get_bits(const secp256k1_scalar_t *a, int offset, int count) {
    VERIFY_CHECK((offset + count - 1) / 32 == offset / 32);
    return (a->d[offset / 32] >> (offset % 32)) & ((1 << count) - 1);
}

SECP256K1_INLINE static int secp256k1_scalar_check_overflow(const secp256k1_scalar_t *a) {
    int yes = 0;
    int no = 0;
    no |= (a->d[7] < SECP256K1_N_7); /* No need for a > check. */
    no |= (a->d[6] < SECP256K1_N_6); /* No need for a > check. */
    no |= (a->d[5] < SECP256K1_N_5); /* No need for a > check. */
    no |= (a->d[4] < SECP256K1_N_4);
    yes |= (a->d[4] > SECP256K1_N_4) & ~no;
    no |= (a->d[3] < SECP256K1_N_3) & ~yes;
    yes |= (a->d[3] > SECP256K1_N_3) & ~no;
    no |= (a->d[2] < SECP256K1_N_2) & ~yes;
    yes |= (a->d[2] > SECP256K1_N_2) & ~no;
    no |= (a->d[1] < SECP256K1_N_1) & ~yes;
    yes |= (a->d[1] > SECP256K1_N_1) & ~no;
    yes |= (a->d[0] >= SECP256K1_N_0) & ~no;
    return yes;
}

SECP256K1_INLINE static int secp256k1_scalar_reduce(secp256k1_scalar_t *r, uint32_t overflow) {
    VERIFY_CHECK(overflow <= 1);
    uint64_t t = (uint64_t)r->d[0] + overflow * SECP256K1_N_C_0;
    r->d[0] = t & 0xFFFFFFFFUL; t >>= 32;
    t += (uint64_t)r->d[1] + overflow * SECP256K1_N_C_1;
    r->d[1] = t & 0xFFFFFFFFUL; t >>= 32;
    t += (uint64_t)r->d[2] + overflow * SECP256K1_N_C_2;
    r->d[2] = t & 0xFFFFFFFFUL; t >>= 32;
    t += (uint64_t)r->d[3] + overflow * SECP256K1_N_C_3;
    r->d[3] = t & 0xFFFFFFFFUL; t >>= 32;
    t += (uint64_t)r->d[4] + overflow * SECP256K1_N_C_4;
    r->d[4] = t & 0xFFFFFFFFUL; t >>= 32;
    t += (uint64_t)r->d[5];
    r->d[5] = t & 0xFFFFFFFFUL; t >>= 32;
    t += (uint64_t)r->d[6];
    r->d[6] = t & 0xFFFFFFFFUL; t >>= 32;
    t += (uint64_t)r->d[7];
    r->d[7] = t & 0xFFFFFFFFUL;
    return overflow;
}

static void secp256k1_scalar_add(secp256k1_scalar_t *r, const secp256k1_scalar_t *a, const secp256k1_scalar_t *b) {
    uint64_t t = (uint64_t)a->d[0] + b->d[0];
    r->d[0] = t & 0xFFFFFFFFULL; t >>= 32;
    t += (uint64_t)a->d[1] + b->d[1];
    r->d[1] = t & 0xFFFFFFFFULL; t >>= 32;
    t += (uint64_t)a->d[2] + b->d[2];
    r->d[2] = t & 0xFFFFFFFFULL; t >>= 32;
    t += (uint64_t)a->d[3] + b->d[3];
    r->d[3] = t & 0xFFFFFFFFULL; t >>= 32;
    t += (uint64_t)a->d[4] + b->d[4];
    r->d[4] = t & 0xFFFFFFFFULL; t >>= 32;
    t += (uint64_t)a->d[5] + b->d[5];
    r->d[5] = t & 0xFFFFFFFFULL; t >>= 32;
    t += (uint64_t)a->d[6] + b->d[6];
    r->d[6] = t & 0xFFFFFFFFULL; t >>= 32;
    t += (uint64_t)a->d[7] + b->d[7];
    r->d[7] = t & 0xFFFFFFFFULL; t >>= 32;
    secp256k1_scalar_reduce(r, t + secp256k1_scalar_check_overflow(r));
}

static void secp256k1_scalar_set_b32(secp256k1_scalar_t *r, const unsigned char *b32, int *overflow) {
    r->d[0] = (uint32_t)b32[31] | (uint32_t)b32[30] << 8 | (uint32_t)b32[29] << 16 | (uint32_t)b32[28] << 24;
    r->d[1] = (uint32_t)b32[27] | (uint32_t)b32[26] << 8 | (uint32_t)b32[25] << 16 | (uint32_t)b32[24] << 24;
    r->d[2] = (uint32_t)b32[23] | (uint32_t)b32[22] << 8 | (uint32_t)b32[21] << 16 | (uint32_t)b32[20] << 24;
    r->d[3] = (uint32_t)b32[19] | (uint32_t)b32[18] << 8 | (uint32_t)b32[17] << 16 | (uint32_t)b32[16] << 24;
    r->d[4] = (uint32_t)b32[15] | (uint32_t)b32[14] << 8 | (uint32_t)b32[13] << 16 | (uint32_t)b32[12] << 24;
    r->d[5] = (uint32_t)b32[11] | (uint32_t)b32[10] << 8 | (uint32_t)b32[9] << 16 | (uint32_t)b32[8] << 24;
    r->d[6] = (uint32_t)b32[7] | (uint32_t)b32[6] << 8 | (uint32_t)b32[5] << 16 | (uint32_t)b32[4] << 24;
    r->d[7] = (uint32_t)b32[3] | (uint32_t)b32[2] << 8 | (uint32_t)b32[1] << 16 | (uint32_t)b32[0] << 24;
    int over = secp256k1_scalar_reduce(r, secp256k1_scalar_check_overflow(r));
    if (overflow) {
        *overflow = over;
    }
}

static void secp256k1_scalar_get_b32(unsigned char *bin, const secp256k1_scalar_t* a) {
    bin[0] = a->d[7] >> 24; bin[1] = a->d[7] >> 16; bin[2] = a->d[7] >> 8; bin[3] = a->d[7];
    bin[4] = a->d[6] >> 24; bin[5] = a->d[6] >> 16; bin[6] = a->d[6] >> 8; bin[7] = a->d[6];
    bin[8] = a->d[5] >> 24; bin[9] = a->d[5] >> 16; bin[10] = a->d[5] >> 8; bin[11] = a->d[5];
    bin[12] = a->d[4] >> 24; bin[13] = a->d[4] >> 16; bin[14] = a->d[4] >> 8; bin[15] = a->d[4];
    bin[16] = a->d[3] >> 24; bin[17] = a->d[3] >> 16; bin[18] = a->d[3] >> 8; bin[19] = a->d[3];
    bin[20] = a->d[2] >> 24; bin[21] = a->d[2] >> 16; bin[22] = a->d[2] >> 8; bin[23] = a->d[2];
    bin[24] = a->d[1] >> 24; bin[25] = a->d[1] >> 16; bin[26] = a->d[1] >> 8; bin[27] = a->d[1];
    bin[28] = a->d[0] >> 24; bin[29] = a->d[0] >> 16; bin[30] = a->d[0] >> 8; bin[31] = a->d[0];
}

SECP256K1_INLINE static int secp256k1_scalar_is_zero(const secp256k1_scalar_t *a) {
    return (a->d[0] | a->d[1] | a->d[2] | a->d[3] | a->d[4] | a->d[5] | a->d[6] | a->d[7]) == 0;
}

static void secp256k1_scalar_negate(secp256k1_scalar_t *r, const secp256k1_scalar_t *a) {
    uint32_t nonzero = 0xFFFFFFFFUL * (secp256k1_scalar_is_zero(a) == 0);
    uint64_t t = (uint64_t)(~a->d[0]) + SECP256K1_N_0 + 1;
    r->d[0] = t & nonzero; t >>= 32;
    t += (uint64_t)(~a->d[1]) + SECP256K1_N_1;
    r->d[1] = t & nonzero; t >>= 32;
    t += (uint64_t)(~a->d[2]) + SECP256K1_N_2;
    r->d[2] = t & nonzero; t >>= 32;
    t += (uint64_t)(~a->d[3]) + SECP256K1_N_3;
    r->d[3] = t & nonzero; t >>= 32;
    t += (uint64_t)(~a->d[4]) + SECP256K1_N_4;
    r->d[4] = t & nonzero; t >>= 32;
    t += (uint64_t)(~a->d[5]) + SECP256K1_N_5;
    r->d[5] = t & nonzero; t >>= 32;
    t += (uint64_t)(~a->d[6]) + SECP256K1_N_6;
    r->d[6] = t & nonzero; t >>= 32;
    t += (uint64_t)(~a->d[7]) + SECP256K1_N_7;
    r->d[7] = t & nonzero;
}

SECP256K1_INLINE static int secp256k1_scalar_is_one(const secp256k1_scalar_t *a) {
    return ((a->d[0] ^ 1) | a->d[1] | a->d[2] | a->d[3] | a->d[4] | a->d[5] | a->d[6] | a->d[7]) == 0;
}

static int secp256k1_scalar_is_high(const secp256k1_scalar_t *a) {
    int yes = 0;
    int no = 0;
    no |= (a->d[7] < SECP256K1_N_H_7);
    yes |= (a->d[7] > SECP256K1_N_H_7) & ~no;
    no |= (a->d[6] < SECP256K1_N_H_6) & ~yes; /* No need for a > check. */
    no |= (a->d[5] < SECP256K1_N_H_5) & ~yes; /* No need for a > check. */
    no |= (a->d[4] < SECP256K1_N_H_4) & ~yes; /* No need for a > check. */
    no |= (a->d[3] < SECP256K1_N_H_3) & ~yes;
    yes |= (a->d[3] > SECP256K1_N_H_3) & ~no;
    no |= (a->d[2] < SECP256K1_N_H_2) & ~yes;
    yes |= (a->d[2] > SECP256K1_N_H_2) & ~no;
    no |= (a->d[1] < SECP256K1_N_H_1) & ~yes;
    yes |= (a->d[1] > SECP256K1_N_H_1) & ~no;
    yes |= (a->d[0] > SECP256K1_N_H_0) & ~no;
    return yes;
}

/* Inspired by the macros in OpenSSL's crypto/bn/asm/x86_64-gcc.c. */

/** Add a*b to the number defined by (c0,c1,c2). c2 must never overflow. */
#define muladd(a,b) { \
    uint32_t tl, th; \
    { \
        uint64_t t = (uint64_t)a * b; \
        th = t >> 32;         /* at most 0xFFFFFFFE */ \
        tl = t; \
    } \
    c0 += tl;                 /* overflow is handled on the next line */ \
    th += (c0 < tl) ? 1 : 0;  /* at most 0xFFFFFFFF */ \
    c1 += th;                 /* overflow is handled on the next line */ \
    c2 += (c1 < th) ? 1 : 0;  /* never overflows by contract (verified in the next line) */ \
    VERIFY_CHECK((c1 >= th) || (c2 != 0)); \
}

/** Add a*b to the number defined by (c0,c1). c1 must never overflow. */
#define muladd_fast(a,b) { \
    uint32_t tl, th; \
    { \
        uint64_t t = (uint64_t)a * b; \
        th = t >> 32;         /* at most 0xFFFFFFFE */ \
        tl = t; \
    } \
    c0 += tl;                 /* overflow is handled on the next line */ \
    th += (c0 < tl) ? 1 : 0;  /* at most 0xFFFFFFFF */ \
    c1 += th;                 /* never overflows by contract (verified in the next line) */ \
    VERIFY_CHECK(c1 >= th); \
}

/** Add 2*a*b to the number defined by (c0,c1,c2). c2 must never overflow. */
#define muladd2(a,b) { \
    uint32_t tl, th; \
    { \
        uint64_t t = (uint64_t)a * b; \
        th = t >> 32;               /* at most 0xFFFFFFFE */ \
        tl = t; \
    } \
    uint32_t th2 = th + th;         /* at most 0xFFFFFFFE (in case th was 0x7FFFFFFF) */ \
    c2 += (th2 < th) ? 1 : 0;       /* never overflows by contract (verified the next line) */ \
    VERIFY_CHECK((th2 >= th) || (c2 != 0)); \
    uint32_t tl2 = tl + tl;         /* at most 0xFFFFFFFE (in case the lowest 63 bits of tl were 0x7FFFFFFF) */ \
    th2 += (tl2 < tl) ? 1 : 0;      /* at most 0xFFFFFFFF */ \
    c0 += tl2;                      /* overflow is handled on the next line */ \
    th2 += (c0 < tl2) ? 1 : 0;      /* second overflow is handled on the next line */ \
    c2 += (c0 < tl2) & (th2 == 0);  /* never overflows by contract (verified the next line) */ \
    VERIFY_CHECK((c0 >= tl2) || (th2 != 0) || (c2 != 0)); \
    c1 += th2;                      /* overflow is handled on the next line */ \
    c2 += (c1 < th2) ? 1 : 0;       /* never overflows by contract (verified the next line) */ \
    VERIFY_CHECK((c1 >= th2) || (c2 != 0)); \
}

/** Add a to the number defined by (c0,c1,c2). c2 must never overflow. */
#define sumadd(a) { \
    c0 += (a);                  /* overflow is handled on the next line */ \
    unsigned int over = (c0 < (a)) ? 1 : 0; \
    c1 += over;                 /* overflow is handled on the next line */ \
    c2 += (c1 < over) ? 1 : 0;  /* never overflows by contract */ \
}

/** Add a to the number defined by (c0,c1). c1 must never overflow, c2 must be zero. */
#define sumadd_fast(a) { \
    c0 += (a);                 /* overflow is handled on the next line */ \
    c1 += (c0 < (a)) ? 1 : 0;  /* never overflows by contract (verified the next line) */ \
    VERIFY_CHECK((c1 != 0) | (c0 >= (a))); \
    VERIFY_CHECK(c2 == 0); \
}

/** Extract the lowest 32 bits of (c0,c1,c2) into n, and left shift the number 32 bits. */
#define extract(n) { \
    (n) = c0; \
    c0 = c1; \
    c1 = c2; \
    c2 = 0; \
}

/** Extract the lowest 32 bits of (c0,c1,c2) into n, and left shift the number 32 bits. c2 is required to be zero. */
#define extract_fast(n) { \
    (n) = c0; \
    c0 = c1; \
    c1 = 0; \
    VERIFY_CHECK(c2 == 0); \
}

static void secp256k1_scalar_reduce_512(secp256k1_scalar_t *r, const uint32_t *l) {
    uint32_t n0 = l[8], n1 = l[9], n2 = l[10], n3 = l[11], n4 = l[12], n5 = l[13], n6 = l[14], n7 = l[15];

    /* 96 bit accumulator. */
    uint32_t c0, c1, c2;

    /* Reduce 512 bits into 385. */
    /* m[0..12] = l[0..7] + n[0..7] * SECP256K1_N_C. */
    c0 = l[0]; c1 = 0; c2 = 0;
    muladd_fast(n0, SECP256K1_N_C_0);
    uint32_t m0; extract_fast(m0);
    sumadd_fast(l[1]);
    muladd(n1, SECP256K1_N_C_0);
    muladd(n0, SECP256K1_N_C_1);
    uint32_t m1; extract(m1);
    sumadd(l[2]);
    muladd(n2, SECP256K1_N_C_0);
    muladd(n1, SECP256K1_N_C_1);
    muladd(n0, SECP256K1_N_C_2);
    uint32_t m2; extract(m2);
    sumadd(l[3]);
    muladd(n3, SECP256K1_N_C_0);
    muladd(n2, SECP256K1_N_C_1);
    muladd(n1, SECP256K1_N_C_2);
    muladd(n0, SECP256K1_N_C_3);
    uint32_t m3; extract(m3);
    sumadd(l[4]);
    muladd(n4, SECP256K1_N_C_0);
    muladd(n3, SECP256K1_N_C_1);
    muladd(n2, SECP256K1_N_C_2);
    muladd(n1, SECP256K1_N_C_3);
    sumadd(n0);
    uint32_t m4; extract(m4);
    sumadd(l[5]);
    muladd(n5, SECP256K1_N_C_0);
    muladd(n4, SECP256K1_N_C_1);
    muladd(n3, SECP256K1_N_C_2);
    muladd(n2, SECP256K1_N_C_3);
    sumadd(n1);
    uint32_t m5; extract(m5);
    sumadd(l[6]);
    muladd(n6, SECP256K1_N_C_0);
    muladd(n5, SECP256K1_N_C_1);
    muladd(n4, SECP256K1_N_C_2);
    muladd(n3, SECP256K1_N_C_3);
    sumadd(n2);
    uint32_t m6; extract(m6);
    sumadd(l[7]);
    muladd(n7, SECP256K1_N_C_0);
    muladd(n6, SECP256K1_N_C_1);
    muladd(n5, SECP256K1_N_C_2);
    muladd(n4, SECP256K1_N_C_3);
    sumadd(n3);
    uint32_t m7; extract(m7);
    muladd(n7, SECP256K1_N_C_1);
    muladd(n6, SECP256K1_N_C_2);
    muladd(n5, SECP256K1_N_C_3);
    sumadd(n4);
    uint32_t m8; extract(m8);
    muladd(n7, SECP256K1_N_C_2);
    muladd(n6, SECP256K1_N_C_3);
    sumadd(n5);
    uint32_t m9; extract(m9);
    muladd(n7, SECP256K1_N_C_3);
    sumadd(n6);
    uint32_t m10; extract(m10);
    sumadd_fast(n7);
    uint32_t m11; extract_fast(m11);
    VERIFY_CHECK(c0 <= 1);
    uint32_t m12 = c0;

    /* Reduce 385 bits into 258. */
    /* p[0..8] = m[0..7] + m[8..12] * SECP256K1_N_C. */
    c0 = m0; c1 = 0; c2 = 0;
    muladd_fast(m8, SECP256K1_N_C_0);
    uint32_t p0; extract_fast(p0);
    sumadd_fast(m1);
    muladd(m9, SECP256K1_N_C_0);
    muladd(m8, SECP256K1_N_C_1);
    uint32_t p1; extract(p1);
    sumadd(m2);
    muladd(m10, SECP256K1_N_C_0);
    muladd(m9, SECP256K1_N_C_1);
    muladd(m8, SECP256K1_N_C_2);
    uint32_t p2; extract(p2);
    sumadd(m3);
    muladd(m11, SECP256K1_N_C_0);
    muladd(m10, SECP256K1_N_C_1);
    muladd(m9, SECP256K1_N_C_2);
    muladd(m8, SECP256K1_N_C_3);
    uint32_t p3; extract(p3);
    sumadd(m4);
    muladd(m12, SECP256K1_N_C_0);
    muladd(m11, SECP256K1_N_C_1);
    muladd(m10, SECP256K1_N_C_2);
    muladd(m9, SECP256K1_N_C_3);
    sumadd(m8);
    uint32_t p4; extract(p4);
    sumadd(m5);
    muladd(m12, SECP256K1_N_C_1);
    muladd(m11, SECP256K1_N_C_2);
    muladd(m10, SECP256K1_N_C_3);
    sumadd(m9);
    uint32_t p5; extract(p5);
    sumadd(m6);
    muladd(m12, SECP256K1_N_C_2);
    muladd(m11, SECP256K1_N_C_3);
    sumadd(m10);
    uint32_t p6; extract(p6);
    sumadd_fast(m7);
    muladd_fast(m12, SECP256K1_N_C_3);
    sumadd_fast(m11);
    uint32_t p7; extract_fast(p7);
    uint32_t p8 = c0 + m12;
    VERIFY_CHECK(p8 <= 2);

    /* Reduce 258 bits into 256. */
    /* r[0..7] = p[0..7] + p[8] * SECP256K1_N_C. */
    uint64_t c = p0 + (uint64_t)SECP256K1_N_C_0 * p8;
    r->d[0] = c & 0xFFFFFFFFUL; c >>= 32;
    c += p1 + (uint64_t)SECP256K1_N_C_1 * p8;
    r->d[1] = c & 0xFFFFFFFFUL; c >>= 32;
    c += p2 + (uint64_t)SECP256K1_N_C_2 * p8;
    r->d[2] = c & 0xFFFFFFFFUL; c >>= 32;
    c += p3 + (uint64_t)SECP256K1_N_C_3 * p8;
    r->d[3] = c & 0xFFFFFFFFUL; c >>= 32;
    c += p4 + (uint64_t)p8;
    r->d[4] = c & 0xFFFFFFFFUL; c >>= 32;
    c += p5;
    r->d[5] = c & 0xFFFFFFFFUL; c >>= 32;
    c += p6;
    r->d[6] = c & 0xFFFFFFFFUL; c >>= 32;
    c += p7;
    r->d[7] = c & 0xFFFFFFFFUL; c >>= 32;

    /* Final reduction of r. */
    secp256k1_scalar_reduce(r, c + secp256k1_scalar_check_overflow(r));
}

static void secp256k1_scalar_mul(secp256k1_scalar_t *r, const secp256k1_scalar_t *a, const secp256k1_scalar_t *b) {
    /* 96 bit accumulator. */
    uint32_t c0 = 0, c1 = 0, c2 = 0;

    uint32_t l[16];

    /* l[0..15] = a[0..7] * b[0..7]. */
    muladd_fast(a->d[0], b->d[0]);
    extract_fast(l[0]);
    muladd(a->d[0], b->d[1]);
    muladd(a->d[1], b->d[0]);
    extract(l[1]);
    muladd(a->d[0], b->d[2]);
    muladd(a->d[1], b->d[1]);
    muladd(a->d[2], b->d[0]);
    extract(l[2]);
    muladd(a->d[0], b->d[3]);
    muladd(a->d[1], b->d[2]);
    muladd(a->d[2], b->d[1]);
    muladd(a->d[3], b->d[0]);
    extract(l[3]);
    muladd(a->d[0], b->d[4]);
    muladd(a->d[1], b->d[3]);
    muladd(a->d[2], b->d[2]);
    muladd(a->d[3], b->d[1]);
    muladd(a->d[4], b->d[0]);
    extract(l[4]);
    muladd(a->d[0], b->d[5]);
    muladd(a->d[1], b->d[4]);
    muladd(a->d[2], b->d[3]);
    muladd(a->d[3], b->d[2]);
    muladd(a->d[4], b->d[1]);
    muladd(a->d[5], b->d[0]);
    extract(l[5]);
    muladd(a->d[0], b->d[6]);
    muladd(a->d[1], b->d[5]);
    muladd(a->d[2], b->d[4]);
    muladd(a->d[3], b->d[3]);
    muladd(a->d[4], b->d[2]);
    muladd(a->d[5], b->d[1]);
    muladd(a->d[6], b->d[0]);
    extract(l[6]);
    muladd(a->d[0], b->d[7]);
    muladd(a->d[1], b->d[6]);
    muladd(a->d[2], b->d[5]);
    muladd(a->d[3], b->d[4]);
    muladd(a->d[4], b->d[3]);
    muladd(a->d[5], b->d[2]);
    muladd(a->d[6], b->d[1]);
    muladd(a->d[7], b->d[0]);
    extract(l[7]);
    muladd(a->d[1], b->d[7]);
    muladd(a->d[2], b->d[6]);
    muladd(a->d[3], b->d[5]);
    muladd(a->d[4], b->d[4]);
    muladd(a->d[5], b->d[3]);
    muladd(a->d[6], b->d[2]);
    muladd(a->d[7], b->d[1]);
    extract(l[8]);
    muladd(a->d[2], b->d[7]);
    muladd(a->d[3], b->d[6]);
    muladd(a->d[4], b->d[5]);
    muladd(a->d[5], b->d[4]);
    muladd(a->d[6], b->d[3]);
    muladd(a->d[7], b->d[2]);
    extract(l[9]);
    muladd(a->d[3], b->d[7]);
    muladd(a->d[4], b->d[6]);
    muladd(a->d[5], b->d[5]);
    muladd(a->d[6], b->d[4]);
    muladd(a->d[7], b->d[3]);
    extract(l[10]);
    muladd(a->d[4], b->d[7]);
    muladd(a->d[5], b->d[6]);
    muladd(a->d[6], b->d[5]);
    muladd(a->d[7], b->d[4]);
    extract(l[11]);
    muladd(a->d[5], b->d[7]);
    muladd(a->d[6], b->d[6]);
    muladd(a->d[7], b->d[5]);
    extract(l[12]);
    muladd(a->d[6], b->d[7]);
    muladd(a->d[7], b->d[6]);
    extract(l[13]);
    muladd_fast(a->d[7], b->d[7]);
    extract_fast(l[14]);
    VERIFY_CHECK(c1 == 0);
    l[15] = c0;

    secp256k1_scalar_reduce_512(r, l);
}

static void secp256k1_scalar_sqr(secp256k1_scalar_t *r, const secp256k1_scalar_t *a) {
    /* 96 bit accumulator. */
    uint32_t c0 = 0, c1 = 0, c2 = 0;

    uint32_t l[16];

    /* l[0..15] = a[0..7]^2. */
    muladd_fast(a->d[0], a->d[0]);
    extract_fast(l[0]);
    muladd2(a->d[0], a->d[1]);
    extract(l[1]);
    muladd2(a->d[0], a->d[2]);
    muladd(a->d[1], a->d[1]);
    extract(l[2]);
    muladd2(a->d[0], a->d[3]);
    muladd2(a->d[1], a->d[2]);
    extract(l[3]);
    muladd2(a->d[0], a->d[4]);
    muladd2(a->d[1], a->d[3]);
    muladd(a->d[2], a->d[2]);
    extract(l[4]);
    muladd2(a->d[0], a->d[5]);
    muladd2(a->d[1], a->d[4]);
    muladd2(a->d[2], a->d[3]);
    extract(l[5]);
    muladd2(a->d[0], a->d[6]);
    muladd2(a->d[1], a->d[5]);
    muladd2(a->d[2], a->d[4]);
    muladd(a->d[3], a->d[3]);
    extract(l[6]);
    muladd2(a->d[0], a->d[7]);
    muladd2(a->d[1], a->d[6]);
    muladd2(a->d[2], a->d[5]);
    muladd2(a->d[3], a->d[4]);
    extract(l[7]);
    muladd2(a->d[1], a->d[7]);
    muladd2(a->d[2], a->d[6]);
    muladd2(a->d[3], a->d[5]);
    muladd(a->d[4], a->d[4]);
    extract(l[8]);
    muladd2(a->d[2], a->d[7]);
    muladd2(a->d[3], a->d[6]);
    muladd2(a->d[4], a->d[5]);
    extract(l[9]);
    muladd2(a->d[3], a->d[7]);
    muladd2(a->d[4], a->d[6]);
    muladd(a->d[5], a->d[5]);
    extract(l[10]);
    muladd2(a->d[4], a->d[7]);
    muladd2(a->d[5], a->d[6]);
    extract(l[11]);
    muladd2(a->d[5], a->d[7]);
    muladd(a->d[6], a->d[6]);
    extract(l[12]);
    muladd2(a->d[6], a->d[7]);
    extract(l[13]);
    muladd_fast(a->d[7], a->d[7]);
    extract_fast(l[14]);
    VERIFY_CHECK(c1 == 0);
    l[15] = c0;

    secp256k1_scalar_reduce_512(r, l);
}

#undef sumadd
#undef sumadd_fast
#undef muladd
#undef muladd_fast
#undef muladd2
#undef extract
#undef extract_fast

#endif
