#pragma once

#include <util/generic/ptr.h>

class IInputStream;

class TAllocatorStats {
    public:
        TAllocatorStats();
        ~TAllocatorStats();

        bool Read(IInputStream* s);
        void Print() const;

    private:
        class TImpl;
        THolder<TImpl> mImpl;
};
