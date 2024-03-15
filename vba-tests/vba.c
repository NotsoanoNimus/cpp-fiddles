#include "vba.h"


uint64_t compute_address_hash_suffix(uint8_t *voucher_seed,
                                     size_t voucher_seed_size,
                                     uint8_t *mac_address,
                                     size_t mac_address_size,
                                     uint16_t iterations,
                                     enum VbaAlgorithm algorithm)
{
    size_t res_buffer_size;
    uint8_t res_buffer[32] = {0};

    /* Argon2-specific parameters. */
    size_t salt_len = 9;
    uint8_t salt[9] = { 0, 0, 0, 0, 0, 0, 'v', 'b', 'a' };
    memcpy(&salt[0], mac_address, mac_address_size % salt_len);

    switch (algorithm) {
        case PBKDF2:
            res_buffer_size = 32;
            PKCS5_PBKDF2_HMAC(voucher_seed,
                              voucher_seed_size,
                              salt,
                              salt_len,
                              iterations * ITERATIONS_FACTOR,
                              EVP_sha256(),
                              res_buffer_size,
                              res_buffer);

            break;
        case ARGON2:
            res_buffer_size = 16;
            argon2d_hash_raw(iterations,
                             64,
                             1,
                             voucher_seed,
                             voucher_seed_size,
                             salt,
                             salt_len,
                             res_buffer,
                             res_buffer_size);

            break;
        default:
            fprintf(stderr, "Address suffix computation called for unknown algorithm type.\n");
            return -1;
    }

    return *((uint64_t *)&res_buffer[res_buffer_size - 8]);
}

uint64_t build_address_suffix(uint16_t iterations, uint64_t hash_result)
{
    return ((uint64_t)(~iterations) << 48) | (0x0000FFFFFFFFFFFF & hash_result);
}

void print_lladdr_from_suffix(uint64_t suffix)
{
    printf("fe80::");
    for (int i = 1; i < 9; ++i)
        printf("%02x%s",
               (uint8_t)(0xFF & (suffix >> (64 - (i * 8)))),
               (i - 1) % 2 && i < 8 ? ":" : "");
}