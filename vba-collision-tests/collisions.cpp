#include <chrono>
#include <sstream>
#include <pthread.h>

#include "collisions.hpp"

#include "vba.h"
#include "generator.h"


extern uint16_t _fixed_iter[FIXED_ITERS_COUNT];
extern uint16_t _fixed_iter_step;

static worker_ctx_t _thread_contexts[THREAD_COUNT] = {0};

static inline void _find_collisions(VbaAlgorithm);

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

static inline double _convert_time_to_seconds(std::chrono::high_resolution_clock::time_point start,
                                                     std::chrono::high_resolution_clock::time_point end)
{
    std::chrono::duration<double> duration = end - start;
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

static uint8_t _stable_mac_address[6] = {
    0xC0, 0x01, 0xCA, 0x70, 0xFF, 0xFF
};
static uint8_t _stable_voucher_seed[8] = {
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


static void* _thread_routine_collision_random(void* thread_ctx)
{
    uint8_t fake_mac[6] = {0};
    uint64_t fake_suffix = 0x0;
    uint64_t loop_breaker = 1ULL << 24;

    worker_ctx_t* ctx = (worker_ctx_t *)thread_ctx;

    auto start_random = std::chrono::high_resolution_clock::now();

    do {
        _roll_mac_address(fake_mac, UINT64_MAX);

        uint64_t fake_hash =
                compute_address_hash_suffix(ctx->voucher_seed,
                                            fake_mac,
                                            ctx->iterations,
                                            ctx->algorithm);

        fake_suffix = build_address_suffix(ctx->iterations, fake_hash);
    } while (--loop_breaker && fake_suffix != ctx->legitimate_suffix);

    auto end_random = std::chrono::high_resolution_clock::now();
    double duration_random = _convert_time_to_seconds(start_random, end_random);

    if (!loop_breaker) {
        printf("\n\tThread %d: FAILURE: Loop broken; no matches", ctx->id);
    } else {
        *(ctx->match_found_sync_bool) = true;
        printf("\n\tThread %d: SUCCESS: Impostor MAC is ", ctx->id);
        for (int x = 0; x < 6; ++x)
            printf("%02x%s", fake_mac[x], x != 5 ? "-" : "");
    }

    printf("\n\t\t\tThread %d: Operation took '%f' seconds.", ctx->id, duration_random);
    return NULL;
}

static void* _thread_routine_collision_ordered(void* thread_ctx)
{
    uint8_t fake_mac[6] = {0};
    uint64_t fake_suffix = 0x0;
    uint64_t mac = 0x0;

    worker_ctx_t* ctx = (worker_ctx_t *)thread_ctx;
    mac = ctx->starting_mac;

    auto start_ordered = std::chrono::high_resolution_clock::now();

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
                compute_address_hash_suffix(ctx->voucher_seed,
                                            fake_mac,
                                            ctx->iterations,
                                            ctx->algorithm);

        fake_suffix = build_address_suffix(ctx->iterations, fake_hash);
    } while (++mac < ctx->ending_mac && fake_suffix != ctx->legitimate_suffix);

    auto end_ordered = std::chrono::high_resolution_clock::now();
    double duration_ordered = _convert_time_to_seconds(start_ordered, end_ordered);

    if (mac >= 0x0000FFFFFFFFFFFF) {
        printf("\n\tThread %d: FAILURE: MACs exhausted; no matches", ctx->id);
    } else {
        *(ctx->match_found_sync_bool) = true;
        printf("\n\tThread %d: SUCCESS: Impostor MAC is ", ctx->id);
        for (int x = 0; x < 6; ++x)
            printf("%02x%s", fake_mac[x], x != 5 ? "-" : "");
    }

    printf("\t\t\tThread %d: Operation took '%f' seconds.", ctx->id, duration_ordered);
    return NULL;
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
    bool is_random_match = false;
    bool is_ordered_match = false;

    for (int i = 0, j = 0; i < FIXED_ITERS_COUNT; ++i, j += 2) {
        uint16_t iterations = _fixed_iter[i];

        printf("Computing address for '0x%04x' (%d) iterations.\n", iterations, iterations);

        uint64_t legitimate_hash =
            compute_address_hash_suffix(_stable_voucher_seed,
                                        _stable_mac_address,
                                        iterations,
                                        algorithm);

        uint64_t legitimate_suffix = build_address_suffix(iterations,
                                                          legitimate_hash);
        
        printf("\tGot address: ");
        print_lladdr_from_suffix(legitimate_suffix);

        printf("\n\tNow searching for a collision...\n\t\tRandom... \n");

        for (unsigned int x = 0; x < THREAD_COUNT; ++x) {
            worker_ctx_t thread_ctx = {
                    {0},
                    x + 1,
                    0,
                    0,
                    &_stable_voucher_seed[0],
                    legitimate_suffix,
                    iterations,
                    algorithm,
                    &is_random_match
            };
            memcpy(&_thread_contexts[x], &thread_ctx, sizeof(worker_ctx_t));

            pthread_create(&(_thread_contexts[x].thread_handle),
                           NULL,
                           _thread_routine_collision_random,
                           &_thread_contexts[x]);
        }
        for (int x = 0; x < THREAD_COUNT; ++x)
            pthread_join(_thread_contexts[x].thread_handle, NULL);

        if (is_random_match)
            printf("\n\n=== A RANDOM MAC ADDRESS WAS USED TO CREATE A COLLISION! ===\n\n");

        /* ================================= */
        printf("\n\t\tOrdered... \n");

        for (unsigned int x = 0; x < THREAD_COUNT; ++x) {
            worker_ctx_t thread_ctx = {
                    {0},
                    x + 1,
                    x * MACS_PER_THREAD,
                    ((x + 1) * MACS_PER_THREAD) - 1,
                    &_stable_voucher_seed[0],
                    legitimate_suffix,
                    iterations,
                    algorithm,
                    &is_ordered_match
            };
            memcpy(&_thread_contexts[x], &thread_ctx, sizeof(worker_ctx_t));

            pthread_create(&(_thread_contexts[x].thread_handle),
                           NULL,
                           _thread_routine_collision_ordered,
                           &_thread_contexts[x]);
        }
        for (int x = 0; x < THREAD_COUNT; ++x)
            pthread_join(_thread_contexts[x].thread_handle, NULL);

        if (is_ordered_match)
            printf("\n\n=== AN ORDERED MAC ADDRESS WAS USED TO CREATE A COLLISION! ===\n\n");

        printf("\n\n");
    }
}