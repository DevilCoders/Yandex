#include "json_config.h"

#include "config_global.h"
#include "yql_rule_set.h"

#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/enum.h>
#include <antirobot/lib/json.h>

#include <util/generic/cast.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/string/join.h>
#include <util/system/fs.h>

namespace NAntiRobot {
    namespace {
        const size_t MAX_REQ_URL_SIZE = 1024;

        TVector<TCbbGroupId> CbbFlagFromJson(const NJson::TJsonValue& value) {
            if (value.IsInteger()) {
                return {SafeIntegerCast<TCbbGroupId>(value.GetUIntegerSafe())};
            }

            const auto& items = value.GetArraySafe();

            TVector<TCbbGroupId> ret;
            ret.reserve(items.size());

            for (const auto& item : items) {
                ret.push_back(SafeIntegerCast<TCbbGroupId>(item.GetUIntegerSafe()));
            }

            return ret;
        }

        template <typename TRegExp>
        void ParseRegExps(
            const NJson::TJsonValue& jsonEntries,
            TStringBuf key,
            TVector<TRegExp>* regExps
        ) {
            const auto& entries = jsonEntries.GetArraySafe();

            regExps->clear();
            regExps->reserve(entries.size());

            for (const auto& jsonEntry : entries) {
                const auto& entry = jsonEntry.GetMapSafe();
                if constexpr (std::is_same_v<TRegExp, TReqGroupRegExp>) {
                    if (!entry.contains("path")) {
                        // Get from header
                        regExps->emplace_back(
                            TString(FromString(entry.at(key).GetStringSafe()))
                        );
                        continue;
                    }
                }

                TMaybe<TString> host = entry.contains("host") ? TMaybe<TString>(entry.at("host").GetStringSafe())  : Nothing();
                const auto& path = entry.at("path").GetStringSafe();
                TMaybe<TString> query = entry.contains("query") ? TMaybe<TString>(entry.at("query").GetStringSafe())  : Nothing();

                regExps->emplace_back(
                    host,
                    path,
                    query,
                    FromString(entry.at(key).GetStringSafe())
                );
            }
        }

        TVector<TString> ExtractStrings(const NJson::TJsonValue& value) {
            const auto& items = value.GetArraySafe();

            TVector<TString> strings;
            strings.reserve(items.size());

            for (const auto& item : items) {
                strings.push_back(item.GetStringSafe());
            }

            return strings;
        }

        TVector<TGlobalJsonConfig::TDictionaryInfo> ExtractDictionariesMeta(const NJson::TJsonValue& metaJson, const NJson::TJsonValue& listJson) {
            static const TKeyChecker keyChecker(
                {
                    "name",
                    "type",
                    "key_type",
                },
                {
                    "data",
                    "proxy",
                    "path",
                    "test_data", // used for tests
                }
            );

            const auto& dictsMeta = metaJson.GetArraySafe();
            TVector<TString> needNames;
            for (const auto& elem : listJson.GetArraySafe()) {
                needNames.push_back(elem.GetStringSafe());
            }

            TVector<TGlobalJsonConfig::TDictionaryInfo> ret;
            ret.reserve(needNames.size());
            THashSet<TString> seenNames;

            for (const auto& dict : dictsMeta) {
                keyChecker.Check(dict);
                const auto& name = dict["name"].GetStringSafe();
                if (IsIn(needNames, name)) {
                    const auto type = FromString<TGlobalJsonConfig::EDictionaryType>(dict["key_type"].GetStringSafe());
                    ret.push_back({name, type});
                    const auto inserted = seenNames.insert(name).second;
                    Y_ENSURE(inserted, "Duplicate dictionary name: " << name);
                }
            }

            return ret;
        }

        TVector<TGlobalJsonConfig::TLastVisitsRuleInfo> ExtractLastVisitsRules(
            const NJson::TJsonValue& value
        ) {
            static const TKeyChecker ruleKeyChecker(
                {
                    "id",
                    "name",
                    "rule"
                },
                {
                    "comment",
                }
            );

            const auto& rules = value.GetArraySafe();

            TVector<TGlobalJsonConfig::TLastVisitsRuleInfo> ret;
            ret.reserve(rules.size());
            THashSet<TLastVisitsCookie::TRuleId> seenRuleIds;
            THashSet<TString> seenRuleNames;

            for (const auto& rule : rules) {
                ruleKeyChecker.Check(rule);

                const auto id = SafeIntegerCast<TLastVisitsCookie::TRuleId>(rule["id"].GetIntegerSafe());
                const auto& name = rule["name"].GetStringSafe();
                ret.push_back({id, name, rule["rule"].GetStringSafe()});

                const auto [_, insertedId] = seenRuleIds.insert(id);
                Y_ENSURE(insertedId, "Duplicate last visits rule id: " << id);

                const auto [_2, insertedName] = seenRuleNames.insert(name);
                Y_ENSURE(insertedName, "Duplicate last visits rule name: " << name);
            }

            return ret;
        }

        TVector<TGlobalJsonConfig::TRule> ExtractRules(const NJson::TJsonValue& value) {
            static const TKeyChecker ruleKeyChecker(
                {
                    "id",
                    "cbb",
                    "yql"
                },
                {
                    "author",
                    "comment",
                    "date",
                    "ticket"
                }
            );

            const auto& rules = value.GetArraySafe();

            TVector<TGlobalJsonConfig::TRule> ret;
            ret.reserve(rules.size());
            THashSet<TCbbRuleId> seenRuleIds;

            for (const auto& rule : rules) {
                ruleKeyChecker.Check(rule);

                const auto id = SafeIntegerCast<TCbbRuleId>(rule["id"].GetIntegerSafe());
                ret.push_back({
                    id,
                    ExtractStrings(rule["cbb"]),
                    ExtractStrings(rule["yql"])
                });

                Y_ENSURE(!ret.back().Yql.empty(), "Empty YQL rule array");
                const auto [_, inserted] = seenRuleIds.insert(id);
                Y_ENSURE(inserted, "Duplicate rule id: " << id);
            }

            return ret;
        }
    } // anonymous namespace

    NJson::TJsonValue TJsonConfig::GetProperty(const TString& jsonPropName, const TString& tld,
                                               const NJson::TJsonValue& jsonServiceValue)
    {
        if (tld && jsonServiceValue["zone"][tld].Has(jsonPropName)) {
            return jsonServiceValue["zone"][tld][jsonPropName];
        }
        return jsonServiceValue[jsonPropName];
    }

    void TJsonConfig::SetZoneParams(TJsonConfigZoneParams& zoneParams, EHostType service, const TString& tld, const NJson::TJsonValue& jsonServiceValue) {
        zoneParams.Enabled = GetProperty("enabled", tld, jsonServiceValue).GetBooleanSafe();
        zoneParams.BansEnabled = GetProperty("bans_enabled", tld, jsonServiceValue).GetBooleanSafe();
        zoneParams.BlocksEnabled = GetProperty("blocks_enabled", tld, jsonServiceValue).GetBooleanSafe();
        zoneParams.RandomFactorsProbability = GetProperty("random_factors_fraction", tld, jsonServiceValue).GetDoubleSafe();
        zoneParams.AdditionalFactorsProbability = GetProperty("additional_factors_fraction", tld, jsonServiceValue).GetDoubleSafe();
        zoneParams.RandomEventsProbability = GetProperty("random_events_fraction", tld, jsonServiceValue).GetDoubleSafe();
        zoneParams.PreviewIdentTypeEnabled = GetProperty("preview_ident_type_enabled", tld, jsonServiceValue).GetBooleanSafe(true);
        zoneParams.SuspiciousnessHeaderEnabled = GetProperty("suspiciousness_header_enabled", tld, jsonServiceValue).GetBooleanSafe(false);
        zoneParams.DecodeFuidEnabled = GetProperty("decode_fuid_enabled", tld, jsonServiceValue).GetBooleanSafe(false);

        const std::array<std::tuple<TString, bool, TString&>, 3> formulas{{
            {"formula",          true,  zoneParams.ProcessorFormulaFilePath},
            {"fallback_formula", false, zoneParams.ProcessorFallbackFormulaFilePath},
            {"cacher_formula",   true,  zoneParams.CacherFormulaFilePath},
        }};

        if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
            zoneParams.ProcessorFormulaFilePath = TFsPath(ANTIROBOT_DAEMON_CONFIG.FormulasDir) / "matrixnet.info";
            zoneParams.ProcessorFallbackFormulaFilePath = "";
            zoneParams.CacherFormulaFilePath = TFsPath(ANTIROBOT_DAEMON_CONFIG.FormulasDir) / "cacher_catboost_v3.info";
        } else {
            for (auto [fieldName, required, field] : formulas) {
                const auto fileName = GetProperty(fieldName, tld, jsonServiceValue).GetStringSafe("");
                if (!fileName) {
                    if (required) {
                        ythrow yexception() << "Formula file for " << service << "-" << fieldName << "-" << tld << " is empty";
                    } else {
                        continue;
                    }
                }
                auto formulaFullPath = TFsPath(ANTIROBOT_DAEMON_CONFIG.FormulasDir) / fileName;
                if (!formulaFullPath.Exists()) {
                    ythrow yexception() << "Formula file for " << service << "-" << fieldName << "-" << tld << " " << formulaFullPath << " could not be found";
                }
                field = formulaFullPath;
            }
        }

        zoneParams.ProcessorThreshold = static_cast<float>(GetProperty("threshold", tld, jsonServiceValue).GetDoubleSafe());
        zoneParams.CacherThreshold = static_cast<float>(GetProperty("cacher_threshold", tld, jsonServiceValue).GetDoubleSafe());
        
        zoneParams.AndroidVersionSupportsCaptcha = GetProperty("android_version_supports_captcha", tld, jsonServiceValue).GetStringSafe("");
        zoneParams.IosVersionSupportsCaptcha = GetProperty("ios_version_supports_captcha", tld, jsonServiceValue).GetStringSafe("");

        zoneParams.CacherRandomFactorsProbability = static_cast<float>(
                GetProperty("cacher_random_factors_probability", tld, jsonServiceValue)
                    .GetDoubleSafe(ANTIROBOT_DAEMON_CONFIG.CacherRandomFactorsProbability));
    }

    const TJsonConfigZoneParams& TJsonConfig::ParamsByTld(EHostType service, const TStringBuf& tld) const {
        auto it = ZoneParams[service].find(tld);
        if (it != ZoneParams[service].end())
            return it->second;
        return DefaultZoneParams[service];
    }

    void TJsonConfig::ReadRegExpValues(const NJson::TJsonValue& jsonRegExpValue) {
        ServiceRegExp.clear();
        ServiceRegExp.reserve(jsonRegExpValue.GetArraySafe().size());
        for (const auto& x : jsonRegExpValue.GetArraySafe()) {
            const auto& m = x.GetMapSafe();
            ServiceRegExp.emplace_back(m.at("path").GetStringSafe(),
                    m.contains("query") ? TMaybe<TString>(m.at("query").GetStringSafe()) : Nothing(),
                    FromString(m.at("host").GetStringSafe()));
        }
    }

    void TJsonConfig::ParseRequestRegExps(const NJson::TJsonValue& jsonEntries, TVector<TQueryRegexpHolder>* regExps) {
        ParseRegExps(jsonEntries, "req_type", regExps);
    }

    void TJsonConfig::ParseRequestGroupRegExps(const NJson::TJsonValue& jsonEntries, TVector<TReqGroupRegExp>* regExps) {
        ParseRegExps(jsonEntries, "req_group", regExps);
    }

    void TJsonConfig::ReadConfigValues(const NJson::TJsonValue& jsonValue) {
        static const TKeyChecker rootKeyChecker(
            {
                "service", "re_queries", "enabled", "bans_enabled", "blocks_enabled",
                "formula", "threshold",
                "cbb_ip_flag", "cbb_re_flag", "cbb_re_mark_flag", "cbb_re_mark_log_only_flag",
                "cbb_re_user_mark_flag",
                "cbb_captcha_re_flag", "cbb_checkbox_blacklist_flag", "cbb_farmable_ban_flag",
                "cbb_can_show_captcha_flag", "re_groups",
                "random_events_fraction", "additional_factors_fraction", "random_factors_fraction",
                "cgi_secrets", "cacher_formula", "cacher_threshold",
            },
            {
                "inherit_bans",
                "preview_ident_type_enabled",
                "suspiciousness_header_enabled",
                "whitelists",
                "yandexips",
                "zone",
                "processor_threshold_for_spravka_penalty",
                "decode_fuid_enabled",
                "fallback_formula",
                "block_code",
                "min_requests_with_spravka",
                "max_req_url_size",
                "android_version_supports_captcha",
                "ios_version_supports_captcha",
                "cacher_random_factors_probability",
            }
        );

        std::array<bool, HOST_NUMTYPES> alreadyReadServices{};

        for (auto jsonServiceValue : jsonValue.GetArraySafe()) {
            rootKeyChecker.Check(jsonServiceValue);

            EHostType service;

            Y_ENSURE(
                TryFromString(jsonServiceValue["service"].GetStringSafe(), service),
                "Service name " << jsonServiceValue["service"] << " is not defined"
            );

            Y_ENSURE(
                !alreadyReadServices[service],
                "Service " << jsonServiceValue["service"] << " mentioned more than once"
            );

            alreadyReadServices[service] = true;

            ParseRequestRegExps(jsonServiceValue["re_queries"], &Params[service].ReqTypeRegExps);
            ParseRequestGroupRegExps(jsonServiceValue["re_groups"], &Params[service].ReqGroupRegExps);

            SetZoneParams(DefaultZoneParams[service], service, "", jsonServiceValue);

            for (auto [dst, key] : {
                std::pair{&Params[service].CbbFlagsManual, "cbb_ip_flag"},
                {&Params[service].CbbFlagsDDosControl, "cbb_re_flag"},
                {&Params[service].CbbFlagsDDosControlMarkOnly, "cbb_re_mark_flag"},
                {&Params[service].CbbFlagsDDosControlMarkAndLogOnly, "cbb_re_mark_log_only_flag"},
                {&Params[service].CbbFlagsDDosControlUserMark, "cbb_re_user_mark_flag"},
                {&Params[service].CbbFlagsCaptchaByRegexp, "cbb_captcha_re_flag"},
                {&Params[service].CbbFlagsFarmableIdentificationsBan, "cbb_farmable_ban_flag"},
                {&Params[service].CbbFlagsCheckboxBlacklist, "cbb_checkbox_blacklist_flag"},
                {&Params[service].CbbFlagsCanShowCaptcha, "cbb_can_show_captcha_flag"}
            }) {
                *dst = CbbFlagFromJson(jsonServiceValue[key]);
            }

            Params[service].ProcessorThresholdForSpravkaPenalty = jsonServiceValue["processor_threshold_for_spravka_penalty"].GetDoubleSafe(ANTIROBOT_DAEMON_CONFIG.ProcessorThresholdForSpravkaPenalty);
            Params[service].MinRequestsWithSpravka = jsonServiceValue["min_requests_with_spravka"].GetIntegerSafe(ANTIROBOT_DAEMON_CONFIG.MinRequestsWithSpravka);
            Params[service].MaxReqUrlSize = jsonServiceValue["max_req_url_size"].GetIntegerSafe(MAX_REQ_URL_SIZE);
            Params[service].BlockCode = static_cast<HttpCodes>(jsonServiceValue["block_code"].GetIntegerSafe(HTTP_FORBIDDEN));

            const auto cgiSecrets = jsonServiceValue["cgi_secrets"].GetArraySafe();

            for (const auto& cgiSecret : cgiSecrets) {
                Params[service].CgiSecrets.insert(cgiSecret.GetStringSafe());
            }

            if (jsonServiceValue.Has("inherit_bans")) {
                for (const auto& parentStr : jsonServiceValue["inherit_bans"].GetArraySafe()) {
                    const auto parent = FromString<EHostType>(parentStr.GetStringSafe());
                    Params[service].InheritBans.push_back(parent);
                }
            }

            if (jsonServiceValue.Has("whitelists")) {
                Params[service].Whitelists = ExtractStrings(jsonServiceValue["whitelists"]);
            } else if (!ANTIROBOT_DAEMON_CONFIG.WhiteList.empty()) {
                Params[service].Whitelists = {ANTIROBOT_DAEMON_CONFIG.WhiteList};
                TString serviceFileName = ANTIROBOT_DAEMON_CONFIG.WhiteList + "_" + ToString(service);
                if (NFs::Exists(ANTIROBOT_DAEMON_CONFIG.WhiteListsDir + "/" + serviceFileName)) {
                    Params[service].Whitelists.push_back(serviceFileName);
                }
            }

            if (jsonServiceValue.Has("yandexips")) {
                Params[service].YandexIps = ExtractStrings(jsonServiceValue["yandexips"]);
            } else if (!ANTIROBOT_DAEMON_CONFIG.YandexIpsFile.empty()) {
                Params[service].YandexIps = {ANTIROBOT_DAEMON_CONFIG.YandexIpsFile};
                TString serviceFileName = ANTIROBOT_DAEMON_CONFIG.YandexIpsFile + "_" + ToString(service);
                if (NFs::Exists(ANTIROBOT_DAEMON_CONFIG.YandexIpsDir + "/" + serviceFileName)) {
                    Params[service].YandexIps.push_back(serviceFileName);
                }
            }

            if (jsonServiceValue.Has("zone")) {
                for (const auto& [tld, _] : jsonServiceValue["zone"].GetMapSafe()) {
                    SetZoneParams(ZoneParams[service][tld], service, tld, jsonServiceValue);
                }
            }
        }
    }

    void TJsonConfig::SetParams(const NJson::TJsonValue& jsonValue, const NJson::TJsonValue& jsonRegExpValue, const TString& filePath, const TString& regExpFilePath) {
        try {
            ReadRegExpValues(jsonRegExpValue);
        } catch (NJson::TJsonException&) {
            ythrow yexception() << "Incorrect file " << regExpFilePath << CurrentExceptionMessage();
        }
        try {
            ReadConfigValues(jsonValue);
        } catch (NJson::TJsonException&) {
            ythrow yexception() << "Incorrect file " << filePath << CurrentExceptionMessage();
        }
    }

    void TJsonConfig::ParseConfig(const TString& str, const TString& regExpStr, const TString& filePath, const TString& regExpFilePath) {
        NJson::TJsonValue jsonValue;
        NJson::TJsonValue jsonRegExpValue;
        try {
            ReadJsonTree(str, &jsonValue, true);
        } catch (NJson::TJsonException&) {
            ythrow yexception() << "Incorrect file " << filePath << CurrentExceptionMessage();
        }
        try {
            ReadJsonTree(regExpStr, &jsonRegExpValue, true);
        } catch (NJson::TJsonException&) {
            ythrow yexception() << "Incorrect file " << regExpFilePath << CurrentExceptionMessage();
        }
        SetParams(jsonValue, jsonRegExpValue, filePath, regExpFilePath);
    }

    void TJsonConfig::LoadFromString(const TString& str, const TString& regExpStr) {
        ParseConfig(str, regExpStr, "json config strings", "json regExp strings");
    }

    void TJsonConfig::LoadFromFile(const TString& filePath, const TString& regExpFilePath) {
        Y_ENSURE(!filePath.empty(), "JsonConfFilePath config value cannot be empty");
        Y_ENSURE(!regExpFilePath.empty(), "JsonServiceRegExpFilePath config value cannot be empty");
        TFileInput confFile(filePath);
        TFileInput regExpFile(regExpFilePath);
        ParseConfig(confFile.ReadAll(), regExpFile.ReadAll(), filePath, regExpFilePath);
    }

    std::array<TVector<TQueryRegexpHolder>, HOST_NUMTYPES> TJsonConfig::GetReqTypeRegExps() const {
        std::array<TVector<TQueryRegexpHolder>, HOST_NUMTYPES> array;
        for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
            array[i] = Params[i].ReqTypeRegExps;
        }
        return array;
    }

    std::array<TVector<TReqGroupRegExp>, HOST_NUMTYPES> TJsonConfig::GetReqGroupRegExps() const {
        std::array<TVector<TReqGroupRegExp>, HOST_NUMTYPES> array;
        for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
            array[i] = Params[i].ReqGroupRegExps;
        }
        return array;
    }

    std::array<TString, HOST_NUMTYPES> TJsonConfig::GetFormulaFilesArrayByTld(TStringBuf tld) const {
        std::array<TString, HOST_NUMTYPES> formulaFiles;
        for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
            EHostType service = static_cast<EHostType>(i);
            formulaFiles[service] = ParamsByTld(service, tld).ProcessorFormulaFilePath;
            Y_ENSURE(!formulaFiles[service].empty(), "Empty matrixnet formula filename for service " << service << " tld = " << tld);
        }
        return formulaFiles;
    }

    std::array<TString, HOST_NUMTYPES> TJsonConfig::GetFallbackFormulaFilesArrayByTld(TStringBuf tld) const {
        std::array<TString, HOST_NUMTYPES> formulaFiles;
        for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
            EHostType service = static_cast<EHostType>(i);
            formulaFiles[service] = ParamsByTld(service, tld).ProcessorFallbackFormulaFilePath;
        }
        return formulaFiles;
    }

    std::array<TString, HOST_NUMTYPES> TJsonConfig::GetCacherFormulaFilesArrayByTld(TStringBuf tld) const {
        std::array<TString, HOST_NUMTYPES> formulaFiles;
        for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
            const auto service = static_cast<EHostType>(i);
            formulaFiles[service] = ParamsByTld(service, tld).CacherFormulaFilePath;
            Y_ENSURE(!formulaFiles[service].empty(), "Empty cacher catboost formula filename for service " << service << " tld = " << tld);
        }
        return formulaFiles;
    }

    TJsonConfigParams& TJsonConfig::operator[](size_t service) {
        return Params[service];
    }

    const TJsonConfigParams& TJsonConfig::operator[](size_t service) const {
        return Params[service];
    }

    void TJsonConfig::Dump(IOutputStream& out, EHostType service) const {
        for (auto [key, src] : std::initializer_list<std::pair<TStringBuf, const TVector<TCbbGroupId>*>>{
            {"CbbFlagsManual"_sb, &Params[service].CbbFlagsManual},
            {"CbbFlagsDDosControl"_sb, &Params[service].CbbFlagsDDosControl},
            {"CbbFlagsDDosControlMarkOnly"_sb, &Params[service].CbbFlagsDDosControlMarkOnly},
            {"CbbFlagsDDosControlMarkAndLogOnly"_sb, &Params[service].CbbFlagsDDosControlMarkAndLogOnly},
            {"CbbFlagsDDosControlUserMark"_sb, &Params[service].CbbFlagsDDosControlUserMark},
            {"CbbFlagsCaptchaByRegexp"_sb, &Params[service].CbbFlagsCaptchaByRegexp},
            {"CbbFlagsFarmableIdentificationsBan"_sb, &Params[service].CbbFlagsFarmableIdentificationsBan}
        }) {
            out << key << " = " << JoinSeq(",", *src) << '\n';
        }

        out << "ProcessorThreshold = " << DefaultZoneParams[service].ProcessorThreshold << '\n';
        out << "ProcessorFormulaFilePath = " << DefaultZoneParams[service].ProcessorFormulaFilePath << '\n';
        out << "ProcessorFallbackFormulaFilePath = " << DefaultZoneParams[service].ProcessorFallbackFormulaFilePath << '\n';

        out << "CacherThreshold = " << DefaultZoneParams[service].CacherThreshold << '\n';
        out << "CacherFormulaFilePath = " << DefaultZoneParams[service].CacherFormulaFilePath << '\n';

        out << "CacherRandomFactorsProbability = " << DefaultZoneParams[service].CacherRandomFactorsProbability << '\n';
    }

    void TGlobalJsonConfig::LoadFromString(const TString& filePath, const TString& str) {
        static const TKeyChecker rootKeyChecker = {
            "rules",
            "mark_rules",
            "dictionaries_meta",
            "dictionaries",
            "last_visits",
        };

        NJson::TJsonValue json;

        try {
            ReadJsonTree(str, &json, true);
        } catch (NJson::TJsonException&) {
            ythrow yexception() << "Incorrect file: " << filePath << ": " << CurrentExceptionMessage();
        }

        rootKeyChecker.Check(json);

        Rules = ExtractRules(json["rules"]);
        MarkRules = ExtractRules(json["mark_rules"]);
        Dictionaries = ExtractDictionariesMeta(json["dictionaries_meta"], json["dictionaries"]);
        LastVisitsRules = ExtractLastVisitsRules(json["last_visits"]);
    }

    void TGlobalJsonConfig::LoadFromFile(const TString& filePath) {
        TFileInput fileInput(filePath);
        LoadFromString(filePath, fileInput.ReadAll());
    }

    std::array<float, EHostType::HOST_NUMTYPES> TExperimentsConfig::LoadExpFormulasProbability(const NJson::TJsonValue::TMapType& probabilityOnService) {
        float defaultValue = 0;
        if (auto* it = probabilityOnService.FindPtr("default"); it) {
            defaultValue = it->GetDouble();
        } else {
            ythrow yexception() << "Default value for experiment formulas probability is not defined";
        }

        std::array<float, EHostType::HOST_NUMTYPES> probabilities;
        std::fill(std::begin(probabilities), std::end(probabilities), defaultValue);

        for (auto [serviceAsString, probabilityAsJsonValue] : probabilityOnService) {
            auto probability = probabilityAsJsonValue.GetDouble();
            if (serviceAsString != "default") {
                auto service = FromString<EHostType>(serviceAsString);
                Y_ENSURE(0.0f <= probability && probability <= 1.0f, "Probability should be between 0.0 and 1.0");
                probabilities[service] = probability;
            }
        }

        return probabilities;
    }

    void TExperimentsConfig::ExtractProbability(const NJson::TJsonValue::TMapType& node, std::array<float, EHostType::HOST_NUMTYPES>& probability) {
        probability = LoadExpFormulasProbability(node.at("probability").GetMap());
    }

    void TExperimentsConfig::ExtractProcessorExperiments(const NJson::TJsonValue& json) {
        ui8 formulaID = 0;
        if (json.GetMap().count("processor_experiments")) {
            auto processorExperimentsNode = json.GetMap().at("processor_experiments").GetMap();
            for(const auto& node : processorExperimentsNode) {
                TString formula = node.first;
                float threshold;
                Y_ENSURE(node.second.GetMap().count("threshold"));
                threshold = node.second.GetMap().at("threshold").GetDouble();
                formula = ANTIROBOT_DAEMON_CONFIG.FormulasDir + '/' + formula;
                if (!NFs::Exists(formula)) {
                    ythrow yexception() << "Formula file " << formula.Quote() << " could not be found";
                }
                ANTIROBOT_DAEMON_CONFIG_MUTABLE.ProcessorExpDescriptions.push_back(std::make_pair(formula, threshold));
                ANTIROBOT_DAEMON_CONFIG_MUTABLE.ProcessorExpFormulas.push_back(formula);
                ANTIROBOT_DAEMON_CONFIG_MUTABLE.ProcessorExpThresholds.push_back(threshold);
                ExtractProbability(node.second.GetMap(), ANTIROBOT_DAEMON_CONFIG_MUTABLE.ProcessorExpFormulasProbability[formulaID]);
                ++formulaID;
            }
        }
    }

    void TExperimentsConfig::ExtractCacherExperiments(const NJson::TJsonValue& json) {
        ui8 formulaID = 0;
        if (json.GetMap().count("cacher_experiments")) {
            auto cacherExperimentsNode = json.GetMap().at("cacher_experiments").GetMap();
            for(const auto& node : cacherExperimentsNode) {
                TString formula = node.first;
                float threshold;
                Y_ENSURE(node.second.GetMap().count("threshold"));
                threshold = node.second.GetMap().at("threshold").GetDouble();
                formula = ANTIROBOT_DAEMON_CONFIG.FormulasDir + '/' + formula;
                if (!NFs::Exists(formula)) {
                    ythrow yexception() << "Formula file " << formula.Quote() << " could not be found";
                }
                ANTIROBOT_DAEMON_CONFIG_MUTABLE.CacherExpFormulas.push_back(formula);
                ANTIROBOT_DAEMON_CONFIG_MUTABLE.CacherExpThresholds.push_back(threshold);
                ANTIROBOT_DAEMON_CONFIG_MUTABLE.CacherExpDescriptions.push_back(std::make_pair(formula, threshold));
                ExtractProbability(node.second.GetMap(), ANTIROBOT_DAEMON_CONFIG_MUTABLE.CacherExpFormulasProbability[formulaID]);
                ++formulaID;
            }
        }
    }

    void TExperimentsConfig::LoadFromFile(const TString& filePath) {
        if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
            return;
        }

        if (!filePath.empty()) {
            TFileInput fileInput(filePath);
            LoadFromString(filePath, fileInput.ReadAll());
        }
    }

    void TExperimentsConfig::LoadFromString(const TString& filePath, const TString& str) {
        NJson::TJsonValue json;

        try {
            NJson::ReadJsonTree(str, &json, true);
        }  catch (NJson::TJsonException&) {
            ythrow yexception() << "Incorrect file: " << filePath << ": " << CurrentExceptionMessage();
        }

        ExtractProcessorExperiments(json);
        ExtractCacherExperiments(json);
    }
}
