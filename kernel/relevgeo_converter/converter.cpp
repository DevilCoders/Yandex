#include "converter.h"
#include <util/string/split.h>

const TString& TRequestPart::AnyLang() {
    static const TString anyLang("*");
    return anyLang;
}

static const unsigned char ConfigurationArchive[] = {
    #include <kernel/relevgeo_converter/rules.inc>
};

constexpr TStringBuf IgnoreRegionsName = "-";
constexpr TStringBuf DisableRegionsName = "x";

TConverter::TConverter(const TString& regionsFilePath, TArchiveReader* archive) {
    TAutoPtr<IInputStream> regionsFile = archive ? archive->ObjectByKey(regionsFilePath) : new TFileInput(regionsFilePath);

    TString line;
    while (regionsFile->ReadLine(line)) {

        if (line.empty() || line[0] == '#') {
            continue;
        }

        TVector<TString> parts;
        StringSplitter(line).Split('\t').AddTo(&parts);

        if (parts.size() < 2 || parts.size() > 3) {
            ythrow yexception() << "Wrong line in file " << regionsFilePath << ": " << line;
        }

        const TCateg parentRegion = FromString<TCateg>(parts[0]);
        const TCateg representativeRegion = FromString<TCateg>(parts[1]);
        const TCateg snipRegion = (parts.size() < 3)? END_CATEG : FromString<TCateg>(parts[2]);

        Regions.insert(TRegionsMap::value_type(parentRegion, TRegionData(representativeRegion, snipRegion)));

        if (DefaultRegion.GetRelevGeo() == END_CATEG) {
            DefaultRegion = TRegionData(representativeRegion, snipRegion);
        }
    }
}

const TRegionData TConverter::ConvertRegion(const TConvertRegionCtx& convertCtx, TString* error) const {
    Y_ASSERT(!Ignore && !Disable && (convertCtx.RegionAttr || convertCtx.AllRegions));

    TCateg region = convertCtx.OriginalRegion;

    if (Dummy) {
        return TRegionData(region, region);
    }

    if (convertCtx.RegionAttr) {
        while (region != END_CATEG) {
            if (const TRegionData* regionData = Regions.FindPtr(region)) {
                return *regionData;
            }
            region = convertCtx.RegionAttr->Categ2Parent(region);
        }
    }

    if (convertCtx.AllRegions && !convertCtx.AllRegions->empty()) {
        for (const TCateg& r : *convertCtx.AllRegions) {
            if (const TRegionData* regionData = Regions.FindPtr(r)) {
                return *regionData;
            }
        }

        if (error) {
            *error = "can't convert region via all_regions";
        }

        return TRegionData(convertCtx.AllRegions->back(), END_CATEG);
    }

    if (error) {
        *error = "can't convert region via all_regions and geo.c2p";
    }

    return DefaultRegion;
}

const TConverter TConverter::IgnoreConverter(true, false, false);
const TConverter TConverter::DisableConverter(false, true, false);
const TConverter TConverter::DummyConverter(false, false, true);

TConvertRules::TConvertRules(const TString& rulesDirectory, const TString& rulesFilename, const TString& langRulesFilename)
    : RulesDirectory(rulesDirectory)
{
    LoadRulesFile(rulesFilename, true, nullptr);

    TString langRulesFilePath = RulesDirectory + "/" + langRulesFilename;
    if (langRulesFilename != "" && NFs::Exists(langRulesFilePath))
        LoadRulesFile(langRulesFilename, false, nullptr);
}

TConvertRules::TConvertRules(const TString& rulesFilename, const TString& langRulesFilename) {
    TArchiveReader archive(TBlob::NoCopy(ConfigurationArchive, sizeof(ConfigurationArchive)));

    LoadRulesFile(rulesFilename, true, &archive);

    if (!langRulesFilename.empty()) {
        // Check that langRulesFilename exists in the archive
        bool hasLangFile = true;
        try {
            (void)archive.BlobByKey((TString("/") + langRulesFilename).c_str());
        } catch(yexception e) {
            hasLangFile = false;
        }

        if (hasLangFile) {
            LoadRulesFile(langRulesFilename, false, &archive);
        }
    }
}

const TConverter* TConvertRules::GetRegionConverter(const TRequestPart& p) const {
    TConverterByPart::const_iterator it = ByPart.find(p.WithoutQueryLanguage());
    const TConverter* queryTypeResult = nullptr;

    // using lang rules
    if (p.QueryLanguage != TRequestPart::AnyLang()) {
        TConverterByPart::const_iterator it2 = ByPart.find(p.WithoutQueryType());
        if (it2 != ByPart.end()) {
            // send using query language route rules
            return it2->second;
        }
    }

    //try to send query using its query type route rules
    if (it != ByPart.end()) {
        queryTypeResult = it->second;
        return queryTypeResult;
    }
    // try to send query using its language route rules
    it = ByPart.find(p.WithoutQueryType());
    if (it != ByPart.end()) {
        // send using query language route rules
        return it->second;
    } else if (queryTypeResult) {
        // do not send due to query type route rules
        return queryTypeResult;
    }

    it = ByPart.find(p.OnlySource());
    if (it != ByPart.end()) {
        return it->second;
    }

    it = ByPart.find(p.OnlyQueryType());
    if (it != ByPart.end()) {
        return it->second;
    }

    it = ByPart.find(p.Default());
    if (it != ByPart.end()) {
        return it->second;
    }

    if (p.IsExperiment() || p.IsUil()) {
        return nullptr;
    }

    return &TConverter::DisableConverter;
}

void TConvertRules::LoadRulesFile(const TString rulesFileName, bool isQueryTypeRulesFile, TArchiveReader* archive) {
    TString rulesFilePath = archive == nullptr ? RulesDirectory + "/" + rulesFileName : TString("/") + rulesFileName;
    TAutoPtr<IInputStream> input = archive == nullptr ? new TFileInput(rulesFilePath): archive->ObjectByKey(rulesFilePath);

    TString line;
    while (input->ReadLine(line)) {

        if (line.empty())
            continue;

        TVector<TString> parts;
        StringSplitter(line).Split('\t').AddTo(&parts);

        if (parts.size() < 3 || parts.size() > 5) {
            ythrow yexception() << "Wrong line in file " << rulesFilePath << ": " << line;
        }
        for (int partIdx = 0; partIdx < parts.ysize(); ++partIdx) {
            parts[partIdx] = StripInPlace(parts[partIdx]);
        }

        TString experiment = "";
        if (parts.size() >= 4) {
            experiment = parts[3];
        }
        TString userInterfaceLanguage = "";
        if (parts.size() >= 5) {
            userInterfaceLanguage = parts[4];
        }
        TRequestPart requestPart(parts[0], TRequestPart::AnyLang(), TRequestPart::AnyLang(), experiment, userInterfaceLanguage);
        if (isQueryTypeRulesFile)
            requestPart.QueryType = parts[1];
        else
            requestPart.QueryLanguage = parts[1];
        const TString& regionsName = parts[2];

        if (ByPart.contains(requestPart)) {
            if (isQueryTypeRulesFile)
                ythrow yexception() << "There are 2 configurations for source '" << requestPart.Source << "' and query type '" << requestPart.QueryType << "'";
            else
                ythrow yexception() << "There are 2 configurations for source '" << requestPart.Source << "' and query language '" << requestPart.QueryLanguage << "'";
        }

        ByPart[requestPart] = LoadOrCreateRegionConverter(regionsName, archive);

        //for every rule excepting *-sources create its _TDI
        if (requestPart.Source != "*") {
            requestPart.Source += "_TDI";
            if (ByPart.find(requestPart) == ByPart.end())
                ByPart[requestPart] = LoadOrCreateRegionConverter(regionsName, archive);
        }

        if (isQueryTypeRulesFile) {
            KnownTypes.insert(requestPart.QueryType);
        } else {
            KnownLanguages.insert(requestPart.QueryLanguage);
        }
    }
}

const TConverter* TConvertRules::LoadOrCreateRegionConverter(const TString& name, TArchiveReader* archive) {
    if (name == IgnoreRegionsName) {
        return &TConverter::IgnoreConverter;
    }
    if (name == DisableRegionsName) {
        return &TConverter::DisableConverter;
    }

    TConverterByName::const_iterator it = ByName.find(name);

    if (it != ByName.end()) {
        return it->second;
    }

    const TConverter* c = LoadRegionConverter(name, archive);
    ByName[name] = c;
    return c;
}

const TConverter* TConvertRules::LoadRegionConverter(const TString& name, TArchiveReader* archive) {
    TString path = TString("/") + name + ".txt";
    if (archive == nullptr) {
        path = RulesDirectory + path;
    }

    TAutoPtr<TConverter> c(new TConverter(path, archive));
    Holder.push_back(c);

    return Holder.back().Get();
}

TRegionData TConvertRules::ConvertRegionSimple(const TQueryInfo& query, const TConverter::TConvertRegionCtx& ctx) {
    TMultiRequestPart mp("WEB", query.QueryType, query.QueryLanguage, query.Experiments, query.UserInterfaceLanguage);
    FixUnknownInfo(mp);
    const bool isSpok = NRl::IsLocaleDescendantOf(query.RelevLocale, NRl::RL_SPOK);
    TString error;
    return GetRegionConverter(mp, isSpok)->ConvertRegion(ctx, &error);
}
