#pragma once

#include <array>
#include <atomic>

#include <sys/types.h>
#include <unistd.h>

namespace NFuse {

class TPidRing {
public:
    static constexpr size_t SIZE = 256;

    void Put(pid_t pid) {
        Buf[Pos].store(pid, std::memory_order_relaxed);
        Pos = (Pos + 1) % SIZE;
    }

    std::array<pid_t, SIZE> Read() const;

private:
    std::array<std::atomic<pid_t>, SIZE> Buf = {};
    size_t Pos = 0;
};

} // namespace NFuse
