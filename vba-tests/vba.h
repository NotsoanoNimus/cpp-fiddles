#ifndef _VBA_H_
#define _VBA_H_


#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/rand.h>

#include "argon2.h"
#include "scrypt-kdf.h"


#define ITERATIONS_FACTOR  256
#define FIXED_ITERS_COUNT  9

enum VbaAlgorithm
{
    PBKDF2 = 1,
    ARGON2 = 2,
    SCRYPT = 2,
};


extern unsigned char global_voucher_seed[8] = {0};
extern uint16_t _fixed_iter[FIXED_ITERS_COUNT] = {
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
extern uint16_t _fixed_iter_step = 0x100;


uint64_t compute_address_hash_suffix(uint8_t *voucher_seed,
                                     size_t voucher_seed_size,
                                     uint8_t *mac_address,
                                     size_t mac_address_size,
                                     uint16_t iterations,
                                     enum VbaAlgorithm algorithm);

uint64_t build_address_suffix(uint16_t iterations, uint64_t hash_result);

void print_lladdr_from_suffix(uint64_t suffix);


#endif /* _VBA_H_ */