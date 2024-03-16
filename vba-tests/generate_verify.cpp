#include <chrono>
#include <sstream>

#include "generate_verify.hpp"

#include "vba.h"
#include "generator.h"
#include "timing.hpp"


extern unsigned char global_voucher_seed[8];
extern uint16_t _fixed_iter[FIXED_ITERS_COUNT];
extern uint16_t _fixed_iter_step;

static inline void _generate_and_verify(VbaAlgorithm);

static inline void _roll_mac_address(uint8_t* mac_address, uint64_t rand)
{
    /* Generate and use any random MAC address. Unrolled for speed. */
    rand = UINT64_MAX == rand ? Xoshiro128p__next_bounded_any() : rand;

    mac_address[0] = *(((uint8_t *)&rand) + 0);
    mac_address[1] = *(((uint8_t *)&rand) + 1);
    mac_address[2] = *(((uint8_t *)&rand) + 2);
    mac_address[3] = *(((uint8_t *)&rand) + 3);
    mac_address[4] = *(((uint8_t *)&rand) + 4);
    mac_address[5] = *(((uint8_t *)&rand) + 5);
}


void generate_and_verify_pbkdf2()
{
    return _generate_and_verify(PBKDF2);
}

void generate_and_verify_argon2()
{
    return _generate_and_verify(ARGON2);
}

void generate_and_verify_scrypt()
{
    return _generate_and_verify(SCRYPT);
}


static inline void
_generate_and_verify(VbaAlgorithm algorithm)
{
    uint8_t mac_address[6] = {0};

    for (int i = 0, j = 0; i < FIXED_ITERS_COUNT; ++i, j += 2) {
        uint16_t iterations = _fixed_iter[i];
        _roll_mac_address(mac_address, UINT64_MAX);

        uint64_t true_iter = algorithm == PBKDF2 ? iterations * ITERATIONS_FACTOR : iterations;
        printf("GENERATE AND VERIFY #%d (Algorithm %d):\n  Generate.\n\t%lu iterations\n\tVoucher: ",
               i,
               algorithm,
               true_iter);
        for (int x = 0; x < 8; ++x)
            printf("%02x", global_voucher_seed[x]);
        printf("\n\tMAC: ");
        for (int x = 0; x < 6; ++x)
            printf("%02x%s", mac_address[x], x != 5 ? "-" : "");
        fflush(stdout);

        auto start_generate = std::chrono::high_resolution_clock::now();

        uint64_t legitimate_hash =
            compute_address_hash_suffix(global_voucher_seed,
                                        mac_address,
                                        iterations,
                                        algorithm);

        uint64_t legitimate_suffix = build_address_suffix(iterations,
                                                          legitimate_hash);

        auto end_generate = std::chrono::high_resolution_clock::now();

        printf("\n\tAddress: ");
        print_lladdr_from_suffix(legitimate_suffix);

        printf("\n  Verify.\n\t");
        auto start_verify = std::chrono::high_resolution_clock::now();

        bool verified = verify_address_suffix(legitimate_suffix,
                                              global_voucher_seed,
                                              mac_address,
                                              algorithm);

        auto end_verify = std::chrono::high_resolution_clock::now();

        if (verified)
            printf("OK");
        else
            printf("FAILED VERIFICATION");

        std::stringstream s_generate, s_verify;
        s_generate << "Generate. " << algorithm << " / Iterations " << iterations;
        s_verify << "Verify. " << algorithm << " / Iterations " << iterations;
        Timing::RecordTiming(j, start_generate, end_generate, s_generate.str());
        Timing::RecordTiming(j + 1, start_verify, end_verify, s_verify.str());

        printf("\n  Completed.\n\n");
    }
}