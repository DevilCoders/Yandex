#pragma once

#include "req_types.h"

#include <antirobot/daemon_lib/config_global.h>
#include <antirobot/daemon_lib/fullreq_info.h>
#include <antirobot/daemon_lib/reloadable_data.h>
#include <antirobot/daemon_lib/request_features.h>
#include <antirobot/daemon_lib/robot_set.h>

#include <metrika/uatraits/include/uatraits/detector.hpp>

#include <library/cpp/json/writer/json.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/resource/resource.h>

#include <util/string/builder.h>
#include <util/string/join.h>

#include <array>

namespace NAntiRobot {
    static void SetupTestData() {
        TFsPath("./data").MkDir();
        TFileOutput("./data/keys").Write("102c46d700bed5c69ed20b7473886468");
        TFileOutput("./data/yasc_key").Write("0dcb4c03e4cf00894c35b9f0424de7bf17d6da680245c678683d3f27fd9cc809");
        TFsPath("./data/bad_user_agents.lst").Touch();
        TFsPath("./lkeys.txt").Touch();
        TFsPath("./special_ips").Touch();
        TFsPath("./data/trbosrvnets").Touch();
        TFsPath("./yandex_ips").Touch();
        TFsPath("./whitelist_ips").Touch();
        TFileOutput("./data/spravka_data_key.txt").Write("4a1faf3281028650e82996f37aada9d97ef7c7c4f3c71914a7cc07a1cbb02d00");
        TFileOutput("./data/market_jws_key").Write("5dKWv1WGF0/Ag4dakf7a2NSMUVB1SihoXGV53kcT6Y4=");
        TFileOutput("./data/narwhal_jws_key").Write("narwhal:HS256:b17825f26f4e0c67aa1680c8da940be3085a358591b3b68b9d75693b991f91d8");
        TFileOutput("./data/autoru_offer_salt.txt").Write("salt");
        TFileOutput("./data/autoru_tamper_salt").Write("smzyaWLpl704/SkWeRHzgVbsVeqZ6H9b1K7AtvqvtaQ=");
    }

    inline TRequestClassifier CreateClassifierForTests() {
        const TString confFilePath = ArcadiaSourceRoot() + "/antirobot/config/service_config.json";

        return TRequestClassifier::CreateFromJsonConfig(confFilePath);
    }

    inline TRequestGroupClassifier CreateRequestGroupClassifierForTests() {
        const TString confFilePath = ArcadiaSourceRoot() + "/antirobot/config/service_config.json";

        return TRequestGroupClassifier::CreateFromJsonConfig(confFilePath);
    }

    inline TAutoPtr<TRequestFeatures> MakeReqFeatures(const TString& request) {
        TTimeStatInfoVector emptyVector = {};
        TTimeStats fakeStats{emptyVector, ""};

        static TRequestClassifier classifier = CreateClassifierForTests();
        static TAutoPtr<TFullReqInfo> req;
        static TReloadableData reloadable;

        TStringInput si(request);
        THttpInput httpInput(&si);

        reloadable.RequestClassifier.Set(classifier);
        reloadable.GeoChecker.Set(TGeoChecker("geodata6-xurma.bin"));

        {
            THashDictionary dict{{CityHash64("771,49195-49196-52393-49199-49200-52392-49171-49172-156-157-47-53,65281-0-23-35-13-5-16-11-10,29-23-24,0"_sb), 0.3F}};
            reloadable.FraudJa3.Set(std::move(dict));
        }
        {
            TSubnetDictionary<24,64> dict{{"81.18.115.0", 0.4F}};
            reloadable.FraudSubnet.Set(std::move(dict));
        }
        {
            THashDictionary dict{{CityHash64("771,49195-49196-52393-49199-49200-52392-49171-49172-156-157-47-53,65281-0-23-35-13-5-16-11-10,29-23-24,0"_sb), 0.5F}};
            reloadable.AutoruJa3.Set(std::move(dict));
        }
        {
            TSubnetDictionary<24,32> dict{{"81.18.115.0", 0.6F}};
            reloadable.AutoruSubnet.Set(std::move(dict));
        }
        {
            TEntityDict<float> dict{{"Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0; Trident/4.0; GTB6.5; SLCC1; .NET CLR 2.0.50727; .NET CLR 3.0.30729; .NET CLR 3.5.30729; InfoPath.2)"_sb, 0.7F}};
            reloadable.AutoruUA.Set(std::move(dict));
        }
        {
            THashDictionary dict{{CityHash64("/film/1443803"_sb), 1.0F}};
            reloadable.KinopoiskFilmsHoneypots.Set(std::move(dict));
        }
        {
            THashDictionary dict{{CityHash64("/name/6261374"_sb), 1.0F}};
            reloadable.KinopoiskNamesHoneypots.Set(std::move(dict));
        }

        req = new TFullReqInfo(httpInput, "", "", reloadable, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());

        TRpsFilter fakeFilter(0, TDuration::Zero(), TDuration::Zero());

        const TString uaData = NResource::Find("browser.xml");
        const TString uaProfiles = NResource::Find("profiles.xml");
        const TString uaExtra = NResource::Find("extra.xml");
        uatraits::detector detector(uaData.c_str(), uaData.length(), uaProfiles.c_str(), uaProfiles.length(), uaExtra.c_str(), uaExtra.length());
        TAutoruOfferDetector autoruOfferDetector("salt");

        TRequestFeatures::TContext ctx = {
            req.Get(),
            nullptr,
            &reloadable,
            &detector,
            &autoruOfferDetector,
            fakeFilter
        };

        return new TRequestFeatures(ctx, fakeStats, fakeStats);
     }

    inline TCacherRequestFeatures MakeCacherReqFeatures(const TString& request) {
        TTimeStatInfoVector emptyVector = {};
        TTimeStats fakeStats{emptyVector, ""};

        static TRequestClassifier classifier = CreateClassifierForTests();
        static TAutoPtr<TFullReqInfo> req;
        static TReloadableData reloadable;

        const auto robots = MakeAtomicShared<TRobotSet>();

        TStringInput si(request);
        THttpInput httpInput(&si);

        reloadable.RequestClassifier.Set(classifier);
        reloadable.GeoChecker.Set(TGeoChecker("geodata6-xurma.bin"));

        {
            THashDictionary dict{{CityHash64("771,49195-49196-52393-49199-49200-52392-49171-49172-156-157-47-53,65281-0-23-35-13-5-16-11-10,29-23-24,0"_sb), 0.3F}};
            reloadable.FraudJa3.Set(std::move(dict));
        }
        {
            TSubnetDictionary<24,64> dict{{"81.18.115.0", 0.4F}};
            reloadable.FraudSubnet.Set(std::move(dict));
        }
        {
            THashDictionary dict{{CityHash64("771,49195-49196-52393-49199-49200-52392-49171-49172-156-157-47-53,65281-0-23-35-13-5-16-11-10,29-23-24,0"_sb), 0.5F}};
            reloadable.AutoruJa3.Set(std::move(dict));
        }
        {
            TSubnetDictionary<24,32> dict{{"81.18.115.0", 0.6F}};
            reloadable.AutoruSubnet.Set(std::move(dict));
        }
        {
            TEntityDict<float> dict{{"Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0; Trident/4.0; GTB6.5; SLCC1; .NET CLR 2.0.50727; .NET CLR 3.0.30729; .NET CLR 3.5.30729; InfoPath.2)"_sb, 0.7F}};
            reloadable.AutoruUA.Set(std::move(dict));
        }

        req = new TFullReqInfo(httpInput, "", "", reloadable, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());

        const TString uaData = NResource::Find("browser.xml");
        const TString uaProfiles = NResource::Find("profiles.xml");
        const TString uaExtra = NResource::Find("extra.xml");
        uatraits::detector detector(uaData.c_str(), uaData.length(), uaProfiles.c_str(), uaProfiles.length(), uaExtra.c_str(), uaExtra.length());

        TCacherRequestFeatures::TContext ctx = {
            req.Get(),
            &reloadable,
            &*robots,
            &detector
        };

        return TCacherRequestFeatures(ctx, fakeStats);
     }


    inline TString GetJsonServiceIdentifierStr() {
        const TString regExpFile = ArcadiaSourceRoot() + "/antirobot/config/service_identifier.json";
        TFileInput file(regExpFile);

        return file.ReadAll();
    }

    class TJsonConfigGenerator {
    public:
        TJsonConfigGenerator()
        {
            ReQueriesWithCgi.fill({});
            ReQueries.fill({});
            Enabled.fill(true);
            BansEnabled.fill(true);
            BlocksEnabled.fill(false);
            RandomFactorsFraction.fill(0);
            RandomEventsFraction.fill(0);
            AdditionalFactorsFraction.fill(0);
            Formula.fill("matrixnet.info");
            Threshold.fill(0.0);
            CacherFormula.fill("catboost.info");
            CacherThreshold.fill(0.0);
            CbbIpFlag.fill(162);
            CbbReFlag.fill(183);
            CbbReMarkFlag.fill(185);
            CbbReMarkLogOnlyFlag.fill(702);
            CbbCaptchaReFlag.fill(262);
            CbbFarmableBanFlag.fill(226);
            ZoneFormula = "matrixnet.info";
            ZoneCacherFormula = "catboost.info";
        }

        void SetReQueries(const std::array<std::vector<std::pair<TString, TString>>, EHostType::HOST_NUMTYPES>& value) {
            ReQueries = value;
        }

        void SetReQueriesWithCgi(const std::array<std::vector<std::pair<TString, TString>>, EHostType::HOST_NUMTYPES>& value) {
            ReQueriesWithCgi = value;
        }

        void SetBansEnabled(const std::array<bool, EHostType::HOST_NUMTYPES>& value) {
            BansEnabled = value;
        }

        void SetBlocksEnabled(const std::array<bool, EHostType::HOST_NUMTYPES>& value) {
            BlocksEnabled = value;
        }

        void SetFormula(const std::array<TString, EHostType::HOST_NUMTYPES>& value) {
            Formula = value;
        }

        void SetThreshold(const std::array<float, EHostType::HOST_NUMTYPES>& value) {
            Threshold = value;
        }

        void SetCacherFormula(const std::array<TString, EHostType::HOST_NUMTYPES>& value) {
            CacherFormula = value;
        }

        void SetCacherThreshold(const std::array<float, EHostType::HOST_NUMTYPES>& value) {
            CacherThreshold = value;
        }

        void SetCbbIpFlag(const std::array<int, EHostType::HOST_NUMTYPES>& value) {
            CbbIpFlag = value;
        }

        void SetCbbReFlag(const std::array<int, EHostType::HOST_NUMTYPES>& value) {
            CbbReFlag = value;
        }

        void SetZoneFormula(const TString& value) {
            ZoneFormula = value;
        }

        void AddZoneThreshold(EHostType service, const TString& zone, float value) {
            ZoneThreshold[{service, zone}] = value;
        }

        void SetZoneCacherFormula(const TString& value) {
            ZoneCacherFormula = value;
        }

        void AddZoneCacherThreshold(EHostType service, const TString& zone, float value) {
            ZoneCacherThreshold[{service, zone}] = value;
        }

        TString Create() {
            TString buf;
            TStringOutput so(buf);
            NJsonWriter::TBuf json(NJsonWriter::HEM_DONT_ESCAPE_HTML, &so);

            json.BeginList();
            for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
                EHostType service = static_cast<EHostType>(i);
                json.BeginObject();
                json.WriteKey("service").WriteString(ToString(service));

                json.WriteKey("re_queries").BeginList();
                for (const auto& x : ReQueries[service]) {
                    json.BeginObject();
                    json.WriteKey("path").WriteString(x.first);
                    json.WriteKey("req_type").WriteString(x.second);
                    json.EndObject();
                }
                json.EndList();

                json.WriteKey("re_groups").BeginList().EndList();

                json.WriteKey("enabled").WriteBool(Enabled[service]);
                json.WriteKey("bans_enabled").WriteBool(BansEnabled[service]);
                json.WriteKey("blocks_enabled").WriteBool(BlocksEnabled[service]);
                json.WriteKey("random_events_fraction").WriteFloat(RandomEventsFraction[service]);
                json.WriteKey("additional_factors_fraction").WriteFloat(AdditionalFactorsFraction[service]);
                json.WriteKey("random_factors_fraction").WriteFloat(RandomFactorsFraction[service]);
                json.WriteKey("formula").WriteString(Formula[service]);
                json.WriteKey("threshold").WriteFloat(Threshold[service]);
                json.WriteKey("cacher_formula").WriteString(CacherFormula[service]);
                json.WriteKey("cacher_threshold").WriteFloat(CacherThreshold[service]);
                json.WriteKey("cbb_ip_flag").WriteInt(CbbIpFlag[service]);
                json.WriteKey("cbb_re_flag").WriteInt(CbbReFlag[service]);
                json.WriteKey("cbb_re_mark_flag").WriteInt(CbbReMarkFlag[service]);
                json.WriteKey("cbb_re_mark_log_only_flag").WriteInt(CbbReMarkLogOnlyFlag[service]);
                json.WriteKey("cbb_re_user_mark_flag").BeginList().EndList();
                json.WriteKey("cbb_captcha_re_flag").WriteInt(CbbCaptchaReFlag[service]);
                json.WriteKey("cbb_checkbox_blacklist_flag").BeginList().EndList();
                json.WriteKey("cbb_can_show_captcha_flag").BeginList().EndList();
                json.WriteKey("cbb_farmable_ban_flag").WriteInt(CbbFarmableBanFlag[service]);
                json.WriteKey("cgi_secrets").BeginList().EndList();

                json.WriteKey("zone").BeginObject();
                for (const auto& zone : Zones) {
                    json.WriteKey(zone).BeginObject();
                    json.WriteKey("formula").WriteString(ZoneFormula);
                    if (auto it = ZoneThreshold.find({service, zone}); it != ZoneThreshold.end()) {
                        json.WriteKey("threshold").WriteFloat(it->second);
                    }

                    json.WriteKey("cacher_formula").WriteString(ZoneCacherFormula);
                    if (auto it = ZoneCacherThreshold.find({service, zone}); it != ZoneCacherThreshold.end()) {
                        json.WriteKey("cacher_threshold").WriteFloat(it->second);
                    }
                    json.EndObject();
                }
                json.EndObject();

                json.EndObject();
            }
            json.EndList();
            so << Endl;
            return buf;
        }

    private:
        std::array<std::vector<std::pair<TString, TString>>, EHostType::HOST_NUMTYPES> ReQueries;
        std::array<std::vector<std::pair<TString, TString>>, EHostType::HOST_NUMTYPES> ReQueriesWithCgi;
        std::array<bool, EHostType::HOST_NUMTYPES> Enabled;
        std::array<bool, EHostType::HOST_NUMTYPES> BansEnabled;
        std::array<bool, EHostType::HOST_NUMTYPES> BlocksEnabled;
        std::array<float, EHostType::HOST_NUMTYPES> RandomFactorsFraction;
        std::array<float, EHostType::HOST_NUMTYPES> RandomEventsFraction;
        std::array<float, EHostType::HOST_NUMTYPES> AdditionalFactorsFraction;
        std::array<TString, EHostType::HOST_NUMTYPES> Formula;
        std::array<float, EHostType::HOST_NUMTYPES> Threshold;
        std::array<TString, EHostType::HOST_NUMTYPES> CacherFormula;
        std::array<float, EHostType::HOST_NUMTYPES> CacherThreshold;
        std::array<int, EHostType::HOST_NUMTYPES> CbbIpFlag;
        std::array<int, EHostType::HOST_NUMTYPES> CbbReFlag;
        std::array<int, EHostType::HOST_NUMTYPES> CbbReMarkFlag;
        std::array<int, EHostType::HOST_NUMTYPES> CbbReMarkLogOnlyFlag;
        std::array<int, EHostType::HOST_NUMTYPES> CbbCaptchaReFlag;
        std::array<int, EHostType::HOST_NUMTYPES> CbbFarmableBanFlag;
        TString ZoneFormula;
        TString ZoneCacherFormula;
        TMap<std::pair<EHostType, TString>, float> ZoneThreshold;
        TMap<std::pair<EHostType, TString>, float> ZoneCacherThreshold;
        const std::array<TString, 6> Zones{"ru", "ua", "by", "kz", "tr", "com"};
    };

    class TTestAntirobotMediumBase : public TTestBase {
    public:
        void SetUp() override {
            SetupTestData();

            ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString("<Daemon>\n"
                                                           "AuthorizeByFuid = 1\n"
                                                           "AuthorizeByICookie = 1\n"
                                                           "AuthorizeByLCookie = 1\n"
                                                           "CaptchaApiHost = ::\n"
                                                           "CbbApiHost = ::\n"
                                                           "CbbEnabled = 0\n"
                                                           "UseTVMClient = 0\n"
                                                           "GeodataBinPath = ./geodata6-xurma.bin\n"
                                                           "FormulasDir = .\n"
                                                           "HypocrisyBundlePath =\n"
                                                           "JsonConfFilePath = " + ArcadiaSourceRoot() + "/antirobot/config/service_config.json\n"
                                                           "PartnerCaptchaType = 1\n"
                                                           "ThreadPoolParams = free_min=0; free_max=0; total_max=0; increase=1\n"
                                                           "WhiteList = ./whitelist_ips\n"
                                                           "</Daemon>\n"
                                                           "<Zone>\n"
                                                           "</Zone>\n"
                                                           "<WizardsRemote>\n"
                                                           "RemoteWizards :::8891\n"
                                                           "</WizardsRemote>\n");
            TJsonConfigGenerator jsonConf;
            ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), GetJsonServiceIdentifierStr());
        }
    };
}
