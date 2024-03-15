#include "generate_verify.hpp"

#include "vba.h"


static inline void _generate_and_verify(VbaAlgorithm);


void generate_and_verify_pbkdf2()
{
    return _generate_and_verify(PBKDF2);
}

void generate_and_verify_argon2()
{
    return _generate_and_verify(ARGON2);
}

void generate_and_verify_scrypt()
{
    return _generate_and_verify(SCRYPT);
}


static inline void
_generate_and_verify(VbaAlgorithm algorithm)
{
}