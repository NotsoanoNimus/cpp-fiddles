#include <vector>
#include <chrono>
#include <string>
#include <stdlib.h>

#include "benchmarking.hpp"
#include "timing.hpp"
#include "vba.h"


extern unsigned char global_voucher_seed[16];
extern uint16_t _fixed_iter[FIXED_ITERS_COUNT];
extern uint16_t _fixed_iter_step;


static inline void _benchmark_algo(VbaAlgorithm);


void
benchmark_pbkdf2()
{
    return _benchmark_algo(PBKDF2);
}

void
benchmark_argon2()
{
    return _benchmark_algo(ARGON2);
}

void benchmark_scrypt()
{
    return _benchmark_algo(SCRYPT);
}


static inline void
_benchmark_algo(VbaAlgorithm algorithm)
{
    uint8_t mac_address[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    for (int i = 0, iterations = _fixed_iter_step;
         iterations <= 0xFF00;
         iterations += _fixed_iter_step, ++i)
    {
        printf("Algorithm #%d iterations '0x%04x' (actual%s: %u) ... ",
               algorithm,
               iterations,
               algorithm == PBKDF2 ? " [x256]" : "",
               algorithm == PBKDF2 ? (iterations * ITERATIONS_FACTOR) : iterations);

        auto start = std::chrono::high_resolution_clock::now();

        uint64_t result =
                compute_address_hash_suffix(global_voucher_seed,
                                            mac_address,
                                            iterations,
                                            algorithm);

        auto end = std::chrono::high_resolution_clock::now();
        
        std::stringstream s;
        s << "Iterations " << iterations;
        Timing::RecordTiming(i, start, end, s.str());

        printf("Result: 0x%016lx\n\tMAC Address: ", result);

        for (int i = 0; i < 6; ++i)
            printf("%02x%s", mac_address[i], i != 5 ? "-" : "");

        printf("    Seed: 0x");
        for (unsigned int i = 0; i < sizeof(global_voucher_seed); ++i)
            printf("%02x", global_voucher_seed[i]);

        uint64_t addr_suffix = build_address_suffix(iterations, result);
        printf("\n\tFinal IPv6 Addr (lladdr): ");
        print_lladdr_from_suffix(addr_suffix);

        auto us =  Timing::ConvertTimeToMicroseconds(start, end);
        printf("\tDuration: %16lu us   (%16f ms)\n\n", us, us / 1000.0);

        if (iterations == 0xFF00) break;
    }
}
