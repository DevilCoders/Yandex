#pragma once

#include <util/generic/noncopyable.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>

namespace NThreading {

class TReadWriteGuard;

/**
 * Read/Write lock that supports lock and unlock from different threads
 * Implementation is similar to TRWMutex, Windows/Max version, because pthread_mutex'es
 * cannot be unlocked in different thread.
 */
class TMRWLock {
public:
    void AcquireRead();
    void AcquireWrite();
    void ReleaseRead();
    void ReleaseWrite();
    TReadWriteGuard LockRead();
    TReadWriteGuard LockWrite();
    bool HoldsWriteLock();
private:
    bool CanRead();
    bool CanWrite();
private:
    TMutex Lock_;
    int WorkingReaders_ = 0;
    bool WorkingWriter_ = false;
    int WaitingWriters_ = 0; // new readers should wait if there are writers in the queue
    TCondVar WriteLockAllowed_;
    TCondVar ReadLockAllowed_;
};

// read/write guard that can be passed between threads
class TReadWriteGuard : public TMoveOnly {
public:
    TReadWriteGuard()
        : Lock_(nullptr)
        , Type_(EType::None)
    {
    }

    TReadWriteGuard(TMRWLock* lock, bool isReadLock)
        : Lock_(lock)
        , Type_(isReadLock ? EType::Read : EType::Write)
    {
        if (isReadLock) {
            Lock_->AcquireRead();
        } else {
            Lock_->AcquireWrite();
        }
    }

    TReadWriteGuard(TReadWriteGuard&& other) {
        Swap(other);
    }

    TReadWriteGuard& operator=(TReadWriteGuard&& other) noexcept {
        Swap(other);
        return *this;
    }

    void Swap(TReadWriteGuard& other) {
        std::swap(this->Lock_, other.Lock_);
        std::swap(this->Type_, other.Type_);
    }

    ~TReadWriteGuard() {
        Release();
    }

    bool HoldsReadLock() const {
        return Type_ == EType::Read;
    }

    bool HoldsWriteLock() const {
        return Type_ == EType::Write;
    }

    void Release() {
        switch (Type_) {
            case EType::Read:
                Lock_->ReleaseRead();
                break;
            case EType::Write:
                Lock_->ReleaseWrite();
                break;
            case EType::None:
                // nop
                break;
        }
        Type_ = EType::None;
    }

private:
    enum class EType {
        None,
        Read,
        Write
    };

    TMRWLock* Lock_;
    EType Type_;
};

} // namespace NThreading
