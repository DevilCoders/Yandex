#pragma once

#include <util/system/defaults.h>
/*
Implemenation of Miller-Rabin primality test
http://en.wikipedia.org/wiki/Miller-Rabin
*/

bool IsPrime(ui64 n);

template <typename T>
T NextPrime(T n) {
    T nn = (n % 2 == 0 ? n + 1 : n + 2);
    while (!IsPrime(nn))
        nn += 2;
    return nn;
}
