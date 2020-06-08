// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ripemd160.h"

#include "common.h"

#include <string.h>

// Internal implementation code.
namespace
{
/// Internal RIPEMD-160 implementation.
namespace ripemd160
{
uint32_t inline f1(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
uint32_t inline f2(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
uint32_t inline f3(uint32_t x, uint32_t y, uint32_t z) { return (x | ~y) ^ z; }
uint32_t inline f4(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
uint32_t inline f5(uint32_t x, uint32_t y, uint32_t z) { return x ^ (y | ~z); }

/** Initialize RIPEMD-160 state. */
void inline Initialize(uint32_t* s)
{
    s[0] = 0x67452301ul;
    s[1] = 0xEFCDAB89ul;
    s[2] = 0x98BADCFEul;
    s[3] = 0x10325476ul;
    s[4] = 0xC3D2E1F0ul;
}

uint32_t inline rol(uint32_t x, int i) { return (x << i) | (x >> (32 - i)); }

void inline Round(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t f, uint32_t x, uint32_t k, int r)
{
    a = rol(a + f + x + k, r) + e;
    c = rol(c, 10);
}

void inline R11(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, int r) { Round(a, b, c, d, e, f1(b, c, d), x, 0, r); }
void inline R21(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, int r) { Round(a, b, c, d, e, f2(b, c, d), x, 0x5A827999ul, r); }
void inline R31(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, int r) { Round(a, b, c, d, e, f3(b, c, d), x, 0x6ED9EBA1ul, r); }
void inline R41(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, int r) { Round(a, b, c, d, e, f4(b, c, d), x, 0x8F1BBCDCul, r); }
void inline R51(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, int r) { Round(a, b, c, d, e, f5(b, c, d), x, 0xA953FD4Eul, r); }

void inline R12(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, int r) { Round(a, b, c, d, e, f5(b, c, d), x, 0x50A28BE6ul, r); }
void inline R22(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, int r) { Round(a, b, c, d, e, f4(b, c, d), x, 0x5C4DD124ul, r); }
void inline R32(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, int r) { Round(a, b, c, d, e, f3(b, c, d), x, 0x6D703EF3ul, r); }
void inline R42(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, int r) { Round(a, b, c, d, e, f2(b, c, d), x, 0x7A6D76E9ul, r); }
void inline R52(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, int r) { Round(a, b, c, d, e, f1(b, c, d), x, 0, r); }

/** Perform a RIPEMD-160 transformation, processing a 64-byte chunk. */
void Transform(uint32_t* s, const unsigned char* chunk)
{
    uint32_t a1 = s[0], b1 = s[1], c1 = s[2], d1 = s[3], e1 = s[4];
    uint32_t a2 = a1, b2 = b1, c2 = c1, d2 = d1, e2 = e1;
    uint32_t w0 = ReadLE32(chunk + 0), w1 = ReadLE32(chunk + 4), w2 = ReadLE32(chunk + 8), w3 = ReadLE32(chunk + 12);
    uint32_t w4 = ReadLE32(chunk + 16), w5 = ReadLE32(chunk + 20), w6 = ReadLE32(chunk + 24), w7 = ReadLE32(chunk + 28);
    uint32_t w8 = ReadLE32(chunk + 32), w9 = ReadLE32(chunk + 36), w10 = ReadLE32(chunk + 40), w11 = ReadLE32(chunk + 44);
    uint32_t w12 = ReadLE32(chunk + 48), w13 = ReadLE32(chunk + 52), w14 = ReadLE32(chunk + 56), w15 = ReadLE32(chunk + 60);

    R11(a1, b1, c1, d1, e1, w0, 11);
    R12(a2, b2, c2, d2, e2, w5, 8);
    R11(e1, a1, b1, c1, d1, w1, 14);
    R12(e2, a2, b2, c2, d2, w14, 9);
    R11(d1, e1, a1, b1, c1, w2, 15);
    R12(d2, e2, a2, b2, c2, w7, 9);
    R11(c1, d1, e1, a1, b1, w3, 12);
    R12(c2, d2, e2, a2, b2, w0, 11);
    R11(b1, c1, d1, e1, a1, w4, 5);
    R12(b2, c2, d2, e2, a2, w9, 13);
    R11(a1, b1, c1, d1, e1, w5, 8);
    R12(a2, b2, c2, d2, e2, w2, 15);
    R11(e1, a1, b1, c1, d1, w6, 7);
    R12(e2, a2, b2, c2, d2, w11, 15);
    R11(d1, e1, a1, b1, c1, w7, 9);
    R12(d2, e2, a2, b2, c2, w4, 5);
    R11(c1, d1, e1, a1, b1, w8, 11);
    R12(c2, d2, e2, a2, b2, w13, 7);
    R11(b1, c1, d1, e1, a1, w9, 13);
    R12(b2, c2, d2, e2, a2, w6, 7);
    R11(a1, b1, c1, d1, e1, w10, 14);
    R12(a2, b2, c2, d2, e2, w15, 8);
    R11(e1, a1, b1, c1, d1, w11, 15);
    R12(e2, a2, b2, c2, d2, w8, 11);
    R11(d1, e1, a1, b1, c1, w12, 6);
    R12(d2, e2, a2, b2, c2, w1, 14);
    R11(c1, d1, e1, a1, b1, w13, 7);
    R12(c2, d2, e2, a2, b2, w10, 14);
    R11(b1, c1, d1, e1, a1, w14, 9);
    R12(b2, c2, d2, e2, a2, w3, 12);
    R11(a1, b1, c1, d1, e1, w15, 8);
    R12(a2, b2, c2, d2, e2, w12, 6);

    R21(e1, a1, b1, c1, d1, w7, 7);
    R22(e2, a2, b2, c2, d2, w6, 9);
    R21(d1, e1, a1, b1, c1, w4, 6);
    R22(d2, e2, a2, b2, c2, w11, 13);
    R21(c1, d1, e1, a1, b1, w13, 8);
    R22(c2, d2, e2, a2, b2, w3, 15);
    R21(b1, c1, d1, e1, a1, w1, 13);
    R22(b2, c2, d2, e2, a2, w7, 7);
    R21(a1, b1, c1, d1, e1, w10, 11);
    R22(a2, b2, c2, d2, e2, w0, 12);
    R21(e1, a1, b1, c1, d1, w6, 9);
    R22(e2, a2, b2, c2, d2, w13, 8);
    R21(d1, e1, a1, b1, c1, w15, 7);
    R22(d2, e2, a2, b2, c2, w5, 9);
    R21(c1, 