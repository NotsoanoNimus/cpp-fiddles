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
#include <sstream>
#include <tuple>
#include <vector>
#include <string>

#include "vba.h"
#include "timing.hpp"
#include "generator.h"

#include "benchmarking.hpp"
#include "collisions.hpp"
#include "generate_verify.hpp"


#define RECORD_TIMES(test_name, method) \
    rotate_voucher_seed(); \
    (method)(); \
    Timing::DumpToCsv(#test_name, csv);


static inline void rotate_voucher_seed()
{
    for (int i = 0; i < sizeof(global_voucher_seed) / 8; ++i)
        *((uint64_t *)&global_voucher_seed[i]) = Xoshiro128p__next_bounded_any();
}


int main()
{
    /* Initialize the optimized PRNG and some results recording devices. */
    Xoshiro128p__init();
    
    std::stringstream csv;
    csv << "Test,Slot,Microseconds,Milliseconds,Seconds,Comment" << std::endl;

    /* The first benchmarking is done on random voucher-seed values of a fixed length. */
    /*   Let it walk up the iteration count for each and see how it performs. */
    RECORD_TIMES("BENCH_PBKDF2", benchmark_pbkdf2);
    RECORD_TIMES("BENCH_ARGON2", benchmark_argon2);
    RECORD_TIMES("BENCH_SCRYPT", benchmark_scrypt);

    /* The next tests are more targeted towards the purpose of VBAs. */
    /*   A few different fixed iteration counts are compared for security. */
    RECORD_TIMES("COLLISIONS_PBKDF2", find_collisions_pbkdf2);
    RECORD_TIMES("COLLISIONS_ARGON2", find_collisions_argon2);
    RECORD_TIMES("COLLISIONS_SCRYPT", find_collisions_scrypt);

    /* The next tests analyze the performance of recipient machines. */
    /*   By nature of the algorithm, receivers spend the same time as generators. */
    /*   Again, this is done at a few different fixed iteration counts. */
    RECORD_TIMES("GNV_PBKDF2", generate_and_verify_pbkdf2);
    RECORD_TIMES("GNV_ARGON2", generate_and_verify_argon2);
    RECORD_TIMES("GNV_SCRYPT", generate_and_verify_scrypt);

    /* Final tests. These don't really do anything extra. */
    /*   They demonstrate a way to encapsulate all properties of a voucher into an object. */
    /*   It can then generate/verify an address given a data-link address and iterations input. */
    // NdpVoucher vBigSeedPbkdf2(...);
    // NdpVoucher vBigSeedArgon2(...);
    // NdpVoucher vTinySeedPbKdf2(...);
    // NdpVoucher vTinySeedArgon2(...);


    /* Output the CSV to the console. This can be changed later to write to a file. */
    std::cout << "Final Data:\n" << csv.str();
}
