#include <library/cpp/json/json_prettifier.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>
#include <util/stream/file.h>

class TFmlConfigTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TFmlConfigTest);
    UNIT_TEST(TestValidJson);
    UNIT_TEST(TestSorted);
    UNIT_TEST(TestCanonized);
    UNIT_TEST_SUITE_END();

    TString FmlConfig;

public:
    void SetUp() override;

    void TestValidJson();
    void TestSorted();
    void TestCanonized();
};
UNIT_TEST_SUITE_REGISTRATION(TFmlConfigTest);

///////////////////////////////////////
//             TESTS                 //
///////////////////////////////////////
void TFmlConfigTest::SetUp() {
    FmlConfig = NResource::Find("fml_config.json");
    if (FmlConfig.EndsWith("\n")) {
        FmlConfig = FmlConfig.substr(0, FmlConfig.size() - 1);
    }
}

void TFmlConfigTest::TestValidJson() {
    UNIT_ASSERT(NJson::ValidateJson(FmlConfig, NJson::TJsonReaderConfig(), false));
}

void TFmlConfigTest::TestSorted() {
    auto etalon = NJson::PrettifyJson(NSc::TValue::FromJson(FmlConfig).ToJson(true), false, 4, false);
    UNIT_ASSERT_NO_DIFF(FmlConfig, etalon);
}

void TFmlConfigTest::TestCanonized() {
    const auto& config = NSc::TValue::FromJson(FmlConfig);
    NSc::TValue vert2dup;
    for (auto& vertical2tld : config.GetDict()) {
        const TString& vert = TString{vertical2tld.first};
        for (auto& tld2expr : vertical2tld.second.GetDict()) {
            const TString& tld = TString{tld2expr.first};
            for (auto& expr2prior : tld2expr.second.GetDict()) {
                const TString& expr = TString{expr2prior.first};
                for (auto& prior2cfg : expr2prior.second.GetDict()) {
                    const TString& prior = TString{prior2cfg.first};
                    const NSc::TValue& cfg = prior2cfg.second;
                    const TString& str = vert + " ===> " + expr + " ===> " + prior + " ===> " + cfg.ToJson(true);
                    vert2dup[str].Push(NSc::TValue(tld));
                }
            }
        }
    }
    NSc::TValue dups;
    for (auto& cfg2tlds : vert2dup.GetDict()) {
        if (cfg2tlds.second.ArraySize() > 1) {
            dups[cfg2tlds.first] = cfg2tlds.second;
        }
    }
    if (!dups.IsNull()) {
        auto pretty = NJson::PrettifyJson(dups.ToJson(true), false, 4, false);
        UNIT_ASSERT_STRINGS_EQUAL(pretty, "null");
    }
    UNIT_ASSERT(dups.IsNull());
}
