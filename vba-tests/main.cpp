/*
 * vba-tests
 *
 * AUTHOR : Zack Puhl <github@xmit.xyz>
 * DATE   : 2024-03-14
 * 
 * A benchmarking application for IPv6 Voucher-Based Address generation.
 *   Tests different key derivation functions for both generation and
 *   verification against a set of known and expected results.
 * 
 */


#include <chrono>

#include "generator.h"

#include "vba_pbkdf2.h"
#include "vba_argon2.h"


static unsigned char global_voucher_seed[64] = {};
static inline void rotate_voucher_seed()
{
    for (int i = 0; i < sizeof(global_voucher_seed) / 4; ++i)
        *((uint64_t *)global_voucher_seed[i]) = Xoshiro128p__next_bounded();
}

static inline void benchmark_pbkdf2();
static inline void benchmark_argon2();


int main()
{
    Xoshiro128p__init();

    /* The first benchmarking is done on random voucher-seed values of a fixed length. */
    /*   Let it walk up the iteration count for each and see how it performs. */
    rotate_voucher_seed();
    benchmark_pbkdf2();

    rotate_voucher_seed();
    benchmark_argon2();
}


static inline void benchmark_pbkdf2()
{
    return;
}


static inline void benchmark_argon2()
{
    return;
}