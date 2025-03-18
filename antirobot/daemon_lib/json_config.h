#pragma once

#include "antirobot_cookie.h"
#include "cbb_id.h"
#include "req_types.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/http/misc/httpcodes.h>

#include <util/generic/hash_set.h>
#include <util/string/vector.h>

#include <array>

namespace NAntiRobot {
    class TQueryRegexpHolder {
    public:
        TQueryRegexpHolder(TMaybe<TString> host, TString doc, TMaybe<TString> cgi, EReqType reqType)
            : Host(std::move(host))
            , Doc(std::move(doc))
            , Cgi(std::move(cgi))
            , ReqType(reqType)
        {
        }

    public:
        TMaybe<TString> Host;
        TString Doc;
        TMaybe<TString> Cgi;
        EReqType ReqType;
    };

    class TReqGroupRegExp {
    public:
        TReqGroupRegExp(const TString& group)
            : GetFromHeader(true)
            , Group(group)
        {
        }

        TReqGroupRegExp(TMaybe<TString> host, TString doc, TMaybe<TString> cgi, const TString& group)
            : Host(std::move(host))
            , Doc(std::move(doc))
            , Cgi(std::move(cgi))
            , Group(group)
        {
        }

    public:
        bool GetFromHeader = false;
        TMaybe<TString> Host;
        TString Doc;
        TMaybe<TString> Cgi;
        TString Group;
    };

    class TServiceRegExpHolder {
    public:
        TServiceRegExpHolder(TString doc, TMaybe<TString> cgi, EHostType host)
            : Doc(std::move(doc))
            , Cgi(std::move(cgi))
            , Host(host)
        {
        }

    public:
        TString Doc;
        TMaybe<TString> Cgi;
        EHostType Host;
    };

    class TJsonConfigZoneParams {
    public:
        bool Enabled;
        bool BansEnabled;
        bool BlocksEnabled;
        float AdditionalFactorsProbability;
        float RandomFactorsProbability;
        float RandomEventsProbability;
        bool PreviewIdentTypeEnabled;
        bool SuspiciousnessHeaderEnabled;
        TString ProcessorFormulaFilePath;
        TString ProcessorFallbackFormulaFilePath;
        float ProcessorThreshold;
        TString CacherFormulaFilePath;
        float CacherThreshold;
        bool DecodeFuidEnabled;
        float CacherRandomFactorsProbability;

        TString AndroidVersionSupportsCaptcha;
        TString IosVersionSupportsCaptcha;
    };

    class TJsonConfigParams {
    public:
        TVector<TQueryRegexpHolder> ReqTypeRegExps;
        TVector<TReqGroupRegExp> ReqGroupRegExps;
        TVector<TCbbGroupId> CbbFlagsManual;
        TVector<TCbbGroupId> CbbFlagsDDosControl;
        TVector<TCbbGroupId> CbbFlagsDDosControlMarkOnly;
        TVector<TCbbGroupId> CbbFlagsDDosControlMarkAndLogOnly;
        TVector<TCbbGroupId> CbbFlagsDDosControlUserMark;
        TVector<TCbbGroupId> CbbFlagsCaptchaByRegexp;
        TVector<TCbbGroupId> CbbFlagsFarmableIdentificationsBan;
        TVector<TCbbGroupId> CbbFlagsCheckboxBlacklist;
        TVector<TCbbGroupId> CbbFlagsCanShowCaptcha;
        THashSet<TString> CgiSecrets;
        TVector<TString> YqlRules;
        TVector<EHostType> InheritBans;
        TVector<TString> Whitelists;
        TVector<TString> YandexIps;
        float ProcessorThresholdForSpravkaPenalty;
        size_t MinRequestsWithSpravka;
        size_t MaxReqUrlSize;
        HttpCodes BlockCode;
    };

    template <class T>
    class TParamHolder {
    public:
        T& operator[](size_t service) {
            return Values[service];
        }

        const T& operator[](size_t service) const {
            return Values[service];
        }

    private:
        std::array<T, EHostType::HOST_NUMTYPES> Values;
    };

    class TJsonConfig {
    public:
        void LoadFromString(const TString& str, const TString& regExpStr);
        void LoadFromFile(const TString& filePath, const TString& regExpFilePath);
        const TJsonConfigZoneParams& ParamsByTld(EHostType service, const TStringBuf& tld) const;
        std::array<TVector<TQueryRegexpHolder>, HOST_NUMTYPES> GetReqTypeRegExps() const;
        std::array<TVector<TReqGroupRegExp>, HOST_NUMTYPES> GetReqGroupRegExps() const;
        std::array<TString, HOST_NUMTYPES> GetFormulaFilesArrayByTld(TStringBuf tld) const;
        std::array<TString, HOST_NUMTYPES> GetFallbackFormulaFilesArrayByTld(TStringBuf tld) const;
        std::array<TString, HOST_NUMTYPES> GetCacherFormulaFilesArrayByTld(TStringBuf tld) const;
        TJsonConfigParams& operator[](size_t service);
        const TJsonConfigParams& operator[](size_t service) const;
        void Dump(IOutputStream& out, EHostType service) const;

        static void ParseRequestRegExps(const NJson::TJsonValue& jsonEntries, TVector<TQueryRegexpHolder>* regExps);
        static void ParseRequestGroupRegExps(const NJson::TJsonValue& jsonEntries, TVector<TReqGroupRegExp>* regExps);

    public:
        TVector<TServiceRegExpHolder> ServiceRegExp;

    private:
        void ParseConfig(const TString& str, const TString& regExpStr, const TString& filePath, const TString& regExpFilePath);
        void SetParams(const NJson::TJsonValue& jsonValue, const NJson::TJsonValue& jsonRegExpValue, const TString& filePath, const TString& regExpFilePath);
        void SetZoneParams(TJsonConfigZoneParams& zoneParams, EHostType service, const TString& tld, const NJson::TJsonValue& jsonServiceValue);
        NJson::TJsonValue GetProperty(const TString& jsonPropName, const TString& tld, const NJson::TJsonValue& jsonServiceValue);
        void ReadRegExpValues(const NJson::TJsonValue& jsonRegExpValue);
        void ReadConfigValues(const NJson::TJsonValue& jsonValue);

    private:
        TParamHolder<TJsonConfigParams> Params;
        TParamHolder<TJsonConfigZoneParams> DefaultZoneParams;
        TParamHolder<THashMap<TString, TJsonConfigZoneParams>> ZoneParams;
    };

    struct TGlobalJsonConfig {
        struct TRule {
            TCbbRuleId Id{};
            TVector<TString> Cbb;
            TVector<TString> Yql;
        };

        enum class EDictionaryType : ui8 {
            String /* "string" */,
            CityHash64 /* "cityhash64" */,
            Subnet /* "subnet" */,
            Uid /* "uid" */,
            MiniGeobase /* "mini_geobase" */,
            JwsStats /* "jws_stats" */,
            MarketStats /* "market_stats" */,
        };

        struct TDictionaryInfo {
            TString Name;
            EDictionaryType Type{};
        };

        struct TLastVisitsRuleInfo {
            TLastVisitsCookie::TRuleId Id{};
            TString Name;
            TString Rule;
        };

        TVector<TRule> Rules;
        TVector<TRule> MarkRules;
        TVector<TDictionaryInfo> Dictionaries;
        TVector<TLastVisitsRuleInfo> LastVisitsRules;

        void LoadFromString(const TString& filePath, const TString& str);
        void LoadFromFile(const TString& filePath);
    };

    struct TExperimentsConfig {
        std::array<float, EHostType::HOST_NUMTYPES> LoadExpFormulasProbability(const NJson::TJsonValue::TMapType& probabilityOnService);
        void ExtractProbability(const NJson::TJsonValue::TMapType& node, std::array<float, EHostType::HOST_NUMTYPES>& probability);
        void ExtractProcessorExperiments(const NJson::TJsonValue& json);
        void ExtractCacherExperiments(const NJson::TJsonValue& json);
        void LoadFromString(const TString& filePath, const TString& str);
        void LoadFromFile(const TString& filePath);
    };
} // namespace NAntiRobot
