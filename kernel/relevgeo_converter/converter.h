#pragma once

#include <kernel/relev_locale/relev_locale.h>
#include <kernel/searchlog/errorlog.h>

#include <kernel/groupattrs/metainfo.h>
#include <kernel/region2country/countries.h>
#include <kernel/search_types/search_types.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/langs/langs.h>

#include <util/folder/dirut.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/mapfindptr.h>
#include <util/generic/set.h>
#include <util/memory/blob.h>
#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/string/strip.h>


struct TRequestPart {
public:
    TRequestPart(const TString& source, const TString& queryType, const TString& queryLanguage, const TString& experiment, const TString& userInterfaceLanguage )
        : Source(source)
        , QueryType(queryType)
        , QueryLanguage(queryLanguage)
        , Experiment(experiment)
        , UserInterfaceLanguage(userInterfaceLanguage)
    {
    }

    TRequestPart(const TString& source, const TString& queryType, const TString& queryLanguage)
        : Source(source)
        , QueryType(queryType)
        , QueryLanguage(queryLanguage)
        , Experiment("")
        , UserInterfaceLanguage("")
    {
    }

    inline bool operator == (const TRequestPart& p) const noexcept {
        return p.Source == Source && p.QueryType == QueryType && p.QueryLanguage == QueryLanguage && p.Experiment == Experiment && p.UserInterfaceLanguage == UserInterfaceLanguage;
    }

    inline bool IsExperiment() const noexcept {
        return Experiment != "";
    }

    inline bool IsUil() const noexcept {
        return UserInterfaceLanguage != "";
    }

    inline TRequestPart WithoutExperiment() const noexcept {
        return TRequestPart(Source, QueryType, QueryLanguage, "", UserInterfaceLanguage);
    }

    inline TRequestPart OnlySource() const noexcept {
        return TRequestPart(Source, AnyLang(), AnyLang(), Experiment, UserInterfaceLanguage);
    }

    inline TRequestPart OnlyQueryType() const noexcept {
        return TRequestPart(AnyLang(), QueryType, AnyLang(), Experiment, UserInterfaceLanguage);
    }

    inline TRequestPart WithoutQueryType() const noexcept {
        return TRequestPart(Source, AnyLang(), QueryLanguage, Experiment, UserInterfaceLanguage);
    }
    inline TRequestPart WithoutQueryLanguage() const noexcept {
        return TRequestPart(Source, QueryType, AnyLang(), Experiment, UserInterfaceLanguage);
    }

    inline TRequestPart Default() const noexcept {
        return TRequestPart(AnyLang(), AnyLang(), AnyLang(), Experiment, UserInterfaceLanguage);
    }
public:
    static const TString& AnyLang();
public:
    TString Source;
    TString QueryType;
    TString QueryLanguage;
    TString Experiment;
    TString UserInterfaceLanguage;
};

struct TMultiRequestPart {
public:
    TMultiRequestPart(const TString& source, const TString& queryType, const TString queryLanguage, const TSet<TString>& experiments, const TString& userInterfaceLanguage)
        : Source(source)
        , QueryType(queryType)
        , QueryLanguage(queryLanguage)
        , ExperimentSet(experiments)
        , UserInterfaceLanguage(userInterfaceLanguage)
    {
        ExperimentSequence = "";
        for (TSet<TString>::const_iterator it = ExperimentSet.begin(); it != ExperimentSet.end(); ++it) {
            ExperimentSequence += *it;
            ExperimentSequence.append('\t');
        }
    }
public:
    inline const TRequestPart MakeExperimentRequestPart(TString experiment) const {
        return TRequestPart(Source, QueryType, QueryLanguage, experiment, "");
    }
    inline const TRequestPart MakeUilRequestPart() const {
        return TRequestPart(Source, QueryType, QueryLanguage, "", UserInterfaceLanguage);
    }
    inline bool operator == (const TMultiRequestPart& p) const noexcept {
        return p.Source == Source && p.QueryType == QueryType && p.QueryLanguage == QueryLanguage && p.ExperimentSequence == ExperimentSequence && p.UserInterfaceLanguage == UserInterfaceLanguage;
    }

public:
    TString Source;
    TString QueryType;
    TString QueryLanguage;
    const TSet<TString> ExperimentSet;
    TString ExperimentSequence;
    TString UserInterfaceLanguage;
};


class TRegionData {
public:
    TRegionData()
        : RelevGeo(END_CATEG)
        , SnipGeo(END_CATEG)
    {
    }

    TRegionData(TCateg relevGeo, TCateg snipGeo)
        : RelevGeo(relevGeo)
        , SnipGeo(snipGeo)
    {
    }

    TCateg GetRelevGeo() const {
        return RelevGeo;
    }

    TCateg GetSnipGeo() const {
        return SnipGeo;
    }

private:
    TCateg RelevGeo;
    TCateg SnipGeo;
};

typedef THashMap<TCateg, TRegionData> TRegionsMap;

template <>
struct hash<TRequestPart> {
    inline size_t operator() (const TRequestPart& t) const noexcept {
        return (ComputeHash(t.Source) ^ ComputeHash(t.QueryType) ^ ComputeHash(t.QueryLanguage) ^ ComputeHash(t.Experiment) ^ ComputeHash(t.UserInterfaceLanguage));
    }
};
template <>
struct hash<TMultiRequestPart> {
    inline size_t operator() (const TMultiRequestPart& t) const noexcept {
        return (ComputeHash(t.Source) ^ ComputeHash(t.QueryType) ^ ComputeHash(t.QueryLanguage) ^ ComputeHash(t.ExperimentSequence) ^ ComputeHash(t.UserInterfaceLanguage));
    }
};

class TConverter {
private:
    TRegionsMap Regions;
public:
    TRegionData DefaultRegion;

    bool Ignore = false;
    bool Disable = false;
    bool Dummy = false;

private:
    TConverter(bool ignore, bool disable, bool dummy) noexcept
        : Ignore(ignore)
        , Disable(disable)
        , Dummy(dummy)
    {
    }

public:
    TConverter(const TString& regionsFilePath, TArchiveReader* archive = nullptr);

    struct TConvertRegionCtx {
        TCateg OriginalRegion = END_CATEG;
        const NGroupingAttrs::TMetainfo* RegionAttr = nullptr;
        const TVector<TCateg>* AllRegions = nullptr;
    };

    const TRegionData ConvertRegion(const TConvertRegionCtx& convertCtx, TString* error) const;

    bool IsIgnore() const noexcept {
        return Ignore;
    }

    bool IsDisable() const noexcept {
        return Disable;
    }

    bool IsDummy() const noexcept {
        return Dummy;
    }

public:
    static const TConverter IgnoreConverter;
    static const TConverter DisableConverter;
    static const TConverter DummyConverter;
};

typedef THashMap<TRequestPart, const TConverter*> TConverterByPart;
typedef THashMap<TString, const TConverter*> TConverterByName;
typedef THashSet<TString> TKnownTypes;
typedef THashSet<TString> TKnownLanguages;
typedef THashSet<TRequestPart> TRequestParts;

class TConvertRules {
public:
    TConvertRules()
        : TConvertRules("rules.txt", "lang_rules.txt")
    {
    }
    TConvertRules(const TString& rulesDirectory, const TString& rulesFilename, const TString& langRulesFilename);

    TConvertRules(const TString& rulesFilename, const TString& langRulesFilename);

    void FixUnknownInfo(TMultiRequestPart& part) const {
        if (!KnownTypes.contains(part.QueryType)) {
            part.QueryType = TRequestPart::AnyLang();
        }

        if (!KnownLanguages.contains(part.QueryLanguage)) {
            part.QueryLanguage = TRequestPart::AnyLang();
        }
    }

    const TConverter* GetRegionConverter(const TMultiRequestPart& mp, bool isSpok) const {
        if (isSpok) {
            return &TConverter::DummyConverter;
        }

        for (TSet<TString>::const_iterator it = mp.ExperimentSet.begin(); it != mp.ExperimentSet.end(); ++it) {
            const TConverter* experimentRuleResult = GetRegionConverter(mp.MakeExperimentRequestPart(*it));
            if (experimentRuleResult != nullptr) {
                return experimentRuleResult;
            }
        }
        const TConverter* uilRuleResult = GetRegionConverter(mp.MakeUilRequestPart());
        if (uilRuleResult != nullptr)
            return uilRuleResult;
        return GetRegionConverter(mp.MakeExperimentRequestPart(""));
    }

public:
    struct TQueryInfo {
        NRl::ERelevLocale RelevLocale = NRl::RL_UNIVERSE;
        TString QueryType;
        TString QueryLanguage;
        TSet<TString> Experiments;
        TString UserInterfaceLanguage;

    };

    TRegionData ConvertRegionSimple(const TQueryInfo& query, const TConverter::TConvertRegionCtx& ctx);
private:
    const TConverter* GetRegionConverter(const TRequestPart& p) const;
    void LoadRulesFile(const TString rulesFileName, bool isQueryTypeRulesFile, TArchiveReader* archive);
    const TConverter* LoadOrCreateRegionConverter(const TString& name, TArchiveReader* archive);
    const TConverter* LoadRegionConverter(const TString& name, TArchiveReader* archive);
private:
    TString RulesDirectory;

    TConverterByPart ByPart;
    TConverterByName ByName;
    TVector<TAutoPtr<TConverter> > Holder;

    TKnownTypes KnownTypes;
    TKnownLanguages KnownLanguages;
};

