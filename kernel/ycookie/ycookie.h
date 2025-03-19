#pragma once

#include <util/generic/hash_set.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <library/cpp/string_utils/quote/quote.h>

namespace NYCookie {
    static const TString EXPIRES_FIELD = "expires";
    static const TString VALUE_FIELD = "value";

    static const THashSet<TString> SUPPORTED_CONTAINERS = {"ys", "yp", "yc"};
    static const THashSet<TString> PERMANENT = {"yp", "yc"};

    // ys - session cookie format: name1.value1#name2.value2#name3.value3
    // yp - permanent cookie format: expire1.name1.value1#expire2.name2.value2#expire3.name3.value3
    // yc - client-side permanent cookie
    template<typename T>
    void Parse(const TStringBuf name, TStringBuf s, T& dst) {
        const bool hasExpire = PERMANENT.contains(name);
        TStringBuf tok;
        while (s.NextTok('#', tok)) {
            TStringBuf k, v;
            tok.Split('.', k, v);
            if (hasExpire) {
                T val;
                val[EXPIRES_FIELD] = k;
                v.Split('.', k, v);
                val[VALUE_FIELD] = CGIUnescapeRet(TString{v});
                dst[k] = val;
            } else {
                dst[k] = CGIUnescapeRet(TString{v});
            }
        }
    }

    template<typename T>
    bool TryParse(const TStringBuf name, TStringBuf s, T& dst) {
        if (SUPPORTED_CONTAINERS.contains(name)) {
            Parse(name, s, dst);
            return true;
        }

        return false;
    }
}
