#include "vba.h"
#include "generator.h"


unsigned char global_voucher_seed[16] = {0};
uint16_t _fixed_iter[FIXED_ITERS_COUNT] = {
        0x0001,
        0x0010,
        0x0100,
        0x0500,
        0x1000,
        0x7000,
        0x8500,
        0x8000,
        0x8500,
        0x9000,
        0xF000,
        0xF500,
        0xFE00,
        0xFEE0,
        0xFFFE,
};
uint16_t _fixed_iter_step = 0x100;


void rotate_voucher_seed()
{
    for (unsigned int i = 0; i < sizeof(global_voucher_seed) / sizeof(uint64_t); ++i)
        *((uint64_t *)&global_voucher_seed[i]) = Xoshiro128p__next_bounded_any();
}


uint64_t compute_address_hash_suffix(uint8_t *voucher_seed,
                                     uint8_t *mac_address,
                                     uint16_t iterations,
                                     enum VbaAlgorithm algorithm)
{
    size_t res_buffer_size = 32;
    uint8_t res_buffer[32] = {0};

    /*
     * The 'password' is always the voucher seed. The salt is a combination
     *   of MAC + 'vba' + the 64-bit subnet prefix (or left-most 64 bits of the
     *   unicast address that will be built). This example application uses "fe80::".
     */
    size_t salt_len = 17;
    uint8_t salt[17] = { 0, 0, 0, 0, 0, 0, 'v', 'b', 'a', 0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    memcpy(&salt[0], mac_address, 6);

    switch (algorithm) {
        case PBKDF2:
            PKCS5_PBKDF2_HMAC((const char*)voucher_seed,
                              16,
                              salt,
                              salt_len,
                              iterations * ITERATIONS_FACTOR,
                              EVP_sha256(),
                              res_buffer_size,
                              res_buffer);

            break;
        case ARGON2:
            argon2d_hash_raw(iterations,
                             128,   /* 128 KiB */
                             1,
                             voucher_seed,
                             16,
                             salt,
                             salt_len,
                             res_buffer,
                             res_buffer_size);

            break;
        case SCRYPT:
            /* https://www.tarsnap.com/scrypt.html */
            /* https://words.filippo.io/the-scrypt-parameters/ */
            libscrypt_scrypt(voucher_seed,
                             16,
                             salt,
                             salt_len,
                             128,   /* N */
                             iterations,   /* r */
                             1,   /* p */
                             res_buffer,
                             res_buffer_size);

            break;
        default:
            fprintf(stderr, "Address suffix computation called for an unknown algorithm type.\n");
            return -1;
    }

    /* Always use the first 8 bytes (64 bits) of the resulting hash. */
    return *((uint64_t *)&res_buffer[0]);
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

bool verify_address_suffix(uint64_t suffix,
                           uint8_t* voucher_seed,
                           uint8_t* mac_address,
                           VbaAlgorithm algorithm) {
    uint16_t iterations = (uint16_t)((~suffix >> 48) & 0xFFFF);

    uint64_t hash_result = compute_address_hash_suffix(voucher_seed,
                                                       mac_address,
                                                       iterations,
                                                       algorithm);

    uint64_t computed_suffix = build_address_suffix(iterations, hash_result);

    return computed_suffix == suffix;
}