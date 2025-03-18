#include "pid_ring.h"

namespace NFuse {

std::array<pid_t, TPidRing::SIZE> TPidRing::Read() const {
    std::array<pid_t, SIZE> res;
    for (size_t i = 0; i < SIZE; i++) {
        res[i] = Buf[i].load(std::memory_order_relaxed);
    }
    return res;
}

} // namespace NFuse
