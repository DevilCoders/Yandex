#pragma once
#include <util/generic/serialized_enum.h>
#include <util/generic/set.h>
#include <util/string/cast.h>

namespace NCS {

    template <class TEnum>
    class TEnumWorker {
    public:
        static TSet<TEnum> GetInterval(const TEnum from, const TEnum to, const bool includeTo = false) {
            TSet<TEnum> result;
            for (auto&& i : GetEnumAllValues<TEnum>()) {
                if ((i64)i < (i64)from) {
                    continue;
                }
                if ((i64)i > (i64)to) {
                    continue;
                }
                if (i == to && !includeTo) {
                    continue;
                }
                result.emplace(i);
            }
            return result;
        }

        static bool TryParseFromInt(const i64 intValue, TEnum& result) {
            for (auto&& i : GetEnumAllValues<TEnum>()) {
                if ((i64)i == intValue) {
                    result = i;
                    return true;
                }
            }
            return false;
        }

        static bool TryParseFromString(const TStringBuf strValue, TEnum& result) {
            if (::TryFromString(strValue, result)) {
                return true;
            }
            i64 intValue;
            if(!::TryFromString(strValue, intValue)) {
                return false;
            }
            return TryParseFromInt(intValue, result);
        }

        static TSet<TString> GetEnumNameSet() {
            TSet<TString> result;
            for (auto&& i : GetEnumNames<TEnum>()) {
                result.emplace(i.second);
            }
            return result;
        }
    };
}
