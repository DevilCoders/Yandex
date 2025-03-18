#pragma once

#include <util/system/mutex.h>

template <class T>
class TAtomicCopier {
public:
    TAtomicCopier() {
    }

    TAtomicCopier(const T& item) {
        *this = item;
    }

    void operator=(const T& other) {
        TGuard<TMutex> g(lock);
        item = other;
    }

    operator T() {
        TGuard<TMutex> g(lock);
        return item;
    }

private:
    TAtomicCopier(const TAtomicCopier&);

    T item;
    TMutex lock;
};
