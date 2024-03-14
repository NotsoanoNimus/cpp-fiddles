#ifndef TINYMT64_H
#define TINYMT64_H
/**
 * @file tinymt64.h
 *
 * @brief Tiny Mersenne Twister only 127 bit internal state
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (The University of Tokyo)
 *
 *  Copyright (c) 2011, 2013 Mutsuo Saito, Makoto Matsumoto,
 *  Hiroshima University and The University of Tokyo.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *      * Neither the name of the Hiroshima University nor the names of
 *        its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written
 *        permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdint.h>
#include <inttypes.h>

#define TINYMT64_MEXP 127
#define TINYMT64_SH0 12
#define TINYMT64_SH1 11
#define TINYMT64_SH8 8
#define TINYMT64_MASK UINT64_C(0x7fffffffffffffff)
#define TINYMT64_MUL (1.0 / 9007199254740992.0)

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * tinymt64 internal state vector and parameters
 */
struct TINYMT64_T {
    uint64_t status[2];
    uint32_t mat1;
    uint32_t mat2;
    uint64_t tmat;
};

typedef struct TINYMT64_T tinymt64_t;

void tinymt64_init(tinymt64_t * random, uint64_t seed);
void tinymt64_init_by_array(tinymt64_t * random, const uint64_t init_key[],
                            int key_length);

#if defined(__GNUC__)
/**
 * This function always returns 127
 * @param random not used
 * @return always 127
 */
inline static int tinymt64_get_mexp(
    tinymt64_t * random  __attribute__((unused))) {
    return TINYMT64_MEXP;
}
#else
inline static int tinymt64_get_mexp(tinymt64_t * random) {
    return TINYMT64_MEXP;
}
#endif

/**
 * This function changes internal state of tinymt64.
 * Users should not call this function directly.
 * @param random tinymt internal status
 */
inline static void tinymt64_next_state(tinymt64_t * random) {
    uint64_t x;

    random->status[0] &= TINYMT64_MASK;
    x = random->status[0] ^ random->status[1];
    x ^= x << TINYMT64_SH0;
    x ^= x >> 32;
    x ^= x << 32;
    x ^= x << TINYMT64_SH1;
    random->status[0] = random->status[1];
    random->status[1] = x;
    if ((x & 1) != 0) {
        random->status[0] ^= random->mat1;
        random->status[1] ^= ((uint64_t)random->mat2 << 32);
    }
}

/**
 * This function outputs 64-bit unsigned integer from internal state.
 * Users should not call this function directly.
 * @param random tinymt internal status
 * @return 64-bit unsigned pseudorandom number
 */
inline static uint64_t tinymt64_temper(tinymt64_t * random) {
    uint64_t x;
#if defined(LINEARITY_CHECK)
    x = random->status[0] ^ random->status[1];
#else
    x = random->status[0] + random->status[1];
#endif
    x ^= random->status[0] >> TINYMT64_SH8;
    if ((x & 1) != 0) {
        x ^= random->tmat;
    }
    return x;
}

/**
 * This function outputs floating point number from internal state.
 * Users should not call this function directly.
 * @param random tinymt internal status
 * @return floating point number r (1.0 <= r < 2.0)
 */
inline static double tinymt64_temper_conv(tinymt64_t * random) {
    uint64_t x;
    union {
        uint64_t u;
        double d;
    } conv;
#if defined(LINEARITY_CHECK)
    x = random->status[0] ^ random->status[1];
#else
    x = random->status[0] + random->status[1];
#endif
    x ^= random->status[0] >> TINYMT64_SH8;
    if ((x & 1) != 0) {
        conv.u = ((x ^ random->tmat) >> 12) | UINT64_C(0x3ff0000000000000);
    } else {
        conv.u = (x  >> 12) | UINT64_C(0x3ff0000000000000);
    }
    return conv.d;
}

/**
 * This function outputs floating point number from internal state.
 * Users should not call this function directly.
 * @param random tinymt internal status
 * @return floating point number r (1.0 < r < 2.0)
 */
inline static double tinymt64_temper_conv_open(tinymt64_t * random) {
    uint64_t x;
    union {
        uint64_t u;
        double d;
    } conv;
#if defined(LINEARITY_CHECK)
    x = random->status[0] ^ random->status[1];
#else
    x = random->status[0] + random->status[1];
#endif
    x ^= random->status[0] >> TINYMT64_SH8;
    if ((x & 1) != 0) {
        conv.u = ((x ^ random->tmat) >> 12) | UINT64_C(0x3ff0000000000001);
    } else {
        conv.u = (x >> 12) | UINT64_C(0x3ff0000000000001);
    }
    return conv.d;
}

/**
 * This function outputs 64-bit unsigned integer from internal state.
 * @param random tinymt internal status
 * @return 64-bit unsigned integer r (0 <= r < 2^64)
 */
inline static uint64_t tinymt64_generate_uint64(tinymt64_t * random) {
    tinymt64_next_state(random);
    return tinymt64_temper(random);
}

/**
 * This function outputs floating point number from internal state.
 * This function is implemented using multiplying by (1 / 2^53).
 * @param random tinymt internal status
 * @return floating point number r (0.0 <= r < 1.0)
 */
inline static double tinymt64_generate_double(tinymt64_t * random) {
    tinymt64_next_state(random);
    return (double)(tinymt64_temper(random) >> 11) * TINYMT64_MUL;
}

/**
 * This function outputs floating point number from internal state.
 * This function is implemented using union trick.
 * @param random tinymt internal status
 * @return floating point number r (0.0 <= r < 1.0)
 */
inline static double tinymt64_generate_double01(tinymt64_t * random) {
    tinymt64_next_state(random);
    return tinymt64_temper_conv(random) - 1.0;
}

/**
 * This function outputs floating point number from internal state.
 * This function is implemented using union trick.
 * @param random tinymt internal status
 * @return floating point number r (1.0 <= r < 2.0)
 */
inline static double tinymt64_generate_double12(tinymt64_t * random) {
    tinymt64_next_state(random);
    return tinymt64_temper_conv(random);
}

/**
 * This function outputs floating point number from internal state.
 * This function is implemented using union trick.
 * @param random tinymt internal status
 * @return floating point number r (0.0 < r <= 1.0)
 */
inline static double tinymt64_generate_doubleOC(tinymt64_t * random) {
    tinymt64_next_state(random);
    return 2.0 - tinymt64_temper_conv(random);
}

/**
 * This function outputs floating point number from internal state.
 * This function is implemented using union trick.
 * @param random tinymt internal status
 * @return floating point number r (0.0 < r < 1.0)
 */
inline static double tinymt64_generate_doubleOO(tinymt64_t * random) {
    tinymt64_next_state(random);
    return tinymt64_temper_conv_open(random) - 1.0;
}

#if defined(__cplusplus)
}
#endif

#endif
