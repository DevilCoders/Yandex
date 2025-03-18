#include "prime.h"

static inline ui64 IntPow(ui64 a, ui64 d, ui64 n) {
    ui64 a_pow = a;
    ui64 res = 1;
    while (d > 0) {
        if ((d & 1) == 1)
            res = (static_cast<ui64>(res) * a_pow) % n;
        a_pow = (static_cast<ui64>(a_pow) * a_pow) % n;
        d /= 2;
    }
    return res;
}

static inline bool CheckWitness(ui64 a_exp_d, ui64 n, ui64 s) {
    ui64 i;
    ui64 a_exp = a_exp_d;
    if (a_exp == 1 || a_exp == (n - 1))
        return true;
    for (i = 1; i < s; i++) {
        a_exp = (static_cast<ui64>(a_exp) * a_exp) % n;
        if (a_exp == (n - 1))
            return true;
    }
    return false;
}

bool IsPrime(ui64 n) {
    ui64 a, d, s, a_exp_d;
    if (n == 1) {
        return false;
    }
    if ((n % 2) == 0)
        return false;
    if ((n % 3) == 0)
        return false;
    if ((n % 5) == 0)
        return false;
    if ((n % 7) == 0)
        return false;
    //we decompoe the number n - 1 into 2^s*
    s = 0;
    d = n - 1;
    do {
        s++;
        d /= 2;
    } while ((d % 2) == 0);

    a = 2;
    a_exp_d = IntPow(a, d, n);
    if (CheckWitness(a_exp_d, n, s) == 0)
        return false;
    a = 7;
    a_exp_d = IntPow(a, d, n);
    if (CheckWitness(a_exp_d, n, s) == 0)
        return false;
    a = 61;
    a_exp_d = IntPow(a, d, n);
    if (CheckWitness(a_exp_d, n, s) == 0)
        return false;
    return true;
}
