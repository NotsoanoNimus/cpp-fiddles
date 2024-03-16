/*
 * vba-tests
 *
 * AUTHOR : Zack Puhl <github@xmit.xyz>
 * DATE   : 2024-03-14
 * 
 * A benchmarking application for IPv6 Voucher-Based Address generation.
 *   Tests different key derivation functions for both generation and
 *   verification against a set of known and expected results.
 * 
 */


#include <chrono>
#include <sstream>
#include <tuple>
#include <vector>
#include <string>

#include "vba.h"
#include "generator.h"

#include "collisions.hpp"


int main()
{
    Xoshiro128p__init();

    rotate_voucher_seed();

    find_collisions_pbkdf2();
    find_collisions_argon2();
    find_collisions_scrypt();
}
