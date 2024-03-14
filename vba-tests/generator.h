#ifndef _GENERATOR_H_
#define _GENERATOR_H_


#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>

#include "tinymt64.h"


void Xoshiro128p__init();

uint64_t Xoshiro128p__next_bounded(uint64_t low, uint64_t high);
uint64_t Xoshiro128p__next_bounded_any()
{
    return Xoshiro128p__next_bounded(0, UINT64_MAX);
}


#endif /* _GENERATOR_H_ */
