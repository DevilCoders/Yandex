#pragma once

#include <functional>
#include <util/stream/output.h>
#include <util/ysaveload.h>
#include <util/generic/typetraits.h>

template <class T, bool IsPod>
class TSizeHelper;

template <class T>
class TSizeHelper<T, false> {
private:
    class TSizeCounter: public IOutputStream {
    public:
        TSizeCounter()
            : Size_(0)
        {
        }

        void DoWrite(const void*, size_t len) override {
            Size_ += len;
        }

        size_t Size() const {
            return Size_;
        }

    private:
        size_t Size_;
    };

public:
    size_t operator()(const T& t) const {
        TSizeCounter c;
        Save(&c, t);
        return c.Size();
    }
};

template <class T>
class TSizeHelper<T, true> {
public:
    size_t operator()(const T&) const {
        return sizeof(T);
    }
};

template <class T>
class TSize: public TSizeHelper<T, TTypeTraits<T>::IsPod> {
};
