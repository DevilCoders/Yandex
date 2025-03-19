#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>

namespace NSnippets {
    class TPornodict : private TNonCopyable {
    private:
        struct TImpl;

        THolder<TImpl> Impl;
    public:
        TPornodict();
        ~TPornodict();
        double GetWeight(const TUtf16String& w) const;
        static const TPornodict& GetDefault();
    };
}
