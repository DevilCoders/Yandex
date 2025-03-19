#pragma once

#include <util/system/rwlock.h>

class TMutexOwner {
public:
    inline explicit TMutexOwner(bool readOnly)
        : ReadOnly(readOnly)
    {}

    inline TAutoPtr<TReadGuard> ReadGuard() const {
        return ReadOnly ? nullptr : new TReadGuard(Mutex);
    }

    inline TAutoPtr<TWriteGuard> WriteGuard() const {
        return ReadOnly ? nullptr : new TWriteGuard(Mutex);
    }

    inline void SetReadOnly() {
        ReadOnly = true;
    }
private:
    TRWMutex Mutex;
    bool ReadOnly;
};
