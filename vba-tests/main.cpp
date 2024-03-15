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
#include "vba.h"


static unsigned char global_voucher_seed[8] = {0};
static inline void rotate_voucher_seed()
{
    for (int i = 0; i < sizeof(global_voucher_seed) / 8; ++i)
        *((uint64_t *)&global_voucher_seed[i]) = Xoshiro128p__next_bounded_any();
}

static inline uint64_t _convert_time_to_microseconds(std::chrono::high_resolution_clock::time_point start,
                                                     std::chrono::high_resolution_clock::time_point end)
{
    std::chrono::duration<double> duration = end - start;
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

static uint16_t _fixed_iter[9] = {
        0x0001,
        0x0010,
        0x0100,
        0x7000,
        0x8000,
        0x9000,
        0xFE00,
        0xFEE0,
        0xFFFE,
};
static uint16_t _fixed_iter_step = 0x100;

static inline void _benchmark_algo(VbaAlgorithm);
static inline void benchmark_pbkdf2();
static inline void benchmark_argon2();
static inline void find_collisions_pbkdf2();
static inline void find_collisions_argon2();
static inline void generate_and_verify_pbkdf2();
static inline void generate_and_verify_argon2();


int main()
{
    /* Initialize the optimized PRNG. */
    Xoshiro128p__init();

    /* The first benchmarking is done on random voucher-seed values of a fixed length. */
    /*   Let it walk up the iteration count for each and see how it performs. */
    rotate_voucher_seed();
    benchmark_pbkdf2();
    rotate_voucher_seed();
    benchmark_argon2();

    /* The next tests are more targeted towards the purpose of VBAs. */
    /*   A few different fixed iteration counts are compared for security. */
    rotate_voucher_seed();
    find_collisions_pbkdf2();
    rotate_voucher_seed();
    find_collisions_argon2();

    /* The next tests analyze the performance of recipient machines. */
    /*   By nature of the algorithm, receivers spend the same time as generators. */
    /*   Again, this is done at a few different fixed iteration counts. */
    rotate_voucher_seed();
    generate_and_verify_pbkdf2();
    rotate_voucher_seed();
    generate_and_verify_argon2();
}


static inline void
_benchmark_algo(VbaAlgorithm algorithm)
{
    uint8_t mac_address[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    for (int iterations = 0; iterations <= 0xFF00; iterations += _fixed_iter_step) {
        printf("Algorithm #%d iterations '0x%04x' (actual%s: %u) ... ",
               algorithm,
               iterations,
               algorithm == PBKDF2 ? " [x256]" : "",
               algorithm == PBKDF2 ? (iterations * ITERATIONS_FACTOR) : iterations);

        auto start = std::chrono::high_resolution_clock::now();

        uint64_t result =
                compute_address_hash_suffix(global_voucher_seed,
                                            8,
                                            mac_address,
                                            6,
                                            iterations,
                                            algorithm);

        auto end = std::chrono::high_resolution_clock::now();
        printf("Result: 0x%016llx\n\tMAC Address: ", result);

        for (int i = 0; i < 6; ++i)
            printf("%02x%s", mac_address[i], i != 5 ? "-" : "");

        printf("    Seed: 0x");
        for (int i = 0; i < 8; ++i)
            printf("%02x", global_voucher_seed[i]);

        uint64_t addr_suffix = build_address_suffix(iterations, result);
        printf("\n\tFinal IPv6 Addr (lladdr): ");
        print_lladdr_from_suffix(addr_suffix);

        auto us =  _convert_time_to_microseconds(start, end);
        printf("\tDuration: %10u us   (%10f ms)\n\n", us, us / 1000.0);

        if (iterations == 0xFF00) break;
    }
}
static inline void benchmark_pbkdf2()
{
    return _benchmark_algo(PBKDF2);
}
static inline void benchmark_argon2()
{
    return _benchmark_algo(ARGON2);
}


static inline void find_collisions_pbkdf2()
{
}


static inline void find_collisions_argon2()
{
}


static inline void generate_and_verify_pbkdf2()
{
}


static inline void generate_and_verify_argon2()
{
}
