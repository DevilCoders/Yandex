#pragma once

#include <util/system/sigset.h>

class TSignalBlockingGuard {
public:
    TSignalBlockingGuard() {
        sigset_t newMask;
        SigFillSet(&newMask);

        SigProcMask(SIG_BLOCK, &newMask, &OldMask);
    }

    ~TSignalBlockingGuard() {
        SigProcMask(SIG_SETMASK, &OldMask, nullptr);
    }

protected:
    sigset_t OldMask;
};
