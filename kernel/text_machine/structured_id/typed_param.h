#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/typetraits.h>
#include <util/generic/hash.h>
#include <util/digest/multi.h>

namespace NStructuredId {
    template <typename ParamTraits>
    class TParam {
    public:
        using TThis = TParam<ParamTraits>;
        using TValue = typename ParamTraits::TValue;

        TValue Value = TValue();

    public:
        TParam() = default;
        TParam(const TThis& rhs) = default;
        TParam<ParamTraits>& operator = (const TThis& rhs) = default;

        Y_FORCE_INLINE explicit TParam(typename TTypeTraits<TValue>::TFuncParam value)
            : Value(value)
        {}

        Y_FORCE_INLINE bool operator == (const TThis& rhs) const {
            return Value == rhs.Value;
        }
        Y_FORCE_INLINE bool operator < (const TThis& rhs) const {
            return Value < rhs.Value;
        }
        Y_FORCE_INLINE static TStringBuf GetName() {
            return ParamTraits::GetName();
        }
    };
}

template <typename ParamTraits>
struct THash<::NStructuredId::TParam<ParamTraits>> {
    ui64 operator() (const ::NStructuredId::TParam<ParamTraits>& x) {
        return MultiHash(x.Value);
    }
};
