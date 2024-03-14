#include <stdio.h>
#include <cstdint>
#include <tuple>
#include <string.h>

#include "fastpbkdf2.h"
#include "argon2.h"


#define ITERATIONS_FACTOR  256

enum VbaAlgorithm
{
    PBKDF2,
    ARGON2,
};


uint64_t compute_address_hash_suffix(uint8_t *voucher_seed,
                                     size_t voucher_seed_size,
                                     uint8_t *mac_address,
                                     size_t mac_address_size,
                                     uint16_t iterations,
                                     VbaAlgorithm algorithm)
{
    size_t res_buffer_size;
    uint8_t res_buffer[32] = {0};

    /* Argon2-specific parameters. */
    size_t salt_len = 9;
    uint8_t salt[salt_len] = { 0, 0, 0, 0, 0, 0, 'v', 'b', 'a' };
    memcpy(&salt[0], mac_address, mac_address_size % salt_len);

    switch (algorithm) {
        case PBKDF2:
            res_buffer_size = 32;
            fastpbkdf2_hmac_sha256(voucher_seed,
                                   voucher_seed_size,
                                   mac_address,
                                   mac_address_size,
                                   (uint32_t)iterations,
                                   res_buffer,
                                   res_buffer_size);

            break;
        case ARGON2:
            res_buffer_size = 16;
            argon2d_hash_raw((iterations % ITERATIONS_FACTOR) + 2,
                             64,
                             1,
                             voucher_seed,
                             voucher_seed_size
                             &salt[0],
                             salt_len,
                             res_buffer,
                             res_buffer_size);

            break;
        default:
            fprintf(stderr, "Address suffix computation called for unknown algorithm type.\n");
            return -1;

        return (uint64_t)res_buffer[res_buffer_size - 8];
    }
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

std::tuple<uint16_t, uint64_t> get_parts_from_address_suffix(uint64_t suffix)
{
    uint16_t iterations = (uint16_t)(suffix >> 48) & 0xFFFF;
    uint64_t hash_result = 0x0000FFFFFFFFFFFF & suffix;

    return std::make_tuple(iterations, hash_result);
}


class VoucherBasedAddress
{
public:
    enum ALGORITHM
    {
        PBKDF2,
        ARGON2,
    };

    VoucherBasedAddress(uint8_t *voucher_seed_base,
                        uint8_t *mac_address_base,
                        uint16_t *iterations)
        : m_seed(voucher_seed_base)
        , m_mac(mac_address_base)
        , m_iter(iterations)
    {};

    void Generate()
    {
    }

    void PrintAddress(const std::string const subnet_prefix = "fe80::")
    {
        if (!m_generated) {
            printf("ADDRESS NOT GENERATED.\n");
            return;
        }

        printf("%s", subnet_prefix.c_str());
        printf()
    }

private:
    uint8_t *m_seed;
    uint8_t *m_mac;
    uint16_t *m_iter;

    bool m_generated = false;

    uint64_t m_suffix;
};
