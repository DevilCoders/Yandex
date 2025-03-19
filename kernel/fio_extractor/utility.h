#pragma once

#include <util/generic/yexception.h>
#include <utility>

struct TMaxNumberExceeded: public yexception {
    TMaxNumberExceeded() {
        *this << "Max Number Exceeded";
    }
};

class TLimitedCounter {
public:
    explicit TLimitedCounter(size_t maxFioCount = Max())
        : MaxFioCount(maxFioCount)
        , FioCount(0)
    {
    }

    void Count() {
        if (FioCount == MaxFioCount)
            ythrow TMaxNumberExceeded();
        ++FioCount;
    }

    size_t GetCount() const {
        return FioCount;
    }

private:
    const size_t MaxFioCount;
    size_t FioCount;
};

//template<template<typename> class Wrapper, class Callback, class... Args>
//Wrapper<Callback> Wrap(Callback&& r, Args&&... args) {
//    return Wrapper<Callback>(std::forward<Callback>(r), std::forward<Args>(args)...);
//}
