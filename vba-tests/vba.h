#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/rand.h>

//#include "fastpbkdf2.h"
#include "argon2.h"


#define ITERATIONS_FACTOR  256

enum VbaAlgorithm
{
    PBKDF2 = 1,
    ARGON2 = 2,
};


uint64_t compute_address_hash_suffix(uint8_t *voucher_seed,
                                     size_t voucher_seed_size,
                                     uint8_t *mac_address,
                                     size_t mac_address_size,
                                     uint16_t iterations,
                                     enum VbaAlgorithm algorithm);

uint64_t build_address_suffix(uint16_t iterations, uint64_t hash_result);

void print_lladdr_from_suffix(uint64_t suffix);