#ifndef _VBA_H_
#define _VBA_H_


#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <argon2.h>
#include <libscrypt.h>
#include <scrypt-kdf.h>


#define ITERATIONS_FACTOR  256
#define FIXED_ITERS_COUNT  15

enum VbaAlgorithm
{
    PBKDF2 = 1,
    ARGON2 = 2,
    SCRYPT = 3,
};


void rotate_voucher_seed();

uint64_t compute_address_hash_suffix(uint8_t *voucher_seed,
                                     uint8_t *mac_address,
                                     uint16_t iterations,
                                     enum VbaAlgorithm algorithm);

uint64_t build_address_suffix(uint16_t iterations, uint64_t hash_result);

void print_lladdr_from_suffix(uint64_t suffix);

bool verify_address_suffix(uint64_t suffix,
                           uint8_t* voucher_seed,
                           uint8_t* mac_address,
                           VbaAlgorithm algorithm);


#endif /* _VBA_H_ */