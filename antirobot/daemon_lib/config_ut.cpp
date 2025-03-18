#include <library/cpp/testing/unittest/registar.h>

#include "config.h"
#include "req_types.h"
#include "captcha_gen.h"

#include <antirobot/daemon_lib/ut/utils.h>

#include <util/folder/tempdir.h>

using namespace NAntiRobot;

const TString StubIPsForHosts = "CaptchaApiHost = ::\n"
                                "CbbApiHost = ::\n";

Y_UNIT_TEST_SUITE(TestConfig) {
    void CreateTempFormulaFile(const TFsPath& path) {
        TFileOutput tempFile(path);
    }

Y_UNIT_TEST(TestCustomMatrixnetTresholds) {
    auto tempDir = TTempDir("TestCustomMatrixnetTresholds");

    CreateTempFormulaFile(tempDir.Path() / "matrixnet.info");
    CreateTempFormulaFile(tempDir.Path() / "catboost.info");

    const TString configHeader = "<Daemon>\n"
                                 + StubIPsForHosts +
                                 "FormulasDir = TestCustomMatrixnetTresholds\n"
                                 "</Daemon>\n"
                                 "<Zone>\n";
    const TString configFooter = "</Zone>";

    TString regExpJsonFileStr = GetJsonServiceIdentifierStr();

    {
        TJsonConfigGenerator jsonConf;
        std::array<float, HOST_NUMTYPES> ThresholdValues;
        ThresholdValues.fill(99);
        jsonConf.SetThreshold(ThresholdValues);
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(configHeader + configFooter);
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), regExpJsonFileStr);

        UNIT_ASSERT_VALUES_EQUAL(ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(HOST_MAPS, "").ProcessorThreshold, 99);
    }

    {
        TJsonConfigGenerator jsonConf;
        std::array<float, HOST_NUMTYPES> ThresholdValues;
        ThresholdValues.fill(20);
        jsonConf.AddZoneThreshold(HOST_MAPS, "com", 99);
        jsonConf.SetThreshold(ThresholdValues);
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(configHeader + configFooter);
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), regExpJsonFileStr);

        UNIT_ASSERT_VALUES_EQUAL(ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(HOST_MAPS, "").ProcessorThreshold, 20);
        UNIT_ASSERT_VALUES_EQUAL(ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(HOST_MAPS, "com").ProcessorThreshold, 99);
    }

    {
        TJsonConfigGenerator jsonConf;
        std::array<float, HOST_NUMTYPES> ThresholdValues;
        ThresholdValues.fill(20);
        jsonConf.AddZoneThreshold(HOST_MAPS, "com", 99);
        jsonConf.SetThreshold(ThresholdValues);
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(configHeader + configFooter);
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), regExpJsonFileStr);

        UNIT_ASSERT_VALUES_EQUAL(ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(HOST_MAPS, "ru").ProcessorThreshold, 20);
        UNIT_ASSERT_VALUES_EQUAL(ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(HOST_MAPS, "com").ProcessorThreshold, 99);
    }
}

Y_UNIT_TEST(TestMatrixnetFormulaFiles) {
    auto tempDir = TTempDir("TestMatrixnetFormulaFiles");

    // LoadFromString will try to read these files
    CreateTempFormulaFile(tempDir.Path() / "matrixnet.info");
    CreateTempFormulaFile(tempDir.Path() / "catboost.info");
    CreateTempFormulaFile(tempDir.Path() / "matrixnetCustom.info");

    const TString configHeader = "<Daemon>\n"
                                 + StubIPsForHosts +
                                 "FormulasDir = TestMatrixnetFormulaFiles\n"
                                 "</Daemon>\n"
                                 "<Zone>\n";
    const TString configFooter = "</Zone>";

    TString regExpJsonFileStr = GetJsonServiceIdentifierStr();

    {
        TJsonConfigGenerator jsonConf;
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(configHeader +
                           "\n"
                           + configFooter);
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), regExpJsonFileStr);

        UNIT_ASSERT_STRINGS_EQUAL(ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(HOST_NEWS, "").ProcessorFormulaFilePath, "TestMatrixnetFormulaFiles/matrixnet.info");
    }

    {
        TJsonConfigGenerator jsonConf;
        jsonConf.SetZoneFormula("matrixnetCustom.info");
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(configHeader +
                           "\n"
                           "<ru></ru>\n"
                           + configFooter);
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), regExpJsonFileStr);

        UNIT_ASSERT_STRINGS_EQUAL(ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(HOST_NEWS, "").ProcessorFormulaFilePath, "TestMatrixnetFormulaFiles/matrixnet.info");
    }

    {
        TJsonConfigGenerator jsonConf;
        jsonConf.SetZoneFormula("matrixnetCustom.info");
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(configHeader +
                           "\n"
                           "<ru></ru>\n"
                           + configFooter);
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), regExpJsonFileStr);

        UNIT_ASSERT_STRINGS_EQUAL(ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(HOST_NEWS, "").ProcessorFormulaFilePath, "TestMatrixnetFormulaFiles/matrixnet.info");
    }
}

Y_UNIT_TEST(TestCaptchaTypesWithoutDefault) {
    TAntirobotDaemonConfig cfg;
    const TString configHeader = "<Daemon>\n"
                                 + StubIPsForHosts +
                                 "</Daemon>\n"
                                 "<Zone>\n";
    const TString configFooter = "</Zone>";

    auto config = configHeader +
                  "CaptchaTypes = kinopoisk=ocr;\n"
                  + configFooter;

    UNIT_ASSERT_EXCEPTION(cfg.LoadFromString(config), yexception);
}

Y_UNIT_TEST(TestCaptchaTypes) {
    auto tempDir = TTempDir("TestCaptchaTypes");
    CreateTempFormulaFile(tempDir.Path() / "matrixnet.info");

    const TString configHeader = "<Daemon>\n"
                                 + StubIPsForHosts +
                                 "FormulasDir = TestCaptchaTypes\n"
                                 "</Daemon>\n"
                                 "<Zone>\n"
                                 "MatrixnetFormulaFiles = default=matrixnet.info\n";
    const TString configFooter = "</Zone>";

    {
        TAntirobotDaemonConfig cfg;
        cfg.LoadFromString(configHeader +
                           "\n"
                           + configFooter);

        UNIT_ASSERT_STRINGS_EQUAL(cfg.ConfByTld("ru").CaptchaTypes.at("default").Choose(), "estd");
    }

    {
        TAntirobotDaemonConfig cfg;
        cfg.LoadFromString(configHeader +
                           "CaptchaTypes = default=TEST_TYPE;\n"
                           + configFooter);

        UNIT_ASSERT_STRINGS_EQUAL(cfg.ConfByTld("ru").CaptchaTypes.at("default").Choose(), "TEST_TYPE");
    }

    {
        TAntirobotDaemonConfig cfg;
        cfg.LoadFromString(configHeader +
                           "CaptchaTypes = default=TEST_TYPE;\n"
                           "<ru></ru>"
                           + configFooter);

        UNIT_ASSERT_STRINGS_EQUAL(cfg.ConfByTld("ru").CaptchaTypes.at("default").Choose(), "TEST_TYPE");
    }

    {
        TAntirobotDaemonConfig cfg;
        cfg.LoadFromString(configHeader +
                           "CaptchaTypes = default=TEST_TYPE; kinopoisk=TEST_KINO\n"
                           + configFooter);

        UNIT_ASSERT_STRINGS_EQUAL(cfg.ConfByTld("ru").CaptchaTypes.at("default").Choose(), "TEST_TYPE");
        UNIT_ASSERT_STRINGS_EQUAL(cfg.ConfByTld("ru").CaptchaTypes.at("kinopoisk").Choose(), "TEST_KINO");
    }

    {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(configHeader +
                           "CaptchaTypes = default=ocr; kinopoisk=TEST_KINO\n"
                           "<ru>"
                           "CaptchaTypes = default=TEST_TYPE;\n"
                           "</ru>\n"
                           + configFooter);

        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", false, EHostType::HOST_WEB, EClientType::CLIENT_GENERAL), "TEST_TYPE");
        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", false, EHostType::HOST_KINOPOISK, EClientType::CLIENT_GENERAL), "TEST_KINO");
    }

    {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(configHeader +
                           "CaptchaTypes = default=ocr; kinopoisk=TEST_KINO\n"
                           "<ru>"
                           "CaptchaTypes = default=ocr; autoru=TEST_AUTO\n"
                           "</ru>\n"
                           + configFooter);

        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", false, EHostType::HOST_AUTORU, EClientType::CLIENT_GENERAL), "TEST_AUTO");
        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", false, EHostType::HOST_KINOPOISK, EClientType::CLIENT_GENERAL), "TEST_KINO");
    }

    {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(configHeader +
                           "CaptchaTypes = default=DEFAULT\n"
                           "<ru>"
                           "CaptchaTypes = default=DEFAULT_RU; default:ajax=DEFAULT_RU_AJAX\n"
                           "</ru>\n"
                           + configFooter);

        for (auto partner : {false, true}) {
            UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ua", partner, EHostType::HOST_WEB, EClientType::CLIENT_GENERAL), "DEFAULT");
            UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", partner, EHostType::HOST_WEB, EClientType::CLIENT_GENERAL), "DEFAULT_RU");
            UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", partner, EHostType::HOST_WEB, EClientType::CLIENT_AJAX), "DEFAULT_RU_AJAX");
            UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", partner, EHostType::HOST_WEB, EClientType::CLIENT_XML_PARTNER), "DEFAULT_RU");
        }
    }

    {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(configHeader +
                           "CaptchaTypes = default=DEFAULT\n"
                           "<ru>"
                           "CaptchaTypes = default=DEFAULT_RU; default:ajax=DEFAULT_RU_AJAX; default:xml=XML_OTHER_RU; autoru=AUTORU_RU; non_branded_partners=PARTNER_RU\n"
                           "</ru>\n"
                           + configFooter);

        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ua", false, EHostType::HOST_OTHER, EClientType::CLIENT_XML_PARTNER), "DEFAULT");
        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", true, EHostType::HOST_OTHER, EClientType::CLIENT_XML_PARTNER), "PARTNER_RU");
        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", false, EHostType::HOST_WEB, EClientType::CLIENT_GENERAL), "DEFAULT_RU");
        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", false, EHostType::HOST_WEB, EClientType::CLIENT_AJAX), "DEFAULT_RU_AJAX");
        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", false, EHostType::HOST_AUTORU, EClientType::CLIENT_GENERAL), "AUTORU_RU");
        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", false, EHostType::HOST_AUTORU, EClientType::CLIENT_AJAX), "AUTORU_RU");
        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", false, EHostType::HOST_OTHER, EClientType::CLIENT_XML_PARTNER), "XML_OTHER_RU");
    }

    {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(configHeader +
                           "CaptchaTypes = default=DEFAULT\n"
                           "<ru>"
                           "CaptchaTypes = default=DEFAULT_RU; autoru:ajax=AUTORU_RU_AJAX\n"
                           "</ru>\n"
                           + configFooter);

        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", false, EHostType::HOST_AUTORU, EClientType::CLIENT_GENERAL), "DEFAULT_RU");
        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", false, EHostType::HOST_AUTORU, EClientType::CLIENT_AJAX), "AUTORU_RU_AJAX");
        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", false, EHostType::HOST_WEB, EClientType::CLIENT_GENERAL), "DEFAULT_RU");
        UNIT_ASSERT_STRINGS_EQUAL(GetCaptchaTypeByTld("ru", false, EHostType::HOST_WEB, EClientType::CLIENT_AJAX), "DEFAULT_RU");
    }

    {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(configHeader +
                           "CaptchaTypes = default=DEFAULT\n"
                           "<ru>"
                           "CaptchaTypes = default={0.5:DEFAULT_RU1,0.5:DEFAULT_RU2}; default:ajax=DEFAULT_RU_AJAX\n"
                           "</ru>\n"
                           + configFooter);

        const int numRequests = 200;
        THashMap<TString, int> generalCount, ajaxCount;
        for (int i = 0; i < numRequests; i++) {
            generalCount[GetCaptchaTypeByTld("ru", false, EHostType::HOST_WEB, EClientType::CLIENT_GENERAL)]++;
            ajaxCount[GetCaptchaTypeByTld("ru", false, EHostType::HOST_WEB, EClientType::CLIENT_AJAX)]++;
        }
        UNIT_ASSERT(ajaxCount["DEFAULT"] == 0);
        UNIT_ASSERT(ajaxCount["DEFAULT_RU1"] == 0);
        UNIT_ASSERT(ajaxCount["DEFAULT_RU2"] == 0);
        UNIT_ASSERT(ajaxCount["DEFAULT_RU_AJAX"] == numRequests);

        UNIT_ASSERT(generalCount["DEFAULT"] == 0);
        UNIT_ASSERT(generalCount["DEFAULT_RU1"] > 10);
        UNIT_ASSERT(generalCount["DEFAULT_RU2"] > 10);
        UNIT_ASSERT(generalCount["DEFAULT_RU_AJAX"] == 0);
    }
}

Y_UNIT_TEST(TestMatrixnetExpFormulasProbability) {
    auto tempDir = TTempDir("TestMatrixnetExpFormulasProbability");
    CreateTempFormulaFile(tempDir.Path() / "matrixnet.info");

    const TString configHeader = "<Daemon>\n"
                                 + StubIPsForHosts +
                                 "FormulasDir = TestMatrixnetExpFormulasProbability\n";
    const TString configFooter ="</Daemon>\n"
                                "<Zone>\n"
                                "MatrixnetFormulaFiles = default=matrixnet.info\n"
                                "</Zone>\n";

    const auto& loadProbability = [&](const TString& service, float probability) {
        TAntirobotDaemonConfig cfg;
        cfg.LoadFromString(configHeader + configFooter);
        return cfg.ExperimentsConfig.LoadExpFormulasProbability(NJson::TJsonValue::TMapType{{service, NJson::TJsonValue(probability)}});
    };

    const auto& loadProbability2 = [&](const TString& service1, float probability1, const TString& service2, float probability2) {
        TAntirobotDaemonConfig cfg;
        cfg.LoadFromString(configHeader + configFooter);
        return cfg.ExperimentsConfig.LoadExpFormulasProbability(NJson::TJsonValue::TMapType{
            {service1, NJson::TJsonValue(probability1)},
            {service2, NJson::TJsonValue(probability2)}
        });
    };

    const auto& loadProbability3 = [&](const TString& service1, float probability1, const TString& service2, float probability2, const TString& service3, float probability3) {
        TAntirobotDaemonConfig cfg;
        cfg.LoadFromString(configHeader + configFooter);
        return cfg.ExperimentsConfig.LoadExpFormulasProbability(NJson::TJsonValue::TMapType{
            {service1, NJson::TJsonValue(probability1)},
            {service2, NJson::TJsonValue(probability2)},
            {service3, NJson::TJsonValue(probability3)}
        });
    };

    const double eps = std::numeric_limits<float>::epsilon();

    UNIT_ASSERT_EXCEPTION(loadProbability("", 0), yexception);
    UNIT_ASSERT_EXCEPTION(loadProbability("web", 0.01), yexception);
    UNIT_ASSERT_EXCEPTION(loadProbability("nonexistent", 0.01), yexception);
    UNIT_ASSERT_EXCEPTION(loadProbability("web", 2.0), yexception);
    UNIT_ASSERT_EXCEPTION(loadProbability("web", -0.01), yexception);
    UNIT_ASSERT_EXCEPTION(loadProbability("web", 1.01), yexception);

    UNIT_ASSERT_DOUBLES_EQUAL(loadProbability("default", 1)[EHostType::HOST_WEB], 1, eps);
    UNIT_ASSERT_DOUBLES_EQUAL(loadProbability("default", 1.0)[EHostType::HOST_WEB], 1.0, eps);
    UNIT_ASSERT_DOUBLES_EQUAL(loadProbability("default", 0.999999)[EHostType::HOST_WEB], 0.999999, eps);
    UNIT_ASSERT_DOUBLES_EQUAL(loadProbability("default", 0.555555)[EHostType::HOST_WEB], 0.555555, eps);
    UNIT_ASSERT_DOUBLES_EQUAL(loadProbability("default", 0.111111)[EHostType::HOST_WEB], 0.111111, eps);
    UNIT_ASSERT_DOUBLES_EQUAL(loadProbability("default", 0.1)[EHostType::HOST_WEB], 0.1, eps);
    UNIT_ASSERT_DOUBLES_EQUAL(loadProbability("default", 0.01)[EHostType::HOST_WEB], 0.01, eps);
    UNIT_ASSERT_DOUBLES_EQUAL(loadProbability("default", 0.001)[EHostType::HOST_WEB], 0.001, eps);
    UNIT_ASSERT_DOUBLES_EQUAL(loadProbability("default", 0.0001)[EHostType::HOST_WEB], 0.0001, eps);
    UNIT_ASSERT_DOUBLES_EQUAL(loadProbability("default", 0.00001)[EHostType::HOST_WEB], 0.00001, eps);
    UNIT_ASSERT_DOUBLES_EQUAL(loadProbability("default", 0.0)[EHostType::HOST_WEB], 0.0, eps);
    UNIT_ASSERT_DOUBLES_EQUAL(loadProbability("default", 0)[EHostType::HOST_WEB], 0, eps);

    {
        auto probabilities = loadProbability("default", 1);
        UNIT_ASSERT_DOUBLES_EQUAL(probabilities[EHostType::HOST_WEB], 1, eps);
        UNIT_ASSERT_DOUBLES_EQUAL(probabilities[EHostType::HOST_MUSIC], 1, eps);
    }

    {
        auto probabilities = loadProbability2("default", 1, "web", 0.2);
        UNIT_ASSERT_DOUBLES_EQUAL(probabilities[EHostType::HOST_WEB], 0.2, eps);
        UNIT_ASSERT_DOUBLES_EQUAL(probabilities[EHostType::HOST_MUSIC], 1, eps);
    }

    {
        auto probabilities = loadProbability3("default", 1, "web", 0.2, "music", 0.3);
        UNIT_ASSERT_DOUBLES_EQUAL(probabilities[EHostType::HOST_WEB], 0.2, eps);
        UNIT_ASSERT_DOUBLES_EQUAL(probabilities[EHostType::HOST_MUSIC], 0.3, eps);
    }
}

}
