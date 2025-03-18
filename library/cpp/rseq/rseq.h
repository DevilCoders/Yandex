#include <rseq/rseq.h>

namespace RSeq {

bool DoRegisterCurrentThread();

// RegsiterCurrentThread registers __rseq_abi.
//
// Returns false if initialization failed, because rseq syscall is unavailable.
//
// Very fast for initialized case.
//
// __rseq_abi will be unregistered at some point during thread destruction. Because of this,
// currently it is not safe to use rseq operations from TLS variables destructors.
//
// This function will not cooperate correctly with other libraries using rseq.
//
// This function is not signal safe.
inline bool RegisterCurrentThread()
{
    auto cpu = rseq_current_cpu_raw();
    if (cpu == RSEQ_CPU_ID_REGISTRATION_FAILED) {
        return false;
    } else if (cpu == RSEQ_CPU_ID_UNINITIALIZED) {
        // Assume, that if registration is handled by libc, we will
        // never reach this code.
        return DoRegisterCurrentThread();
    }

    return true;
}

} // namespace RSeq
