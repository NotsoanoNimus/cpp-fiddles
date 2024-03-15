#include <chrono>
#include <sstream>

#include "collisions.hpp"

#include "vba.h"
#include "generator.h"
#include "timing.hpp"


static inline void _find_collisions(VbaAlgorithm);

static inline void _roll_mac_address(uint8_t* mac_address, uint64_t rand)
{
    /* Generate and use any random MAC address. Unrolled for speed. */
    rand = -1 == rand ? Xoshiro128p__next_bounded_any() : rand;

    mac_address[0] = *(((uint8_t *)&rand) + 0);
    mac_address[1] = *(((uint8_t *)&rand) + 1);
    mac_address[2] = *(((uint8_t *)&rand) + 2);
    mac_address[3] = *(((uint8_t *)&rand) + 3);
    mac_address[4] = *(((uint8_t *)&rand) + 4);
    mac_address[5] = *(((uint8_t *)&rand) + 5);
}

static _stable_mac_address[6] = {
    0xC0, 0x01, 0xCA, 0x70, 0xFF, 0xFF
};
static _stable_voucher_seed[8] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xF0, 0x0D
};


void find_collisions_pbkdf2()
{
    return _find_collisions(PBKDF2);
}

void find_collisions_argon2()
{
    return _find_collisions(ARGON2);
}

void find_collisions_scrypt()
{
    return _find_collisions(SCRYPT);
}


/*
 * This is it. Attack the protocol and see how long it takes to find a collision.
 *   Since iteration counts are fixed into a node's address, that value must
 *   remain static throughout the duration of the test.
 * 
 * Therefore, if a node chooses a high iteration count, it is prohibitively costly
 *   for attackers to determine a 48-bit collision in any reasonable time.
 * 
 * Optimizations to this cannot come from time-memory tradeoffs, but can possibly
 *   come from restricting the input set of MAC addresses to those allowed for
 *   normal network nodes (e.g., skipping link-layer Multicast and Broadcast MACs).
 * 
 * For the sake of attack completeness, invalid MAC ranges will be SKIPPED when
 *   attempting to find a collision through the "Ordered" methodology.
 *   See: https://www.rfc-editor.org/rfc/rfc7042#section-2  
 */
static inline void
_find_collisions(VbaAlgorithm algorithm)
{
    uint8_t fake_mac[6] = {0};

    for (int i = 0, j = 0; i < FIXED_ITERS_COUNT; ++i, j += 2) {
        uint16_t iterations = _fixed_iter[i];

        printf("Computing address for '0x%04x' (%d) iterations.\n", iterations, iterations);

        uint64_t legitimate_hash =
            compute_address_hash_suffix(_stable_voucher_seed,
                                        8,
                                        legitimate_mac,
                                        6,
                                        iterations,
                                        algorithm);

        uint64_t legitimate_suffix = build_address_suffix(iterations,
                                                          legitimate_hash);
        
        printf("\tGot address: ");
        print_lladdr_from_suffix(legitimate_suffix);
        printf("\n\tNow searching for a collision...\n\t\tRandom... ");

        auto start_random = std::chrono::high_resolution_clock::now();

        uint64_t fake_suffix = 0x0;
        uint64_t loop_breaker = 1 << 40;   /* 1,099,511,627,776 attempts. */
        do {
            _roll_mac_address(fake_mac, -1);

            uint64_t fake_hash =
                compute_address_hash_suffix(_stable_voucher_seed,
                                            8,
                                            fake_mac,
                                            6,
                                            iterations,
                                            algorithm);

            fake_suffix = build_address_suffix(iterations, fake_hash);
        } while (--loop_breaker && fake_suffix != legitimate_suffix);

        auto end_random = std::chrono::high_resolution_clock::now();
        
        if (!loop_breaker) {
            printf("FAILURE: Loop broken; no matches");
        } else {
            printf("SUCCESS: Impostor MAC is ");
            for (int x = 0; x < 6; ++x)
                printf("%02x%s", fake_mac[x], x != 5 ? "-" : "");
        }

        printf("\n\t\tOrdered... ");
        auto start_ordered = std::chrono::high_resolution_clock::now();

        fake_suffix = 0x0;
        uint64_t mac = 0x1;
        do {
            /* Reserved for IPv4 multicast: OUI 01-00-5E. */
            if (0x000001005e000000 == mac) mac += 0x0000000001000000;

            /* Reserved for use by IANA: OUI 00-5E-xx. */
            if (0x0000005e00000000 == mac) mac += 0x0000000100000000;

            /* Reserved by IANA for PPP: MACs starting with CF. */
            if (0x0000CF0000000000 == mac) mac += 0x0000010000000000;

            /* Reserved IPv6 multicast range: 33-33-00 through 33-33-FF. */
            if (0x0000333300000000 == mac) mac += 0x0000000100000000;

            _roll_mac_address(fake_mac, mac);

            uint64_t fake_hash =
                compute_address_hash_suffix(_stable_voucher_seed,
                                            8,
                                            fake_mac,
                                            6,
                                            iterations,
                                            algorithm);

            fake_suffix = build_address_suffix(iterations, fake_hash);
        } while (++mac < 0x0000FFFFFFFFFFFF);

        auto end_ordered = std::chrono::high_resolution_clock::now();

        if (mac >= 0x0000FFFFFFFFFFFF) {
            printf("FAILURE: Maximum MACs exhausted; no matches");
        } else {
            printf("SUCCESS: Impostor MAC is ");
            for (int x = 0; x < 6; ++x)
                printf("%02x%s", fake_mac[x], x != 5 ? "-" : "");
        }

        printf("\n\n");

        std::stringstream s_random, s_ordered;
        s_random << "Random. Iterations " << iterations;
        s_ordered << "Ordered. Iterations " << iterations;
        Timing::RecordTiming(j, start_random, end_random, s_random.str());
        Timing::RecordTiming(j + 1, start_ordered, end_ordered, s_ordered.str());
    }
}