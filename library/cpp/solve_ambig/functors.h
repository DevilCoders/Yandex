#pragma once

#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/system/defaults.h>

namespace NSolveAmbig {
    namespace NImpl {
        struct TNop {
            template <typename T>
            Y_FORCE_INLINE void operator()(const T&) const {
            }
        };

        template <typename T>
        struct TMoveOp {
            TVector<T>& Result;
            TMoveOp(TVector<T>& r)
                : Result(r)
            {
            }

            Y_FORCE_INLINE void operator()(T& o) const {
                Result.emplace_back();
                DoSwap(Result.back(), o);
            }
        };

    }

}
