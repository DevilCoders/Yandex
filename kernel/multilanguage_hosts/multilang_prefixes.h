#pragma once

#include <kernel/langregion/langregion.h>

#include <utility>
#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>
#include <util/string/vector.h>

TVector<TString> GetMultilangPrefixes(bool robot, bool withseparators);

inline TVector<TString> GetRobotMultilangPrefixes() {
    return GetMultilangPrefixes(true, true);
}

inline TVector<TString> GetSearchMultilangPrefixes() {
    return GetMultilangPrefixes(false, true);
}

inline TVector<TString> GetSearchMultilangLangs() {
    return GetMultilangPrefixes(false, false);
}

class TMultilangPrefixesHolder {
    TVector<TString> Robot;
    TVector<TString> Search;

public:
    TMultilangPrefixesHolder();

    TStringBuf Get(ELangRegion reg, bool robot) const {
        if (reg >= ELR_MAX) {
            ythrow yexception() << "invalid lang region (" << (int)reg << ")";
        }

        return (robot ? Robot : Search)[reg];
    }
};

inline TStringBuf GetRobotMultilangPrefix(ELangRegion reg) {
    return Default<TMultilangPrefixesHolder>().Get(reg, true);
}

inline TStringBuf GetSearchMultilangPrefix(ELangRegion reg) {
    return Default<TMultilangPrefixesHolder>().Get(reg, false);
}

namespace NHostMultilangUtils {

inline TString& DoLang2Prefix(TStringBuf lang, TString& buffer) {
    return !lang ? buffer : buffer.append('@').append(lang).append('.');
}

inline TString& DoRobotPrefix2SearchPrefix(TString& lang) {
    lang.to_lower();
    return lang;
}

inline TString& DoLang2SearchPrefix(TStringBuf lang, TString& buffer) {
    return DoRobotPrefix2SearchPrefix(DoLang2Prefix(lang, buffer));
}

inline TString& DoJoinLangPrefixAndHost(TString& langprefix, TStringBuf host) {
    return langprefix.append(host);
}

inline TString& DoJoinLangPrefixAndHost(TStringBuf langprefix, TStringBuf host, TString& buffer) {
    return DoJoinLangPrefixAndHost(buffer.assign(langprefix), host);
}

inline TString Lang2Prefix(TStringBuf lang) {
    TString lang1;
    return std::forward<TString>(DoLang2Prefix(lang, lang1));
}

inline TString Lang2SearchPrefix(TStringBuf lang) {
    TString lang1;
    return std::forward<TString>(DoLang2SearchPrefix(lang, lang1));
}

inline TString RobotPrefix2SearchPrefix(TStringBuf lang) {
    TString lang1(lang.data(), lang.size());
    return std::forward<TString>(DoRobotPrefix2SearchPrefix(lang1));
}

inline TString JoinLangPrefixAndHost(TStringBuf langprefix, TStringBuf host) {
    TString res = ToString(langprefix);
    return std::forward<TString>(DoJoinLangPrefixAndHost(res, host));
}

TString Lang2SearchPrefixRobust(TStringBuf lang); // accepts both lang codes and lang prefixes

}
