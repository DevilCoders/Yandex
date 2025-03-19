#include "interval.h"

#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/generic/string.h>

template <class TType, typename TChar>
inline NUtil::TInterval<TType> ParseInterval(const TChar* data, size_t length) {
    const TChar* dash = Find(data, data + length, '-');
    size_t position = dash - data;
    try {
        if (!position || position >= length - 1) {
            throw yexception() << "Incorrect dash position: "
                << position;
        }
        return TInterval<TType>(FromString(data, position),
            FromString(dash + 1, length - position - 1));
    } catch (const yexception& exc) {
        throw yexception() << "Failed to parse interval from string "
            << TBasicString<TChar>(data, length).Quote() << ": " << exc.what();
    }
}

#define DECLARE_INTERVAL_FUNCTIONS_IMPL(type, TChar)\
    template <>\
    TInterval<type>\
    FromStringImpl<TInterval<type> >(const TChar* data,\
    size_t length) {\
    return ParseInterval<type>(data, length);\
}

#define DECLARE_INTERVAL_FUNCTIONS(type)\
    DECLARE_INTERVAL_FUNCTIONS_IMPL(type, char)\
    DECLARE_INTERVAL_FUNCTIONS_IMPL(type, wchar16)\
    template <>\
    void Out<TInterval<type> >(IOutputStream& output,\
    TTypeTraits<TInterval<type> >::TFuncParam interval) {\
    const TString& result = interval.ToString();\
    output.Write(result.data(), result.size());\
}

DECLARE_INTERVAL_FUNCTIONS(i16)
DECLARE_INTERVAL_FUNCTIONS(i32)
DECLARE_INTERVAL_FUNCTIONS(i64)
DECLARE_INTERVAL_FUNCTIONS(ui16)
DECLARE_INTERVAL_FUNCTIONS(ui32)
DECLARE_INTERVAL_FUNCTIONS(ui64)
