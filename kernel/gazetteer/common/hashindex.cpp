#include "hashindex.h"

static constexpr unsigned long Y_PRIME_LIST[] = {
    7ul,
    17ul,
    29ul,
    53ul,
    97ul,
    193ul,
    389ul,
    769ul,
    1543ul,
    3079ul,
    6151ul,
    12289ul,
    24593ul,
    49157ul,
    98317ul,
    196613ul,
    393241ul,
    786433ul,
    1572869ul,
    3145739ul,
    6291469ul,
    12582917ul,
    25165843ul,
    50331653ul,
    100663319ul,
    201326611ul,
    402653189ul,
    805306457ul,
    1610612741ul,
    3221225473ul,
    4294967291ul,
};

/** Points to the first prime number in the sorted prime number table. */
static const unsigned long* const Y_FIRST_PRIME = Y_PRIME_LIST;
/** Points to the last prime number in the sorted prime number table.
 *  Note that it is pointing not *past* the last element, but *at* the last element. */
static const unsigned long* const Y_LAST_PRIME = Y_PRIME_LIST + Y_ARRAY_SIZE(Y_PRIME_LIST) - 1;

const unsigned long* NGzt::NPrivate::TPrimeSize::FirstPrime = Y_FIRST_PRIME;
const unsigned long* NGzt::NPrivate::TPrimeSize::LastPrime = Y_LAST_PRIME;
