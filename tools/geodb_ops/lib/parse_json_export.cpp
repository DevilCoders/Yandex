#include "parse_json_export.h"

#include <kernel/search_types/search_types.h>
#include <kernel/geodb/entity.h>
#include <kernel/geodb/geodb.h>

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/logger/global/global.h>

#include <util/charset/utf8.h>
#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/string/type.h>
#include <util/string/vector.h>

#include <tuple>
#include <utility>

static const TVector<TString> FIELDS{
    "Id",
    "Type",
    "Main",
    "Path",
    "chief_region",
    "TzName",
    "TzOffset",
    "ShortName",
    "ZipCode",
    "Lat",
    "Lon",
    "Population",
    "PhoneCode",
    "SpnLat",
    "SpnLon",
    "Tv",
    "Ad",
    "Afisha",
    "Maps",
    "Etrain",
    "Subway",
    "Weather",
    "native_name",
    "synonyms"
};

static const TStringBuf NOMINATIVE("");
static const TStringBuf GENITIVE("Genitive");
static const TStringBuf DATIVE("Dative");
static const TStringBuf PREPOSITIONAL("Prepositional");
static const TStringBuf PREPOSITION("Preposition");
static const TStringBuf DIRECTIONAL("Directional");
static const TStringBuf LOCATIVE("Locative");

// https://wiki.yandex-team.ru/libgeobase/http-export/
static const THashMap<ELanguage, TSet<TStringBuf>> LANGUAGES = {
    {LANG_RUS, {NOMINATIVE, GENITIVE, DATIVE, PREPOSITIONAL, PREPOSITION}},
    {LANG_UKR, {NOMINATIVE, GENITIVE, DATIVE, PREPOSITIONAL, PREPOSITION}},
    {LANG_BEL, {NOMINATIVE, GENITIVE, DATIVE, PREPOSITIONAL, PREPOSITION}},
    {LANG_KAZ, {NOMINATIVE, GENITIVE, DATIVE, PREPOSITIONAL}},
    {LANG_TUR, {NOMINATIVE, GENITIVE, PREPOSITIONAL, DIRECTIONAL, LOCATIVE}},
    {LANG_TAT, {NOMINATIVE, GENITIVE, DIRECTIONAL, LOCATIVE}},
    {LANG_ENG, {NOMINATIVE}}
};

TString NGeoDBBuilder::BuildURLForJSONExport(const TString& hostName) {
    const auto request = TString::Join("http://", hostName, "/?" ,"format=json&",
                                      "types=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15&fields=",
                                      JoinVectorIntoString(FIELDS, TStringBuf(",")));

    TSet<TString> names;
    for (const auto& language : LANGUAGES) {
        const TString key = to_title(TString(IsoNameByLanguage(language.first))) + TStringBuf("name");
        for (const auto& case_ : language.second) {
            names.insert(key + case_);
        }
    }

    return TString::Join(request, ",", JoinStrings(names.cbegin(), names.cend(), TStringBuf(",")));
}

NLastGetopt::TOpts NGeoDBBuilder::TBuilderConfig::GetLastGetopt() {
    auto parser = NLastGetopt::TOpts::Default();
    parser.AddLongOption("without-names")
        .Optional()
        .NoArgument()
        .SetFlag(&WithoutNames)
        .Help("Omit region names");
    parser.AddLongOption("without-location")
        .Optional()
        .NoArgument()
        .SetFlag(&WithoutLocation)
        .Help("Omit locations");
    parser.AddLongOption("without-span")
        .Optional()
        .NoArgument()
        .SetFlag(&WithoutSpan)
        .Help("Omit span");
    parser.AddLongOption("without-phone-codes")
        .Optional()
        .NoArgument()
        .SetFlag(&WithoutPhoneCode)
        .Help("Omit phone codes");
    parser.AddLongOption("without-time-zone")
        .Optional()
        .NoArgument()
        .SetFlag(&WithoutTimeZone)
        .Help("Omit time zone");
    parser.AddLongOption("without-short-name")
        .Optional()
        .NoArgument()
        .SetFlag(&WithoutShortName)
        .Help("Omit short name");
    parser.AddLongOption("without-population")
        .Optional()
        .NoArgument()
        .SetFlag(&WithoutPopulation)
        .Help("Omit population");
    parser.AddLongOption("without-ambiguous")
        .Optional()
        .NoArgument()
        .SetFlag(&WithoutAmbiguous)
        .Help("Omit ambiguation data");
    parser.AddLongOption("without-services")
        .Optional()
        .NoArgument()
        .SetFlag(&WithoutServices)
        .Help("Omit services data");
    parser.AddLongOption("without-native-name")
        .Optional()
        .NoArgument()
        .SetFlag(&WithoutNativeName)
        .Help("Omit native names");
    parser.AddLongOption("without-synonym-names")
        .Optional()
        .NoArgument()
        .SetFlag(&WithoutSynonymNames)
        .Help("Omit synonym names");
    return parser;
}

static bool TryFillPhoneCode(const NJson::TJsonValue& jvalue, NGeoDB::TRegionProto& pregion) {
    if (!jvalue.Has(TStringBuf("PhoneCode"))) {
        return false;
    }

    const auto codes = jvalue[TStringBuf("PhoneCode")].GetString();
    if (!codes) {
        return false;
    }

    for (const auto& it : StringSplitter(codes).Split(' ').SkipEmpty()) {
        const auto& code = it.Token();
        if (!IsNumber(code)) {
            ERROR_LOG << "phone code <" << code << ">"
                      << " for region " << pregion.GetId() << " contains non-numbers" << Endl;
        }

        pregion.AddPhoneCode(ToString(code));
    }

    return true;
}

static bool TryFillServices(const NJson::TJsonValue& jvalue, NGeoDB::TRegionProto& pregion) {
    auto* const services = pregion.MutableServices();

    const auto boolFromJson = [&jvalue](const TStringBuf name) {
        bool val = false;
        TryFromString<bool>(jvalue[name].GetString(), val);
        return val;
    };

    if (boolFromJson(TStringBuf("Tv"))) {
        services->SetHasTv(true);
    }

    if (boolFromJson(TStringBuf("Maps"))) {
        services->SetHasMaps(true);
    }

    if (boolFromJson(TStringBuf("Weather"))) {
        services->SetHasWeather(true);
    }

    if (boolFromJson(TStringBuf("Subway"))) {
        services->SetHasSubway(true);
    }

    if (boolFromJson(TStringBuf("Etrain"))) {
        services->SetHasETrain(true);
    }

    if (boolFromJson(TStringBuf("Afisha"))) {
        services->SetHasAfisha(true);
    }

    if (boolFromJson(TStringBuf("Ad"))) {
        services->SetHasAdv(true);
    }

    return true;
}

static bool TryFillLocation(const NJson::TJsonValue& jvalue, NGeoDB::TRegionProto& pregion) {
    if (!jvalue.Has(TStringBuf("Lon")) || !jvalue.Has(TStringBuf("Lat"))) {
        return false;
    }

    auto* const location = pregion.MutableLocation();
    if (const auto lon = FromString<double>(jvalue[TStringBuf("Lon")].GetString())) {
        location->SetLon(lon);
    }

    if (const auto lat = FromString<double>(jvalue[TStringBuf("Lat")].GetString())) {
        location->SetLat(lat);
    }

    return true;
}

static bool TryFillSpan(const NJson::TJsonValue& jvalue, NGeoDB::TRegionProto& pregion) {
    if (!jvalue.Has(TStringBuf("SpnLon")) || !jvalue.Has(TStringBuf("SpnLat"))) {
        return false;
    }

    auto* const span = pregion.MutableSpan();
    if (const auto height = FromString<double>(jvalue[TStringBuf("SpnLon")].GetString())) {
        span->SetHeight(height);
    }

    if (const auto width = FromString<double>(jvalue[TStringBuf("SpnLat")].GetString())) {
        span->SetWidth(width);
    }

    return true;
}

static bool TryFillCountryAndCapital(const TVector<TCateg>& path,
                                     const THashMap<TCateg, NGeoDB::EType>& type,
                                     const THashMap<TCateg, TCateg>& chiefRegion,
                                     NGeoDB::TRegionProto& pregion) {
    if (NGeoDB::COUNTRY == type.at(pregion.GetId())) {
        pregion.SetCountryId(pregion.GetId());
        pregion.SetCountryCapitalId(chiefRegion.at(pregion.GetId()));
        return true;
    } else {
        for (const auto id : path) {
            if (NGeoDB::COUNTRY != type.at(id)) {
                continue;
            }

            pregion.SetCountryId(id);
            pregion.SetCountryCapitalId(chiefRegion.at(id));
            // assuming that there is only one country ID can belong to
            return true;
        }
    }

    return false;
}

static bool TryFillNames(const NJson::TJsonValue& jvalue, NGeoDB::TRegionProto& pregion) {
    for (const auto& language : LANGUAGES) {
        const auto key = to_title(TString(IsoNameByLanguage(language.first))) + TStringBuf("name");
        const auto nominative = jvalue[key + NOMINATIVE].GetString();
        const auto genitive = jvalue[key + GENITIVE].GetString();
        const auto dative = jvalue[key + DATIVE].GetString();
        const auto prepositional = jvalue[key + PREPOSITIONAL].GetString();
        const auto preposition = jvalue[key + PREPOSITION].GetString();
        const auto locative = jvalue[key + LOCATIVE].GetString();
        const auto directional = jvalue[key + DIRECTIONAL].GetString();

        const auto hasAtLeastOneName = nominative || genitive || dative || prepositional
                                       || preposition || locative || directional;
        if (!hasAtLeastOneName) {
            continue;
        }

        auto* const names = pregion.AddNames();

        names->SetLang(language.first);

        if (language.second.contains(NOMINATIVE) && nominative) {
            names->SetNominative(nominative);
        }
        if (language.second.contains(GENITIVE) && genitive) {
            names->SetGenitive(genitive);
        }
        if (language.second.contains(DATIVE) && dative) {
            names->SetDative(dative);
        }
        if (language.second.contains(PREPOSITIONAL) && prepositional) {
            names->SetPrepositional(prepositional);
        }
        if (language.second.contains(PREPOSITION) && preposition) {
            names->SetPreposition(preposition);
        }
        if (language.second.contains(LOCATIVE) && locative) {
            names->SetLocative(locative);
        }
        if (language.second.contains(DIRECTIONAL) && directional) {
            names->SetDirectional(directional);
        }
    }

    return true;
}

static bool TryFillNativeName(const NJson::TJsonValue& jvalue, NGeoDB::TRegionProto& pregion) {
    if (const auto nativeName = jvalue[TStringBuf("native_name")].GetString()) {
        pregion.SetNativeName(nativeName);
    }
    return true;
}

static bool TryFillSynonymNames(const NJson::TJsonValue& jvalue, NGeoDB::TRegionProto& pregion) {
    const auto line = jvalue[TStringBuf("synonyms")].GetString();
    for (const auto& token : StringSplitter(line).Split(',')) {
        const auto name = Strip(TString{token.Token()});
        if (name) {
            pregion.AddSynonymNames(name);
        }
    }

    return true;
}

static const NGeoDB::TRegionProto_TNames* FindNamePtr(const NGeoDB::TRegionProto& region,
                                                      const ELanguage& language) {
    return FindIfPtr(region.GetNames(),
                     [language](const NGeoDB::TRegionProto_TNames& x){
                         return x.GetLang() == language;
                     });
}

namespace {
    class TRWGeoKeeper: public NGeoDB::TGeoKeeper {
    public:
        void InsertRegion(NGeoDB::TRegionProto& region) {
            if (region.GetType() > NGeoDB::WORLD) {
                if (!region.NamesSize()) {
                    ERROR_LOG << "No names for region: " << region.GetId() << ", skip ambiguation for it" << Endl;
                } else if (const auto* const ptr = FindNamePtr(region, LANG_RUS)) {
                    const auto name = ToLowerUTF8(ptr->GetNominative());
                    if (name) {
                        NameToRegions_[name].insert(region.GetId());
                    }
                }
            }

            NGeoDB::TGeoKeeper::InsertRegion(region);
        }

        void UpdateAmbiguous() {
            for (auto& nameAndRegions : NameToRegions_) {
                if (nameAndRegions.second.size() < 2) {
                    continue;
                }

                for (auto& mutRegion : nameAndRegions.second) {
                    for (const auto& sameRegId : nameAndRegions.second) {
                        if (mutRegion != sameRegId) {
                            auto* const proto = Index_.FindPtr(mutRegion);
                            Y_ENSURE(proto, "Unable to find region: " << mutRegion);
                            proto->MutableAmbiguous()->Add(sameRegId);
                        }
                    }
                }

                ParentAmbiguous(nameAndRegions.second, nameAndRegions.first);
            }
        }

        void NormalizeRecords() {
            for (auto& entry : Index_) {
                Sort(*entry.second.MutableAmbiguous());
                Sort(*entry.second.MutableAmbiguousParents());
                SortBy(*entry.second.MutableNames(),
                       [](const NGeoDB::TRegionProto_TNames& x) {
                           return static_cast<int>(x.GetLang());
                       }
                );
                Sort(*entry.second.MutablePhoneCode());
                Sort(*entry.second.MutableSynonymNames());
            }
        }

        void RemoveUnnacessary(const NGeoDBBuilder::TBuilderConfig& config) {
            for (auto& entry : Index_) {
                if (config.WithoutNames) {
                    entry.second.ClearNames();
                }

                if (config.WithoutLocation) {
                    entry.second.ClearLocation();
                }

                if (config.WithoutSpan) {
                    entry.second.ClearSpan();
                }

                if (config.WithoutPhoneCode) {
                    entry.second.ClearPhoneCode();
                }

                if (config.WithoutTimeZone) {
                    entry.second.ClearTimeZone();
                    entry.second.ClearTimeZoneOffset();
                }

                if (config.WithoutShortName) {
                    entry.second.ClearShortName();
                }

                if (config.WithoutPopulation) {
                    entry.second.ClearPopulation();
                }

                if (config.WithoutAmbiguous) {
                    entry.second.ClearAmbiguous();
                    entry.second.ClearAmbiguousParents();
                }

                if (config.WithoutServices) {
                    entry.second.ClearServices();
                }

                if (config.WithoutNativeName) {
                    entry.second.ClearNativeName();
                }

                if (config.WithoutSynonymNames) {
                    entry.second.ClearSynonymNames();
                }
            }
        }

    private:
        void ParentAmbiguous(const THashSet<TCateg>& regions, const TString& id) {
            if (regions.size() < 2) {
                return;
            }

            TVector<TCateg> commonPath;
            for (size_t i = 0; ; ++i) {
                TCateg geo = END_CATEG;
                for (const auto id : regions) {
                    NGeoDB::TRegionProto* region = Index_.FindPtr(id);
                    Y_ENSURE(region, TStringBuf("Unable to find proto: ") << id);
                    Y_ENSURE(region->PathSize() >= i, TStringBuf("Caught integer overflow"));
                    const size_t idx = region->PathSize() - i;
                    if (idx == 0 || (geo != END_CATEG && region->GetPath(idx - 1) != geo)) {
                        geo = END_CATEG;
                        break;
                    }

                    if (geo == END_CATEG) {
                        geo = region->GetPath(idx - 1);
                    }
                }

                if (geo == END_CATEG) {
                    break;
                }

                commonPath.push_back(geo);
            }

            Y_ENSURE(commonPath, "common path must not be empty");

            NGeoDB::EType commonType = NGeoDB::EType::UNKNOWN;
            {
                const NGeoDB::TGeoPtr r = Find(commonPath.back());
                Y_ENSURE(r, "the last region in common path must be valid: " << commonPath.back());
                commonType = r->GetType();
            }

            if (NGeoDB::EType::UNKNOWN == commonType) {
                return;
            }

            const NGeoDB::TEntityTypeWeight commonTypeWeight = NGeoDB::EntityTypeToWeight(commonType);

            // black magic by ezhi@ from perl report YxWeb::Util::Region
            if (commonTypeWeight >= NGeoDB::EntityTypeToWeight(NGeoDB::EType::CONSTITUENT_ENTITY)) {
                commonType = NGeoDB::EType::CONSTITUENT_ENTITY_AREA;
            } else if (commonTypeWeight >= NGeoDB::EntityTypeToWeight(NGeoDB::EType::COUNTRY)) {
                commonType = NGeoDB::EType::CONSTITUENT_ENTITY;
            } else {
                commonType = NGeoDB::EType::COUNTRY;
            }

            THashMap<TCateg, THashSet<TCateg>> byParent;
            for (const auto id : regions) {
                const NGeoDB::TGeoPtr parentReg = Find(id).ParentByType(commonType);
                if (!parentReg) {
                    continue;
                }

                byParent[parentReg->GetId()].insert(id);
                auto* const proto = Index_.FindPtr(id);
                proto->MutableAmbiguousParents()->Add(parentReg->GetId());
            }

            for (const auto& parentToRegions : byParent) {
                if (parentToRegions.second.size() == regions.size()) {
                    break;
                }

                ParentAmbiguous(parentToRegions.second, id);
            }
        }

    private:
        THashMap<TString, THashSet<TCateg>> NameToRegions_;
    };
}  // namespace

static bool TryFillTimezones(const NJson::TJsonValue& jvalue, NGeoDB::TRegionProto& pregion) {
    auto timeZonePresent = false;
    if (const auto tzName = jvalue[TStringBuf("TzName")].GetString()) {
        pregion.SetTimeZone(tzName);
        timeZonePresent = true;
    }

    auto timeZoneOffsetPreset = false;
    if (const auto tzOffsetString = jvalue[TStringBuf("TzOffset")].GetString()) {
        const int tzOffset = FromString<int>(tzOffsetString);
        if (0 != tzOffset) {
            pregion.SetTimeZoneOffset(tzOffset);
        }

        timeZoneOffsetPreset = true;
    }

    return timeZonePresent && timeZoneOffsetPreset;
}

static void JsonToProtobuf(const NJson::TJsonValue& jvalue, const TVector<TCateg>& path,
                           const THashMap<TCateg, NGeoDB::EType>& type,
                           const THashMap<TCateg, TCateg>& chiefRegion,
                           const THashSet<TCateg>* const /*children*/,
                           NGeoDB::TRegionProto& pregion) {
    const TCateg region = FromString<TCateg>(jvalue[TStringBuf("Id")].GetString());
    pregion.SetId(region);
    if (FromString<bool>(jvalue[TStringBuf("Main")].GetString())) {
        pregion.SetIsMain(true);
    }

    pregion.SetType(type.at(region));

    TryFillTimezones(jvalue, pregion);
    if (const auto shortName = jvalue[TStringBuf("ShortName")].GetString()) {
        pregion.SetShortName(shortName);
    }

    if (const auto populationStr = jvalue[TStringBuf("Population")].GetString()) {
        const auto population = FromString<ui64>(populationStr);
        if (population > 0) {
            pregion.SetPopulation(population);
        }
    }

    const TCateg chiefId = FromString<TCateg>(jvalue[TStringBuf("chief_region")].GetString());
    if (chiefId > 0) {
        pregion.SetChiefId(chiefId);
    }

    TryFillLocation(jvalue, pregion);
    TryFillSpan(jvalue, pregion);
    TryFillPhoneCode(jvalue, pregion);
    TryFillServices(jvalue, pregion);
    TryFillCountryAndCapital(path, type, chiefRegion, pregion);
    TryFillNames(jvalue, pregion);
    TryFillNativeName(jvalue, pregion);
    TryFillSynonymNames(jvalue, pregion);

    for (const auto id : path) {
        pregion.AddPath(id);
    }

    if (path) {
        pregion.SetParentId(path.front());
    }

    /*
    if (children) {
        for (const TCateg id: *children)
            pregion.AddChildren(id);
    }
    */

    pregion.CheckInitialized();
}

static THashMap<TCateg, NJson::TJsonValue> ReadGeoBaseJSONExport(IInputStream& input) {
    NJson::TJsonValue dump;
    NJson::ReadJsonTree(&input, &dump, true);
    THashMap<TCateg, NJson::TJsonValue> result;
    result.reserve(dump.GetArray().size());
    for (const auto& jregion: dump.GetArray()) {
        result.insert({FromString<TCateg>(jregion[TStringBuf("Id")].GetString()), jregion});
    }

    return result;
}

static std::tuple<
    THashMap<TCateg, TVector<TCateg>>,
    THashMap<TCateg, NGeoDB::EType>,
    THashMap<TCateg, TCateg>>
GetPathTypeAndChiefRegion(const THashMap<TCateg, NJson::TJsonValue>& jregions) {
    THashMap<TCateg, TVector<TCateg>> path;
    THashMap<TCateg, NGeoDB::EType> type;
    THashMap<TCateg, TCateg> chiefRegion;
    path.reserve(jregions.size());
    type.reserve(jregions.size());
    chiefRegion.reserve(jregions.size());
    for (const auto& jr : jregions) {
        const auto regionType = FromString<int>(jr.second[TStringBuf("Type")].GetString());
        if (!NGeoDB::EType_IsValid(regionType)) {
            ythrow yexception{} << "Invalid type " << regionType << " of region " << jr.first;
        }

        type[jr.first] = static_cast<NGeoDB::EType>(regionType);
        chiefRegion[jr.first] = FromString<TCateg>(jr.second[TStringBuf("chief_region")].GetString());

        const auto pathStr = jr.second[TStringBuf("Path")].GetString();
        size_t index = 0;
        for (const auto& it : StringSplitter(pathStr).Split('/')) {
            ++index;
            if (Y_UNLIKELY(1 == index)) {
                continue;
            }

            path[jr.first].push_back(FromString<TCateg>(it.Token()));
        }
    }
    return std::make_tuple(path, type, chiefRegion);
}

static THashMap<TCateg, THashSet<TCateg>> GetChildern(
    const THashMap<TCateg, TVector<TCateg>>& path
){
    THashMap<TCateg, THashSet<TCateg>> children;
    children.reserve(path.size());
    for (const auto& value : path) {
        if (value.second) {
            children[value.second.front()].insert(value.first);
        }
    }

    return children;
}

static bool IsBadRegion(const TCateg region, const TVector<TCateg>& path,
                     const THashMap<TCateg, NGeoDB::EType>& type) {
    return 29 != region && 10000 != region && (type.at(region) <= 0
        || AnyOf(path.begin(), path.end(), [&type](const TCateg id) {
               return type.at(id) <= 0 && id != 10000;
           }));
}

void NGeoDBBuilder::ParseGeobaseJSONExport(const TBuilderConfig& config,
                                           IInputStream& input, IOutputStream& output) {
    INFO_LOG << "Parsing geobase JSON dump" << Endl;
    const auto jregions = ReadGeoBaseJSONExport(input);

    THashMap<TCateg, TVector<TCateg>> path;
    THashMap<TCateg, NGeoDB::EType> type;
    THashMap<TCateg, TCateg> chiefRegion;
    std::tie(path, type, chiefRegion) =  GetPathTypeAndChiefRegion(jregions);
    const auto children = GetChildern(path);

    TRWGeoKeeper geodb;
    for (const auto& jr : jregions) {
        const auto region = jr.first;
        const auto& json = jr.second;
        if (IsBadRegion(region, path[region], type)) {
            continue;
        }

        auto pregion = NGeoDB::TRegionProto{};
        JsonToProtobuf(json, path[region], type, chiefRegion,
                       children.contains(region) ? &children.at(region) : nullptr,
                       pregion);

        geodb.InsertRegion(pregion);
    }

    geodb.UpdateAmbiguous();
    geodb.RemoveUnnacessary(config);
    geodb.NormalizeRecords();

    INFO_LOG << "Parsing successfull" << Endl;

    INFO_LOG << "Dumping geodb to output stream" << Endl;
    geodb.Save(&output);
    INFO_LOG << "Dumping successfull" << Endl;
}
