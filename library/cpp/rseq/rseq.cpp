#include <syscall.h>
#include <unistd.h>

#include <util/system/compiler.h>
#include <util/system/yassert.h>
#include <util/system/thread.h>
#include <util/thread/singleton.h>

#include "rseq.h"

Y_WEAK __thread struct rseq __rseq_abi = {
	.cpu_id = static_cast<__u32>(RSEQ_CPU_ID_UNINITIALIZED),
};

#ifndef __NR_rseq
#define __NR_rseq 334
#endif

static int sys_rseq(struct rseq *rseq_abi, uint32_t rseq_len, int flags, uint32_t sig)
{
	return syscall(__NR_rseq, rseq_abi, rseq_len, flags, sig);
}

namespace RSeq {

struct TRegistrator
{
    TRegistrator()
    {
        ThisThread = TThread::CurrentThreadId();

        int rc = sys_rseq(&__rseq_abi, sizeof(struct rseq), 0, RSEQ_SIG);
	    if (rc) {
            __rseq_abi.cpu_id = RSEQ_CPU_ID_REGISTRATION_FAILED;
        }
    }

    ~TRegistrator()
    {
        if (__rseq_abi.cpu_id == static_cast<__u32>(RSEQ_CPU_ID_REGISTRATION_FAILED)) {
            return;
        }

        // __rseq_abi must not be deallocated until thread exit or rseq(UNREGISTER) is called.
        //
        // We can't guarantee that thread local variable will not be deallocated before thread exit,
        // so we must unregister __rseq_abi manually.

		int rc = sys_rseq(&__rseq_abi, sizeof(struct rseq), RSEQ_FLAG_UNREGISTER, RSEQ_SIG);
        Y_VERIFY(rc == 0);
        Y_VERIFY(ThisThread == TThread::CurrentThreadId());
    }

    TThread::TId ThisThread;
};

bool DoRegisterCurrentThread()
{
    FastTlsSingleton<TRegistrator>();
    return __rseq_abi.cpu_id != static_cast<__u32>(RSEQ_CPU_ID_REGISTRATION_FAILED);
}

} // namespace RSeq
