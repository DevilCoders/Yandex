#include "rank.h"

#include <util/string/cast.h>

namespace NSolveAmbig {
    namespace {
        struct TDefaultRankMethod: public TRankMethod {
            explicit TDefaultRankMethod()
                : TRankMethod(3u, RC_G_COVERAGE, RC_L_COUNT, RC_G_WEIGHT)
            {
            }
        };

        inline void Parse(TStringBuf s, TVector<ERankCheck>& checks) {
            TStringBuf check;
            while (!(check = s.NextTok(',')).empty()) {
                checks.push_back(FromString(check));
            }
        }

    } // unnamed namespace

    TRankMethod::TRankMethod(size_t num, ...)
        : TVector<ERankCheck>()
    {
        reserve(num);

        va_list args;
        va_start(args, num);
        for (size_t n = 0; n < num; ++n) {
            push_back(static_cast<ERankCheck>(va_arg(args, int)));
        }
        va_end(args);
    }

    TRankMethod::TRankMethod(const TStringBuf& s)
        : TVector<ERankCheck>()
    {
        Parse(s, *this);
    }

    const TRankMethod& DefaultRankMethod() {
        return Default<TDefaultRankMethod>();
    }

}
