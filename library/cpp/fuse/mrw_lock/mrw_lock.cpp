#include "mrw_lock.h"

namespace NThreading {

TReadWriteGuard TMRWLock::LockRead() {
    return TReadWriteGuard(this, true);
}

TReadWriteGuard TMRWLock::LockWrite() {
    return TReadWriteGuard(this, false);
}


void TMRWLock::AcquireRead() {
    with_lock(Lock_) {
        while (!CanRead()) {
            ReadLockAllowed_.Wait(Lock_);
        }
        WorkingReaders_++;
    }
}

void TMRWLock::AcquireWrite() {
    with_lock(Lock_) {
        WaitingWriters_++;
        while (!CanWrite()) {
            WriteLockAllowed_.Wait(Lock_);
        }
        WaitingWriters_--;
        WorkingWriter_ = true;
    }
}

void TMRWLock::ReleaseRead() {
    with_lock(Lock_) {
        WorkingReaders_--;
        if (CanWrite()) {
            WriteLockAllowed_.Signal();
        }
    }
}

void TMRWLock::ReleaseWrite() {
    with_lock(Lock_) {
        WorkingWriter_ = false;
        if (CanRead()) {
            ReadLockAllowed_.BroadCast();
        }
        else if (CanWrite()) {
            WriteLockAllowed_.Signal();
        }
    }
}

bool TMRWLock::HoldsWriteLock() {
    with_lock(Lock_) {
        return WorkingWriter_ == true;
    }
}

bool TMRWLock::CanRead() {
    return WorkingWriter_ == false && WaitingWriters_ == 0;
}

bool TMRWLock::CanWrite() {
    return WorkingWriter_ == false && WorkingReaders_ == 0;
}

} // namespace NThreading
