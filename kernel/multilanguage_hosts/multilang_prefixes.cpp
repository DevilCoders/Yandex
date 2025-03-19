#include "multilang_prefixes.h"
#include "multilanguage_hosts.h"

TMultilangPrefixesHolder::TMultilangPrefixesHolder()
{
    Robot = GetRobotMultilangPrefixes();
    Search = GetSearchMultilangPrefixes();
}

TVector<TString> GetMultilangPrefixes(bool robot, bool withseparators) {
    TVector<TString> res;

    for (int i = 0; i < ELR_MAX; ++i) {
        res.emplace_back();
        ELangRegion reg = (ELangRegion)i;
        const char* regstr = LangRegion2Str(reg, robot);

        if (withseparators) {
            NHostMultilangUtils::DoLang2Prefix(regstr, res.back());
        } else {
            res.back() = regstr;
        }
    }

    return res;
}

namespace NHostMultilangUtils {

TString Lang2SearchPrefixRobust(TStringBuf lang) {
    if (!lang)
        ythrow yexception() << "empty language";

    if (IsMultilanguageHost(lang)) {
        return RobotPrefix2SearchPrefix(lang);
    } else {
        return Lang2SearchPrefix(lang);
    }
}

}
