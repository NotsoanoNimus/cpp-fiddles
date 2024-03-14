/*
 * Testing VBAs using Cryptographically Generated Address methodology
 *   which requires "proof of work" to easily validate a hash.
 */

#include <stdio.h>
#include <chrono>
#include <algorithm>

#include "generator.h"
#include "sha-256.h"


static unsigned char global_voucher_seed[8] = {0};
static inline void rotate_voucher_seed()
{
    *((uint64_t *)&global_voucher_seed[0]) = Xoshiro128p__next_bounded_any();
}

static inline bool _check_addr_hash(uint8_t *hash, int difficulty)
{
    bool is_good = true;
    difficulty = std::max(1, difficulty);

    for (int i = 0; i < difficulty; ++i)
        is_good &= (0x0A == (i % 2 ? (hash[i / 2] & 0x0F) : ((hash[i / 2] >> 4) & 0x0F)));

    return is_good;
}

static inline uint64_t _convert_time_to_microseconds(std::chrono::high_resolution_clock::time_point start,
                                                     std::chrono::high_resolution_clock::time_point end)
{
    std::chrono::duration<double> duration = end - start;
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

static void colliding_with_a_generated_modifier();
static void iterative_proof_of_work_tests();


int main()
{
    colliding_with_a_generated_modifier();

    iterative_proof_of_work_tests();

    return 0;
}


static void
colliding_with_a_generated_modifier()
{
}


static void
iterative_proof_of_work_tests()
{
    /*
     * 100 Tests; 5 Results each. One for the whole test, one for the initial
     *   address generation, and three for the MAC address collisions.
     */
    uint64_t timings[100][5] = {0};

    int test = 100;
    int difficulty = 0;
    uint8_t mac_address[8] = {0};
    uint8_t hash[32] = {0};

    char pow_input[8 + 6 + 8] = {0};

    Xoshiro128p__init();
    *((uint64_t *)&mac_address[0]) = Xoshiro128p__next_bounded_any();

    do {
        difficulty = (120 - test) / 20;
        printf("Running test #%d, with difficulty '%d'...\n", (101 - test), difficulty);

        auto test_start_time = std::chrono::high_resolution_clock::now();
        rotate_voucher_seed();

        printf("\tGenerating address from seed '");
        for (int i = 0; i < 8; ++i) {
            pow_input[i] = global_voucher_seed[i];
            printf("%02x", global_voucher_seed[i]);
        }
        printf("' and starting MAC '");
        for (int i = 2; i < 8; ++i) {
            pow_input[8 + i - 2] = mac_address[i];
            printf("%02x", mac_address[i]);
        }
        printf("'...\n");

        /* GENERATE A 'MODIFIER' VALUE WHICH WILL PRODUCE THE RIGHT PROOF OF WORK. */
        auto addrgen_start_time = std::chrono::high_resolution_clock::now();
        do {
            *((uint64_t *)&pow_input[8 + 6]) = Xoshiro128p__next_bounded_any();
            calc_sha_256(hash, &pow_input[0], 8 + 6 + 8);
        } while (false == _check_addr_hash(&hash[0], difficulty));

        printf("\t\tDONE!\n\t\tModifier: ");
        for (int i = 0; i < 8; ++i)
            printf("%02x", (uint8_t)pow_input[8 + 6 + i]);
        printf("\n\t\tHash value: ");
        for (int i = 0; i < 32; ++i)
            printf("%02x", hash[i]);
        printf("\n");

        auto addrgen_end_time = std::chrono::high_resolution_clock::now();
        timings[100 - test][1] = _convert_time_to_microseconds(addrgen_start_time, addrgen_end_time);

        /* CHECK FOR DIFFERENT MAC ADDRESSES WHICH WILL ALSO FORM A VALID ADDRESS. */
        int collisions = 0;
        auto collision_start_time = std::chrono::high_resolution_clock::now();
        auto collision_end_time = std::chrono::high_resolution_clock::now();
        do {
            /* Change the MAC. */
            *((uint64_t *)&mac_address[0]) = Xoshiro128p__next_bounded_any();
            for (int i = 2; i < 8; ++i)
                pow_input[8 + i - 2] = mac_address[i];

            calc_sha_256(hash, &pow_input[0], 8 + 6 + 8);

            if (true == _check_addr_hash(&hash[0], difficulty)) {
                printf("\t\t\tCollision for MAC: ");
                for (int i = 2; i < 8; ++i)
                    printf("%02x", mac_address[i]);
                printf("\n");

                ++collisions;
                collision_end_time = std::chrono::high_resolution_clock::now();

                timings[100 - test][2 + collisions] = _convert_time_to_microseconds(
                        collision_start_time, collision_end_time);

                collision_start_time = std::chrono::high_resolution_clock::now();
            }
        } while (collisions != 3);

        auto test_end_time = std::chrono::high_resolution_clock::now();
        timings[100 - test][0] = _convert_time_to_microseconds(test_start_time, test_end_time);

        printf("\n");
    } while (--test);

    printf("\n\n\n=== TIMING SUMMARY ===\n\t\tAll values are in microseconds.\n\n");
    for (int i = 0; i < 100; ++i) {
        printf("Test #%d:\n", i + 1);
        printf("\tCompleted in: %llu", timings[i][0]);
        printf("\tGenerated initial address in: %llu", timings[i][1]);
        printf("\tGenerated collision #1 in: %llu", timings[i][2]);
        printf("\tGenerated collision #2 in: %llu", timings[i][3]);
        printf("\tGenerated collision #3 in: %llu", timings[i][4]);
        printf("\n");
    }
    printf("===========================\n");
}