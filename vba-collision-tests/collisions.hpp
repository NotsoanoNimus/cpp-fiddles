#ifndef _COLLISIONS_H_
#define _COLLISIONS_H_


#include "vba.h"

#define THREAD_COUNT  32
#define MACS_PER_THREAD  ((1ULL << 48) / THREAD_COUNT)


typedef struct _worker_ctx {
    pthread_t thread_handle;
    unsigned int id;
    uint64_t starting_mac;
    uint64_t ending_mac;
    uint8_t* voucher_seed;
    uint64_t legitimate_suffix;
    uint16_t iterations;
    VbaAlgorithm algorithm;
    bool* match_found_sync_bool;
} worker_ctx_t;


void find_collisions_pbkdf2();
void find_collisions_argon2();
void find_collisions_scrypt();


#endif /* _COLLISIONS_H_ */