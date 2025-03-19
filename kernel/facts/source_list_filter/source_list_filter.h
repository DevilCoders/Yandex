#pragma once

#include <library/cpp/scheme/scheme.h>

#include <util/generic/hash_set.h>
#include <util/generic/noncopyable.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>


namespace NFacts {

class TSourceListFilter : public TNonCopyable {  // non-copyable because of linkages of TStringBuf to NSc::TValue which requires nontrivial method implementation for proper copying
public:
    enum class EFilterMode {
        Whitelist,  // check if all-of hostnames are listed
        Blacklist,  // check if any-of hostnames is listed
    };

    TSourceListFilter(TStringBuf configJson, EFilterMode filterMode = EFilterMode::Whitelist);
    TSourceListFilter(const TVector<TStringBuf>& configJsons, EFilterMode filterMode = EFilterMode::Whitelist);
    bool IsSourceListed(TStringBuf source, TStringBuf serpType = {}, TStringBuf factType = {}, const TVector<TString>& hostnames = {}) const;

private:
    struct TConfig {
        THashSet<TStringBuf> Sources;
        THashSet<TStringBuf> SerpTypes;
        THashSet<TStringBuf> FactTypes;
        THashSet<TStringBuf> Hostnames;
        EFilterMode FilterMode;
    };

    TVector<NSc::TValue> ConfigStorages;
    TConfig Config;
};

}  // namespace NFacts
