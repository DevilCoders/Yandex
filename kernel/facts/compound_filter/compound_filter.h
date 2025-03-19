#pragma once

#include <library/cpp/scheme/scheme.h>

#include <util/generic/hash_set.h>
#include <util/generic/noncopyable.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

#include <tuple>


namespace NFacts {

class TCompoundFilter : public TNonCopyable {
    friend class TCompoundFilterTestAdapter;

public:
    TCompoundFilter(TStringBuf configJson);

public:
    TStringBuf Filter(TStringBuf type, TStringBuf source, const TVector<TString>& hostnames, TStringBuf text) const;

private:
    struct TBlacklist {
        THashSet<TStringBuf> Types;
        THashSet<TStringBuf> Sources;
        TVector<TStringBuf> Substrings;
    };

    struct TWhitelist {
        THashSet<TStringBuf> Sources;
        THashSet<TStringBuf> Hostnames;
        THashSet<std::tuple<TStringBuf, TStringBuf>> SourceAndHostnames;
    };

    struct TConfig {
        TBlacklist Blacklist;
        TWhitelist Whitelist;
    };

private:
    NSc::TValue ConfigStorage;
    TConfig Config;
};

}  // namespace NFacts
