#include "select_in_word.h"

#include <immintrin.h>

#if !defined(__IOS__)

ui64 SelectInWordBmi2(ui64 val, int rank) noexcept {
    return _tzcnt_u64(_pdep_u64(1ull << rank, val));
}

#endif
