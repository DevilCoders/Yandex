#pragma once

#include <library/cpp/colorizer/colors.h>

#include <util/stream/output.h>

namespace NTvmknife::NOutput {
    class TOutStream {
    public:
        TOutStream(IOutputStream& stream);
        ~TOutStream();

        IOutputStream& Stream;
        NColorizer::TColors Color;
    };

    template <class T>
    static inline TOutStream&& operator<<(TOutStream&& out, const T& t) {
        out.Stream << t;
        return std::move(out);
    }

    TOutStream Out();

    bool& GetQuiteFlag();
}
