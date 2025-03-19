#include "lang_cfg.h"

#include <library/cpp/yconf/conf.h>
#include <kernel/lemmer/core/language.h>

#include <util/folder/dirut.h>
#include <util/generic/noncopyable.h>

namespace NLangConfig {

namespace {

inline const char* GetSectionAttr(const TYandexConfig::Section& sec, const char* attr) {
    TYandexConfig::SectionAttrs::const_iterator iAttr = sec.Attrs.find(attr);
    return iAttr == sec.Attrs.end() ? nullptr : iAttr->second;
}

inline const char* GetRequiredAttr(TYandexConfig& yc, const TYandexConfig::Section& sec, const char* attr) {
    const char* value = GetSectionAttr(sec, attr);
    if (nullptr == value) {
        yc.ReportError(sec.Name, false, "Section '%s' must have '%s' attribute", sec.Name, attr);
    }
    return value;
}

class TLangDirectives: public TYandexConfig::Directives {
private:
    const TVector<TString>& RequiredParams;
public:
    TLangDirectives(const TVector<TString>& reqParams)
        : TYandexConfig::Directives(false)
        , RequiredParams(reqParams)
    {
        for (size_t i = 0; i < RequiredParams.size(); ++i) {
            declare(RequiredParams[i].data());
        }
    }
    bool CheckOnEnd(TYandexConfig& yc, TYandexConfig::Section& sec) override {
        TLangDirectives& type = *this;
        for (size_t i = 0; i < RequiredParams.size(); ++i) {
            if ((type[RequiredParams[i]] == nullptr || *(type[RequiredParams[i]]) == 0)) {
                yc.ReportError(sec.Name, true,
                    "Section \'%s\' must include directive \'%s\'. Section will be ignored",
                    sec.Name, RequiredParams[i].data());
                return false;
            }
        }
        if (nullptr == GetRequiredAttr(yc, sec, "Mask")) {
            return false;
        }
        return true;
    }
};

class TConfigImpl : public TYandexConfig {
private:
    TVector<TString> RequiredParams;

protected:
    bool OnBeginSection(Section& sec) override;

public:
    TConfigImpl(TStringBuf requiredParams)
        : TYandexConfig()
    {
        TStringBuf param = requiredParams.NextTok(',');
        while (!param.empty()) {
            RequiredParams.push_back(TString{param});
            param = requiredParams.NextTok(',');
        }
    }
};

BEGIN_CONFIG(TConfigImpl)
    BEGIN_TOPSECTION2(Language, TLangDirectives(RequiredParams))
    END_SECTION()
END_CONFIG()

} // unnamed namespace

TVector<TLangSection> ParseConfig(const TString& cfgPath, const TStringBuf& requiredParams) {
    TConfigImpl yc(requiredParams);

    if (!yc.Parse(cfgPath)) {
        TString err;
        yc.PrintErrors(err);
        ythrow yexception() << "Error of config parsing. " << err;
    }

    TVector<TLangSection> res;

    TLangMask cumulativeLanguageMask;
    TYandexConfig::Section* root = yc.GetRootSection();
    for (TYandexConfig::Section* sec = root ? root->Child : nullptr; sec != nullptr; sec = sec->Next) {
        if (0 != stricmp(sec->Name, "Language"))
            continue;

        res.emplace_back();
        res.back().Langs = NLemmer::GetLanguagesMask(GetRequiredAttr(yc, *sec, "Mask"));
        if (res.back().Langs.Empty()) {
            ythrow yexception() << "Section defines empty language mask.";
        }
        if (res.back().Langs.HasAny(cumulativeLanguageMask)) {
            ythrow yexception() << "Config has non-disjoint language sections.";
        }

        res.back().Params.insert(sec->GetDirectives().begin(), sec->GetDirectives().end());
        cumulativeLanguageMask |= res.back().Langs;
    }
    return res;
}

} // NLangConfig
