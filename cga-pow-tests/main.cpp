/*
 * Testing VBAs using Cryptographically Generated Address methodology
 *   which requires "proof of work" to easily validate a hash.
 */

#include <stdio.h>
#include <chrono>

#include "generator.h"
#include "sha-256.h"

static unsigned char global_voucher_seed[8] = {0};
static inline void rotate_voucher_seed()
{
    *((uint64_t *)global_voucher_seed[0]) = Xoshiro128p__next_bounded();
}


int main()
{
    int test = 100;
    uint8_t mac_address[8] = {0};
    uint8_t hash[32] = {0};

    char pow_input[8 + 6 + 8] = {0};

    Xoshiro128p__init();
    *((uint64_t *)&mac_address[0]) = Xoshiro128p__next_bounded();

    do {
        printf("Running test #%d...\n", (101 - test));

        auto test_start_time = std::chrono::high_resolution_clock::now();        
        rotate_voucher_seed();
        
        printf("\tGenerating address from seed '");
        for (int i = 0; i < 8; ++i) {
            printf("%02x", global_voucher_seed[i]);
            pow_input[i] = global_voucher_seed[i];
        }
        printf("' and starting MAC '");
        for (int i = 2; i < 8; ++i) {
            printf("%02x", mac_address[i]);
            pow_input[8 + i - 2] = mac_address[i];
        }
        printf("'...\n");

        auto addrgen_start_time = std::chrono::high_resolution_clock::now();

        do {
            *((uint64_t *)&pow_input[8 + 6]) = Xoshiro128p__next_bounded();
            calc_sha_256(hash, &pow_input[0], 8 + 6 + 8);
        } while (hash[0] != 0xAA);
        
        auto addrgen_end_time = std::chrono::high_resolution_clock::now();

        auto test_end_time = std::chrono::high_resolution_clock::now();

        printf("\n");
    } while (--test);

    return 0;
}