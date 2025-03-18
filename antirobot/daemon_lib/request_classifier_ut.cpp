#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include "request_classifier.h"

#include <antirobot/daemon_lib/ut/utils.h>

#include <library/cpp/json/json_writer.h>

#include <util/folder/tempdir.h>
#include <util/random/shuffle.h>
#include <util/stream/file.h>

using namespace NAntiRobot;

class TestRequestClassifierParams: public TTestBase {
public:
    void SetUp() override {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString("<Daemon>\n"
                                                       "CaptchaApiHost = ::\n"
                                                       "FormulasDir = .\n"
                                                       "CbbApiHost = ::\n"
                                                       "</Daemon>\n"
                                                       "<Zone></Zone>");
        TJsonConfigGenerator jsonConf;
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), GetJsonServiceIdentifierStr());
    }
};

Y_UNIT_TEST_SUITE(TestRequestClassifierBuilder) {
    Y_UNIT_TEST(TestCreateClassifierFromArray) {
        auto classifier = *TRegexpClassifierBuilder::CreateFromArray(
            {
                {Nothing(), "/a", Nothing(), REQ_MAIN},
                {Nothing(), "/b", Nothing(), REQ_YANDSEARCH},
                {Nothing(), "/c", Nothing(), REQ_XMLSEARCH}
            }, REQ_OTHER);

        UNIT_ASSERT_VALUES_EQUAL(classifier["/a?"], REQ_MAIN);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/b?smth"], REQ_YANDSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/c?smth=1"], REQ_XMLSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/d?smth=1&another=2"], REQ_OTHER);
    }

    Y_UNIT_TEST(TestCreateClassifierFromArrayWithDocAndCgi) {
        auto classifier = *TRegexpClassifierBuilder::CreateFromArray(
            {
                {Nothing(), "/a", TMaybe<TString>("smth=1"), REQ_YANDSEARCH},
                {Nothing(), "/b", TMaybe<TString>("smth=2"), REQ_XMLSEARCH},
                {Nothing(), "/a", TMaybe<TString>(".*"), REQ_MAIN}
            }, REQ_OTHER);

        UNIT_ASSERT_VALUES_EQUAL(classifier["/a?"], REQ_MAIN);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/a?smth=1"], REQ_YANDSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/b?smth=2"], REQ_XMLSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/a?smth=3"], REQ_MAIN);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/b?smth=1"], REQ_OTHER);
    }

    Y_UNIT_TEST(TestCreateClassifierFromArrayWithHostAndDoc) {
        auto classifier = *TRegexpClassifierBuilder::CreateFromArray(
            {
                {TMaybe<TString>("d"), "/a", TMaybe<TString>(".*"), REQ_YANDSEARCH},
                {TMaybe<TString>("e"), "/b", TMaybe<TString>(".*"), REQ_XMLSEARCH},
                {TMaybe<TString>("d"), "/b", TMaybe<TString>(".*"), REQ_MAIN}
            }, REQ_OTHER);

        UNIT_ASSERT_VALUES_EQUAL(classifier["d/a?"], REQ_YANDSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["e/b?"], REQ_XMLSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["d/b?"], REQ_MAIN);
        UNIT_ASSERT_VALUES_EQUAL(classifier["f/a?"], REQ_OTHER);
    }

    Y_UNIT_TEST(TestCreateClassifierFromArrayWithHostDocAndCgi) {
        auto classifier = *TRegexpClassifierBuilder::CreateFromArray(
            {
                {TMaybe<TString>("d"), "/a", TMaybe<TString>("smth=1"), REQ_YANDSEARCH},
                {TMaybe<TString>("e"), "/b", TMaybe<TString>("smth=1"), REQ_XMLSEARCH},
                {TMaybe<TString>("d"), "/b", TMaybe<TString>("smth=2"), REQ_MAIN}
            }, REQ_OTHER);

        UNIT_ASSERT_VALUES_EQUAL(classifier["d/a?smth=1"], REQ_YANDSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["e/b?smth=1"], REQ_XMLSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["d/b?smth=2"], REQ_MAIN);
        UNIT_ASSERT_VALUES_EQUAL(classifier["f/a?smth=1"], REQ_OTHER);
    }

    Y_UNIT_TEST(TestCreateClassifierFromLines) {
        auto classifier = *TRegexpClassifierBuilder::CreateFromLines({
                                                                         "/a\tmain",
                                                                         "/b\tys",
                                                                         "/c\txml",
                                                                     },
                                                                     REQ_OTHER);

        UNIT_ASSERT_VALUES_EQUAL(classifier["/a?"], REQ_MAIN);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/b?smth"], REQ_YANDSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/c?smth=1"], REQ_XMLSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/d?smth=1&another=2"], REQ_OTHER);
    }

    Y_UNIT_TEST(TestCreateClassifierFromLinesWithCgi) {
        auto classifier = *TRegexpClassifierBuilder::CreateFromLines({
                                                                         "/a\tsmth=1\tys",
                                                                         "/a\tsmth=2\txml",
                                                                         "/a\tmain",
                                                                     },
                                                                     REQ_OTHER);

        UNIT_ASSERT_VALUES_EQUAL(classifier["/a?"], REQ_MAIN);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/a?smth=1"], REQ_YANDSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/a?smth=2"], REQ_XMLSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/a?smth=3"], REQ_MAIN);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/b?smth=1"], REQ_OTHER);
    }

    Y_UNIT_TEST(TestCreateClassifierFromFilename) {
        auto tempDir = TTempDir();
        auto tempFilePath = tempDir.Path() / "TestCreateClassifierFromFilename";
        TFileOutput tempFile(tempFilePath);

        tempFile
            << "/a\tmain" << Endl
            << "/b\tys" << Endl
            << "/c\txml" << Endl;

        tempFile.Flush();

        auto classifier = *TRegexpClassifierBuilder::CreateFromFile(tempFilePath.GetPath(), REQ_OTHER);

        UNIT_ASSERT_VALUES_EQUAL(classifier["/a?"], REQ_MAIN);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/b?smth"], REQ_YANDSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/c?smth=1"], REQ_XMLSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/d?smth=1&another=2"], REQ_OTHER);
    }

    Y_UNIT_TEST(TestCreateClassifierFromFilenameIgnoringComments) {
        auto tempDir = TTempDir();
        auto tempFilePath = tempDir.Path() / "TestCreateClassifierFromFilenameIgnoringComments";
        TFileOutput tempFile(tempFilePath);

        tempFile
            << "# Some comment here" << Endl
            << "/a\tmain" << Endl
            << "/b\tys" << Endl
            << "/c\txml" << Endl
            << "# /d\tms" << Endl
            << "# Some comment there" << Endl;

        tempFile.Flush();

        auto classifier = *TRegexpClassifierBuilder::CreateFromFile(tempFilePath.GetPath(), REQ_OTHER);

        UNIT_ASSERT_VALUES_EQUAL(classifier["/a?"], REQ_MAIN);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/b?smth"], REQ_YANDSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/c?smth=1"], REQ_XMLSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/d?smth=1&another=2"], REQ_OTHER);
    }

    Y_UNIT_TEST(TestCreateClassifierFromFilenameIgnoringLeadingAndTrailingWhitespaces) {
        auto tempDir = TTempDir();
        auto tempFilePath = tempDir.Path() / "TestCreateClassifierFromFilenameIgnoringLeadingAndTrailingWhitespaces";
        TFileOutput tempFile(tempFilePath);

        tempFile
            << " /a\tmain" << Endl
            << "\t/b\tys" << Endl
            << "  /c\txml\t" << Endl
            << "\t /c2\txml \t" << Endl
            << " # /d\tms" << Endl
            << "\t# /d\tms" << Endl
            << "\t # /d\tms" << Endl
            << " \t# /d\tms" << Endl
            << "  # /d\tms" << Endl
            << "\t\t# /d\tms" << Endl;

        tempFile.Flush();

        auto classifier = *TRegexpClassifierBuilder::CreateFromFile(tempFilePath.GetPath(), REQ_OTHER);

        UNIT_ASSERT_VALUES_EQUAL(classifier["/a?"], REQ_MAIN);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/b?smth"], REQ_YANDSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/c?smth=1"], REQ_XMLSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/c2?smth=1"], REQ_XMLSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/d?smth=1&another=2"], REQ_OTHER);
    }

    Y_UNIT_TEST(TestCreateClassifierFromFilenameAllowingMultipleTabsAndSpacesAsDelimiters) {
        auto tempDir = TTempDir();
        auto tempFilePath = tempDir.Path() / "TestCreateClassifierFromFilenameAllowingMultipleTabsAndSpacesAsDelimiters";
        TFileOutput tempFile(tempFilePath);

        tempFile
            << "/a\t\tmain" << Endl
            << "/b  ys" << Endl
            << "/c \txml" << Endl
            << "/c2\t xml" << Endl;

        tempFile.Flush();

        auto classifier = *TRegexpClassifierBuilder::CreateFromFile(tempFilePath.GetPath(), REQ_OTHER);

        UNIT_ASSERT_VALUES_EQUAL(classifier["/a?"], REQ_MAIN);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/b?smth"], REQ_YANDSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/c?smth=1"], REQ_XMLSEARCH);
        UNIT_ASSERT_VALUES_EQUAL(classifier["/c2?smth=1"], REQ_XMLSEARCH);
    }

    Y_UNIT_TEST(TestCreateClassifierFromFilenameThrowsOnBadRules) {
        auto badRules = {"SOME_BAD_RULE", "/ unknown_req_type", "/smth foo=bar unknown_req_type"};

        for (auto badRule : badRules) {
            auto tempDir = TTempDir();
            auto tempFilePath = tempDir.Path() / "TestCreateClassifierFromFilenameThrowsOnBadRules";
            TFileOutput tempFile(tempFilePath);

            tempFile << badRule << Endl;
            tempFile.Flush();

            UNIT_ASSERT_EXCEPTION(TRegexpClassifierBuilder::CreateFromFile(tempFilePath.GetPath(), REQ_OTHER), yexception);
        }
    }

    Y_UNIT_TEST(TestCreateClassifierFromFilenameMarketRequests) {
        const TString rulesPath = ArcadiaSourceRoot() + "/antirobot/daemon_lib/ut/test_files/"
                                                        "TestCreateClassifierFromFilenameMarketRequests";

        auto classifier = *TRegexpClassifierBuilder::CreateFromFile(rulesPath, REQ_OTHER);

        const std::pair<TString, EReqType> REQS[] = {
            {"/catalogoffers.xml", REQ_CATALOG_OFFERS},
            {"/catalogmodels.xml", REQ_CATALOG_MODELS},
            {"/catalog", REQ_CATALOG},
            {"/catalog/12345", REQ_CATALOG},
            {"/catalog/ds/12", REQ_CATALOG},
            {"/catalog.xml", REQ_CATALOG},
            {"/collections", REQ_COLLECTIONS},
            {"/collections/12345", REQ_COLLECTIONS},
            {"/collections/ds/12", REQ_COLLECTIONS},
            {"/model-prices.xml", REQ_MODEL_PRICES},
            {"/product/12345/reviews", REQ_MODEL},
            {"/guru.xml", REQ_GURU},
            {"/offers.xml", REQ_OFFERS},
            {"/model-opinions.xml", REQ_MODEL_OPINIONS},
            {"/brands-list", REQ_BRANDS},
            {"/compare.xml", REQ_COMPARE},
            {"/shop", REQ_SHOP},
            {"/search.xml", REQ_YANDSEARCH},
            {"/guru_filter.xml", REQ_OTHER},
            {"/", REQ_MAIN},
            {"/_/IPTWNHaZr1NPkiF6pJY2y4iDDO8.css", REQ_STATIC},
            {"/catalog/91650/list", REQ_CATALOG},
            {"/catalog/4684840/filters", REQ_CATALOG},
            {"/collections/91650/list", REQ_COLLECTIONS},
            {"/collections/", REQ_COLLECTIONS},
            {"/product/10760531/", REQ_MODEL},
            {"/product/10760531", REQ_MODEL},
            {"/product/10583605/offers", REQ_MODEL},
            {"/model-reviews.xml", REQ_MODEL},
            {"/brands/7342300", REQ_BRANDS},
            {"/shop/6069/reviews", REQ_SHOP},
            {"/gate/actualize", REQ_OTHER},
            {"/find-in-other-shop/123454/98765432", REQ_SHOP},
            {"/articles/some-podborka", REQ_CATALOG},
            {"/gate/order-loader", REQ_SHOP},
            {"/gate/order/cart", REQ_SHOP},
            {"/stats.xml", REQ_YANDSEARCH},
            {"/banki", REQ_OTHER},
            {"/banki/ipoteka", REQ_OTHER},
            {"/banki/ipoteka/search.xml", REQ_YANDSEARCH},
            {"/banki/deposits/search.xml", REQ_YANDSEARCH},
            {"/checkout", REQ_CHECKOUT},
            {"/api/cart", REQ_CHECKOUT},
        };

        for (const auto& request : REQS) {
            UNIT_ASSERT_VALUES_EQUAL_C(classifier[request.first + "?"], request.second, request.first);
        }
    }
}

Y_UNIT_TEST_SUITE_IMPL(TestRequestClassifier, TestRequestClassifierParams) {
    Y_UNIT_TEST(TestIsMobile) {
        auto classifier = CreateClassifierForTests();
        UNIT_ASSERT(classifier.IsMobileRequest("m.yandex.ru", "https://m.yandex.ru/search"));
        UNIT_ASSERT(classifier.IsMobileRequest("m.yandex.ru", "https://yandex.ru/search"));
        UNIT_ASSERT(!classifier.IsMobileRequest("yandex.ru", "https://yandex.ru/search"));
        UNIT_ASSERT(classifier.IsMobileRequest("yandex.ru", "https://yandex.ru/search/touch"));
        UNIT_ASSERT(classifier.IsMobileRequest("yandex.ru", "https://yandex.ru/images/touch/search"));
    }

    Y_UNIT_TEST(CreateFromFile) {
        TJsonConfigGenerator generator;
        std::array<std::vector<std::pair<TString, TString>>, EHostType::HOST_NUMTYPES> rules;
        rules[EHostType::HOST_WEB] = {{"/search", "ys"}};
        generator.SetReQueries(rules);
        auto config = generator.Create();
        auto tempDir = TTempDir();
        auto tempFilePath = tempDir.Path() / "TestRequestClassifierCreateFromFile";
        TFileOutput tempFile(tempFilePath);
        tempFile << config;
        tempFile.Flush();

        auto classifier = TRequestClassifier::CreateFromJsonConfig(tempFilePath);

        UNIT_ASSERT_VALUES_EQUAL(classifier.DetectRequestType(EHostType::HOST_WEB, "/search", "", EReqType::REQ_OTHER),
                                 EReqType::REQ_YANDSEARCH);
    }

    std::array<std::vector<std::pair<TString, TString>>, EHostType::HOST_NUMTYPES> GetClassifierRules() {
        std::array<std::vector<std::pair<TString, TString>>, EHostType::HOST_NUMTYPES> rules;
        for (size_t i = EHostType::HOST_TYPE_FIRST; i < EHostType::HOST_TYPE_LAST; i++) {
            auto service = static_cast<EHostType>(i);
            rules[service] = {{"/" + ToString(service), "ys"}};
        }
        return rules;
    }

    NJson::TJsonValue ShuffleConfig(const NJson::TJsonValue& config) {
        auto services = config.GetArraySafe();
        ShuffleRange(services);
        // To guarantee that order is changed
        if (services[0]["service"].GetStringSafe() == "other") {
            std::swap(services[0], services[1]);
        }
        NJson::TJsonValue shuffledConfig;
        for (const auto& service : services) {
            shuffledConfig.AppendValue(service);
        }
        return shuffledConfig;
    }

    TFsPath GetPathToJsonConfigWithShuffledServices(const TTempDir& tempDir) {
        TJsonConfigGenerator generator;
        auto rules = GetClassifierRules();
        generator.SetReQueries(rules);
        auto config = generator.Create();

        NJson::TJsonValue configJson;
        ReadJsonTree(config, &configJson, true);

        auto shuffledConfigJson = ShuffleConfig(configJson);
        auto shuffledConfig = NJson::WriteJson(shuffledConfigJson);

        auto tempFilePath = tempDir.Path() / "TestRequestClassifierCreateFromFileIndependentOnOrder";
        TFileOutput tempFile(tempFilePath);
        tempFile << shuffledConfig;
        tempFile.Flush();

        return tempFilePath;
    }

    void AssertServiceIsCorrect(EHostType service, const TRequestClassifier& classifier) {
        UNIT_ASSERT_VALUES_EQUAL_C(classifier.DetectRequestType(service, "/" + ToString(service), "", EReqType::REQ_OTHER),
                                   EReqType::REQ_YANDSEARCH,
                                   "Rules for " << service << " are incorrect");
    }

    Y_UNIT_TEST(CreateFromFileIndependentOfOrder) {
        auto tempDir = TTempDir();
        auto jsonConfigPath = GetPathToJsonConfigWithShuffledServices(tempDir);

        auto classifier = TRequestClassifier::CreateFromJsonConfig(jsonConfigPath);

        for (size_t i = EHostType::HOST_TYPE_FIRST; i < EHostType::HOST_TYPE_LAST; i++) {
            auto service = static_cast<EHostType>(i);
            AssertServiceIsCorrect(service, classifier);
        }
    }
}
