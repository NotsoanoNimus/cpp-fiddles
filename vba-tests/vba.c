#include "vba.h"


uint64_t compute_address_hash_suffix(uint8_t *voucher_seed,
                                     size_t voucher_seed_size,
                                     uint8_t *mac_address,
                                     size_t mac_address_size,
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
            argon2d_hash_raw(iterations,
                             102400,   /* 100 x 1,024 in KiB == 100 MiB */
                             1,
                             voucher_seed,
                             voucher_seed_size,
                             salt,
                             salt_len,
                             res_buffer,
                             res_buffer_size);

            break;
        case SCRYPT:
            /* https://www.tarsnap.com/scrypt.html */
            /* https://words.filippo.io/the-scrypt-parameters/ */
            crypto_scrypt(voucher_seed,
                          voucher_seed_size,
                          salt,
                          salt_len,
                          65536, /* N */
                          16,    /* r */
                          1,     /* p */
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