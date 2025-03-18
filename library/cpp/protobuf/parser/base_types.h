#pragma once

#include <util/system/defaults.h>
#include <util/generic/cast.h>
#include <util/generic/typelist.h>
#include <util/generic/ymath.h>
#include <util/system/type_name.h>
#include <util/string/cast.h>

namespace NProtoParser {
    namespace NBaseTypesConvertor {
    // * -> string : ToString
    // string -> * : FromString
    //
    // integer -> integer: SafeIntegerCast
    // integer -> float/double: static_cast
    //
    // float/double -> float/double: static_cast
    // float/double -> integer: SafeIntegerCast(static_cast<i64>)
    //
    // integer -> bool: integer != 0

#define TO_SOME_TYPE_FROM_STRING_DEFINITION(templateDecl, to, spec, funcName) \
    templateDecl inline to funcName(const TStringBuf& value) {                \
        return FromString<to>(value);                                         \
    }                                                                         \
                                                                              \
    templateDecl inline to funcName(const TString& value) {                   \
        const TStringBuf stringBuf = value;                                   \
        return funcName spec(stringBuf);                                      \
    }

        template <typename TInt, typename TFloat>
        TInt SafeFloatToIntCast(TFloat f) {
            if (Y_UNLIKELY(IsNan(f)))
                ythrow TBadCastException() << "Conversion 'Nan' to '" << TypeName<TInt>();
            if (Y_UNLIKELY(!IsFinite(f)))
                ythrow TBadCastException() << "Conversion 'Infinity' to '" << TypeName<TInt>();
            if (f >= 0.0) {
                if (Y_UNLIKELY(static_cast<double>(f) > static_cast<double>(Max<ui64>())))
                    ythrow TBadCastException() << "Conversion '" << f << "' to '"
                                               << TypeName<TInt>()
                                               << "', too large value";
                return SafeIntegerCast<TInt>(static_cast<ui64>(f));
            } else {
                if (Y_UNLIKELY(static_cast<double>(f) < static_cast<double>(Min<i64>())))
                    ythrow TBadCastException() << "Conversion '" << f << "' to '"
                                               << TypeName<TInt>()
                                               << "', too large negative value";
                return SafeIntegerCast<TInt>(static_cast<i64>(f));
            }
        }

        // From integer
        template <class TTo, class TFrom>
        TTo ToInteger(TFrom value) {
            return SafeIntegerCast<TTo>(value);
        }

        // From float
        template <class TTo>
        TTo ToInteger(double value) {
            return SafeFloatToIntCast<TTo>(value);
        }

        template <class TTo>
        TTo ToInteger(float value) {
            return SafeFloatToIntCast<TTo>(value);
        }

        // From string
        TO_SOME_TYPE_FROM_STRING_DEFINITION(template <class TTo>, TTo, <TTo>, ToInteger)

        // From integer & float
        template <class TTo, class TFrom>
        TTo ToFloat(TFrom value) {
            return static_cast<TTo>(value);
        }

        // From string
        TO_SOME_TYPE_FROM_STRING_DEFINITION(template <class TTo>, TTo, <TTo>, ToFloat)

        // From integer & float
        template <class TFrom>
        bool ToBool(const TFrom& value) {
            return value != 0;
        }

        // From string
        TO_SOME_TYPE_FROM_STRING_DEFINITION(, bool, , ToBool)

#undef TO_SOME_TYPE_FROM_STRING_DEFINITION
    }
}
