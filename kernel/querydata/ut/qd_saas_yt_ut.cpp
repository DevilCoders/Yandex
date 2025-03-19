#include <kernel/querydata/saas_yt/qd_saas_yt.h>
#include <kernel/querydata/saas_yt/qd_raw_trie_conversion.h>
#include <kernel/querydata/saas_yt/idl/qd_saas_input_meta.pb.h>
#include <kernel/querydata/saas_yt/idl/qd_saas_input_record.pb.h>
#include <kernel/querydata/saas_yt/idl/qd_saas_snapshot_record.pb.h>
#include <kernel/querydata/idl/querydata_structs.pb.h>
#include <kernel/querydata/ut_utils/qd_ut_utils.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/split.h>
#include <util/string/join.h>
#include <google/protobuf/text_format.h>

class TSaaSQuerySearchIndexer : public NUnitTest::TTestBase {
    UNIT_TEST_SUITE(TSaaSQuerySearchIndexer)
        UNIT_TEST(TestValidateMeta)
        UNIT_TEST(TestInputRecord2SnapshotRecord)
        UNIT_TEST(TestQDSourceFactors2InputRecord)
        UNIT_TEST(TestQDFileDescription2InputMeta)
        UNIT_TEST(TestConvertMask)
    UNIT_TEST_SUITE_END();
public:

    template <class T>
    static T ParseFromText(const TString& text) {
        T record;
        google::protobuf::TextFormat::Parser p;
        Y_ENSURE(p.ParseFromString(text, &record), "could not parse '" << text << "'");
        return record;
    }

    struct TTest {
        TString Meta;
        TString Input;
        TString Expected;

        TTest(const TString& meta, const TString& in, const TString& out)
            : Meta(meta)
            , Input(in)
            , Expected(out)
        {}
    };

    const TVector<std::pair<TString, bool>> TestData{
            {"Namespace: \"invalid/nspace\" Timestamp_Microseconds: 1486393062123456", false},
            {"Namespace: \"\" Timestamp_Microseconds: 1486393062123456", false},
            {"Namespace: \"invalid_tstamp\" Timestamp_Microseconds: 14863930621234560", false},
            {"Namespace: \"invalid_tstamp\" Timestamp_Microseconds: 148639306212345", false},
            {"Namespace: \"ok:nspace\" Timestamp_Microseconds: 1486393062123456", true}
    };

    void TestValidateMeta() {
        for (const auto& test : TestData) {
            auto meta = ParseFromText<NQueryDataSaaS::TQDSaaSInputMeta>(test.first);
            if (test.second) {
                UNIT_ASSERT_NO_EXCEPTION(NQueryDataSaaS::ValidateInputMeta(meta));
            } else {
                UNIT_ASSERT_EXCEPTION(NQueryDataSaaS::ValidateInputMeta(meta), yexception);
            }
        }
    }

    const TVector<TTest> TestsVec{
            {"Namespace: \"invalid_key\" Timestamp_Microseconds: 1486393062123456", "", ""},
            {"Namespace: \"invalid_key\" Timestamp_Microseconds: 1486393062123456", "Subkey_QueryStrong: \" \"", ""},
            {
                "Namespace: \"V_alid-ate:1\" Timestamp_Microseconds: 1486393062123456", "Subkey_QueryStrong: \" xxx \"",
                "SaasKey: \"FD57E897A380D5A7CAE51D4D21940CF3\" "
                    "SaasPropName: \"QDSaaS:V_alid-ate:1\" "
                    "Timestamp: 1486393062123456 "
                    "SourceFactors { SourceName: \"V_alid-ate:1\" Version: 1486393062123456 SourceKey: \"xxx\" SourceKeyType: KT_QUERY_STRONG } "
                    "Namespace: \"V_alid-ate:1\" "
                    "SourceFactorsHR: \"SourceName: \\\"V_alid-ate:1\\\" Version: 1486393062123456 SourceKey: \\\"xxx\\\" SourceKeyType: KT_QUERY_STRONG\" "
                    "Subkey_QueryStrong: \"xxx\""
            },
            {
                "Namespace: \"all_keys_left\" Timestamp_Microseconds: 1486393062123456",
                    "Subkey_QueryDoppel: \"query\" "
                    "Subkey_Url: \"Kernel.org/\" "
                    "Subkey_Owner: \"yandex.ru\" "
                    "Subkey_UrlMask: \"yandex.ru www.yandex.ru/search\" "
                    "Subkey_ZDocId: \"Z1234567812345678\" "
                    "Subkey_UserId: \"y1234\" "
                    "Subkey_UserLoginHash: \"fafafafa\" "
                    "Subkey_UserRegion: \"213\" "
                    "Subkey_UserRegionIpReg: \"225\" "
                    "Subkey_UserIpType: \"mobile_any\" "
                    "Subkey_StructKey: \"aKey\" "
                    "Subkey_SerpTLD: \"com.tr\" "
                    "Subkey_SerpUIL: \"ru\" "
                    "Subkey_SerpDevice: \"touch\"",
                "SaasKey: \"103623CD4F5B886A58C9ED65BD999934\" "
                    "SaasPropName: \"QDSaaS:all_keys_left~225~mobile_any~com.tr~ru~touch\" "
                    "Timestamp: 1486393062123456 "
                    "SourceFactors { SourceName: \"all_keys_left\" Version: 1486393062123456 "
                        "SourceKey: \"query\" SourceKeyType: KT_QUERY_DOPPEL "
                        "SourceSubkeys { Key: \"y1234\" Type: KT_USER_ID } "
                        "SourceSubkeys { Key: \"fafafafa\" Type: KT_USER_LOGIN_HASH } "
                        "SourceSubkeys { Key: \"http://kernel.org/\" Type: KT_URL } "
                        "SourceSubkeys { Key: \"yandex.ru\" Type: KT_CATEG } "
                        "SourceSubkeys { Key: \"www.yandex.ru/search\" Type: KT_CATEG_URL } "
                        "SourceSubkeys { Key: \"Z1234567812345678\" Type: KT_DOCID } "
                        "SourceSubkeys { Key: \"aKey\" Type: KT_STRUCTKEY } "
                        "SourceSubkeys { Key: \"213\" Type: KT_USER_REGION } "
                        "SourceSubkeys { Key: \"225\" Type: KT_USER_REGION_IPREG } "
                        "SourceSubkeys { Key: \"mobile_any\" Type: KT_USER_IP_TYPE } "
                        "SourceSubkeys { Key: \"com.tr\" Type: KT_SERP_TLD } "
                        "SourceSubkeys { Key: \"ru\" Type: KT_SERP_UIL } "
                        "SourceSubkeys { Key: \"touch\" Type: KT_SERP_DEVICE } "
                    "} "
                    "Namespace: \"all_keys_left\" "
                    "SourceFactorsHR: \"SourceName: \\\"all_keys_left\\\" Version: 1486393062123456 "
                        "SourceKey: \\\"query\\\" SourceKeyType: KT_QUERY_DOPPEL "
                        "SourceSubkeys { Key: \\\"y1234\\\" Type: KT_USER_ID } "
                        "SourceSubkeys { Key: \\\"fafafafa\\\" Type: KT_USER_LOGIN_HASH } "
                        "SourceSubkeys { Key: \\\"http://kernel.org/\\\" Type: KT_URL } "
                        "SourceSubkeys { Key: \\\"yandex.ru\\\" Type: KT_CATEG } "
                        "SourceSubkeys { Key: \\\"www.yandex.ru/search\\\" Type: KT_CATEG_URL } "
                        "SourceSubkeys { Key: \\\"Z1234567812345678\\\" Type: KT_DOCID } "
                        "SourceSubkeys { Key: \\\"aKey\\\" Type: KT_STRUCTKEY } "
                        "SourceSubkeys { Key: \\\"213\\\" Type: KT_USER_REGION } "
                        "SourceSubkeys { Key: \\\"225\\\" Type: KT_USER_REGION_IPREG } "
                        "SourceSubkeys { Key: \\\"mobile_any\\\" Type: KT_USER_IP_TYPE } "
                        "SourceSubkeys { Key: \\\"com.tr\\\" Type: KT_SERP_TLD } "
                        "SourceSubkeys { Key: \\\"ru\\\" Type: KT_SERP_UIL } "
                        "SourceSubkeys { Key: \\\"touch\\\" Type: KT_SERP_DEVICE }\" "
                    "Subkey_QueryDoppel: \"query\" "
                    "Subkey_Url: \"http://kernel.org/\" "
                    "Subkey_Owner: \"yandex.ru\" "
                    "Subkey_UrlMask: \"www.yandex.ru/search\" "
                    "Subkey_ZDocId: \"Z1234567812345678\" "
                    "Subkey_UserId: \"y1234\" "
                    "Subkey_UserLoginHash: \"fafafafa\" "
                    "Subkey_UserRegion: \"213\" "
                    "Subkey_UserRegionIpReg: \"225\" "
                    "Subkey_UserIpType: \"mobile_any\" "
                    "Subkey_StructKey: \"all_keys_left@aKey\" "
                    "Subkey_SerpTLD: \"com.tr\" "
                    "Subkey_SerpUIL: \"ru\" "
                    "Subkey_SerpDevice: \"touch\""
            },
            {
                "Namespace: \"forbidden_standalone_keys\" Timestamp_Microseconds: 1486393062123456",
                    "Subkey_UserRegion: \"213\" "
                    "Subkey_UserRegionIpReg: \"225\" "
                    "Subkey_UserIpType: \"mobile_any\" "
                    "Subkey_SerpTLD: \"com.tr\" "
                    "Subkey_SerpUIL: \"ru\" "
                    "Subkey_SerpDevice: \"touch\"",
                ""
            },
            {
                "Namespace: \"json_value\" Timestamp_Microseconds: 1486393062123456",
                "Subkey_QueryStrong: \"xxx\" Data_JSON: \"{\\\"x\\\":\\\"y\\\"}\"",
                "SaasKey: \"FD57E897A380D5A7CAE51D4D21940CF3\" SaasPropName: \"QDSaaS:json_value\" Timestamp: 1486393062123456 "
                    "SourceFactors { SourceName: \"json_value\" Version: 1486393062123456 SourceKey: \"xxx\" SourceKeyType: KT_QUERY_STRONG Json: \"{\\\"x\\\":\\\"y\\\"}\" } "
                    "Namespace: \"json_value\" "
                    "SourceFactorsHR: \"SourceName: \\\"json_value\\\" Version: 1486393062123456 SourceKey: \\\"xxx\\\" SourceKeyType: KT_QUERY_STRONG Json: \\\"{\\\\\\\"x\\\\\\\":\\\\\\\"y\\\\\\\"}\\\"\" Subkey_QueryStrong: \"xxx\""
            },
            {
                "Namespace: \"invalid_json_value\" Timestamp_Microseconds: 1486393062123456",
                "Subkey_QueryStrong: \"xxx\" Data_JSON: \"{x:y}\"",
                ""
            },
            {
                "Namespace: \"tskv_value\" Timestamp_Microseconds: 1486393062123456",
                "Subkey_QueryStrong: \"xxx\" Data_TSKV: \"f-i:i=1\tf-f:f=1.5\tf-s=xxx\tf-b:b=a\\\\tb\tf-none\tf-B:B=YWJj\"",
                "SaasKey: \"FD57E897A380D5A7CAE51D4D21940CF3\" "
                    "SaasPropName: \"QDSaaS:tskv_value\" "
                    "Timestamp: 1486393062123456 SourceFactors { SourceName: \"tskv_value\" Version: 1486393062123456 "
                    "Factors { Name: \"f-i\" IntValue: 1 } "
                    "Factors { Name: \"f-f\" FloatValue: 1.5 } "
                    "Factors { Name: \"f-s\" StringValue: \"xxx\" } "
                    "Factors { Name: \"f-b\" StringValue: \"a\\tb\" } "
                    "Factors { Name: \"f-none\" StringValue: \"\" } "
                    "Factors { Name: \"f-B\" BinaryValue: \"abc\" } "
                    "SourceKey: \"xxx\" SourceKeyType: KT_QUERY_STRONG } "
                    "Namespace: \"tskv_value\" "
                    "SourceFactorsHR: \"SourceName: \\\"tskv_value\\\" Version: 1486393062123456 "
                    "Factors { Name: \\\"f-i\\\" IntValue: 1 } "
                    "Factors { Name: \\\"f-f\\\" FloatValue: 1.5 } "
                    "Factors { Name: \\\"f-s\\\" StringValue: \\\"xxx\\\" } "
                    "Factors { Name: \\\"f-b\\\" StringValue: \\\"a\\\\tb\\\" } "
                    "Factors { Name: \\\"f-none\\\" StringValue: \\\"\\\" } "
                    "Factors { Name: \\\"f-B\\\" BinaryValue: \\\"abc\\\" } "
                    "SourceKey: \\\"xxx\\\" SourceKeyType: KT_QUERY_STRONG\" "
                    "Subkey_QueryStrong: \"xxx\""
            },
    };

    void TestInputRecord2SnapshotRecord() {
        using namespace NQueryDataSaaS;

        for (const auto& test : TestsVec) {
            auto in = ParseFromText<TQDSaaSInputRecord>(test.Input);
            auto meta = ParseFromText<TQDSaaSInputMeta>(test.Meta);
            TQDSaaSSnapshotRecord out;
            if (test.Expected) {
                FillSnapshotRecordFromInputRecord(out, in, meta, true, EQDSaaSType::KV);
                UNIT_ASSERT_VALUES_EQUAL(out.ShortUtf8DebugString(), test.Expected);
            } else {
                UNIT_ASSERT_EXCEPTION(FillSnapshotRecordFromInputRecord(out, in, meta, true, EQDSaaSType::KV), yexception);
            }
        }
    }

    const TVector<std::pair<TString, TString>> TestPairs{
            {"", ""},
            {
                "SourceName: \"Test1/factors-test\" Version: 1486389984 SourceKey: \"xxx\" SourceKeyType: KT_QUERY_STRONG "
                "SourceSubkeys { Key: \"Kernel.org/\" Type: KT_URL } "
                "SourceSubkeys { Key: \"com.tr\" Type: KT_SERP_TLD } "
                "Factors { Name: \"f-i\" IntValue: 1} "
                "Factors { Name: \"f-f\" FloatValue: 1.5 } "
                "Factors { Name: \"f-s\" StringValue: \"abc\" } "
                "Factors { Name: \"f-b\" StringValue: \"a\tb\" } "
                "Factors { Name: \"f-none\" } "
                "Factors { Name: \"f-B\" BinaryValue: \"abc\" }",
                "Subkey_QueryStrong: \"xxx\" "
                "Subkey_Url: \"http://kernel.org/\" "
                "Subkey_SerpTLD: \"com.tr\" "
                "Data_TSKV: \"f-i:i=1\\tf-f:f=1.5\\tf-s:s=abc\\tf-b:b=a\\\\tb\\tf-none\\tf-B:B=YWJj\""
            },
            {
                "SourceName: \"Test2:json_test\" Version: 20160101 SourceKey: \"yandex.ru www.yandex.ru/search\" SourceKeyType: KT_SNIPCATEG_URL "
                "SourceSubkeys { Key: \"213\" Type: KT_USER_REGION } "
                "SourceSubkeys { Key: \"Z1234567812345678\" Type: KT_SNIPDOCID } "
                "SourceSubkeys { Key: \"yandex.ru\" Type: KT_SNIPCATEG } "
                "SourceSubkeys { Key: \"ru\" Type: KT_SERP_UIL } "
                "Json: \"{\\\"x\\\":\\\"y\\\"}\"",
                "Subkey_Owner: \"yandex.ru\" "
                "Subkey_UrlMask: \"www.yandex.ru/search\" "
                "Subkey_ZDocId: \"Z1234567812345678\" "
                "Subkey_UserRegion: \"213\" "
                "Subkey_SerpUIL: \"ru\" "
                "Data_JSON: \"{\\\"x\\\":\\\"y\\\"}\""
            },
            {
                "SourceName: \"all_keys_left\" Version: 1486389984123456 "
                "SourceSubkeys { Key: \"213\" Type: KT_USER_REGION_IPREG } "
                "SourceSubkeys { Key: \"mobile_any\" Type: KT_USER_IP_TYPE } "
                "SourceSubkeys { Key: \"touch\" Type: KT_SERP_DEVICE } "
                "SourceSubkeys { Key: \"xxx\" Type: KT_STRUCTKEY } "
                "SourceSubkeys { Key: \"yyy\" Type: KT_QUERY_DOPPEL } "
                "SourceSubkeys { Key: \"y10303535353\" Type: KT_USER_ID } "
                "SourceSubkeys { Key: \"0abc\" Type: KT_USER_LOGIN_HASH } "
                "SourceSubkeys { Key: \"Z1234567812345678\" Type: KT_DOCID } "
                "SourceSubkeys { Key: \"yandex.ru\" Type: KT_CATEG } "
                "SourceSubkeys { Key: \"yandex.ru www.yandex.ru/search\" Type: KT_CATEG_URL }",
                "Subkey_QueryDoppel: \"yyy\" "
                "Subkey_Owner: \"yandex.ru\" "
                "Subkey_UrlMask: \"www.yandex.ru/search\" "
                "Subkey_ZDocId: \"Z1234567812345678\" "
                "Subkey_UserId: \"y10303535353\" "
                "Subkey_UserLoginHash: \"0abc\" "
                "Subkey_UserRegionIpReg: \"213\" "
                "Subkey_UserIpType: \"mobile_any\" "
                "Subkey_StructKey: \"xxx\" "
                "Subkey_SerpDevice: \"touch\""
            },
            {
                "SourceName: \"test\" Version: 1486389984 SourceKey: \"www.yandex.ru/search\" SourceKeyType: KT_CATEG_URL",
                "Subkey_UrlMask: \"www.yandex.ru/search\""
            },
            {
                "SourceName: \"forbidden_standalone_keys\" Version: 1486389984 "
                "SourceSubkeys { Key: \"xxx\" Type: KT_USER_REGION_IPREG } "
                "SourceSubkeys { Key: \"xxx\" Type: KT_USER_IP_TYPE } "
                "SourceSubkeys { Key: \"xxx\" Type: KT_SERP_TLD } "
                "SourceSubkeys { Key: \"xxx\" Type: KT_SERP_UIL } "
                "SourceSubkeys { Key: \"xxx\" Type: KT_SERP_DEVICE } "
                "SourceSubkeys { Key: \"xxx\" Type: KT_USER_REGION }",
                ""
            },
    };

    void TestQDSourceFactors2InputRecord() {
        using namespace NQueryDataSaaS;

        for (const auto& test : TestPairs) {
            auto in = ParseFromText<NQueryData::TSourceFactors>(test.first);
            TQDSaaSInputRecord out;
            if (test.second) {
                FillInputRecordFromQDSourceFactors(out, in);
                UNIT_ASSERT_VALUES_EQUAL(out.ShortUtf8DebugString(), test.second);
            } else {
                UNIT_ASSERT_EXCEPTION(FillInputRecordFromQDSourceFactors(out, in), yexception);
            }
        }
    }

    const TVector<std::pair<TString, TString>> TestPairs2{
            {"SourceName: \"xxx\" Version: 1486393062 KeyType: KT_QUERY_STRONG", "Namespace: \"xxx\" Timestamp_Microseconds: 1486393062000000"},
            {"SourceName: \"xxx/yyy\" Version: 1486393062123456 KeyType: KT_QUERY_STRONG", "Namespace: \"yyy:xxx\" Timestamp_Microseconds: 1486393062000000"},
    };

    void TestQDFileDescription2InputMeta() {
        using namespace NQueryDataSaaS;

        for (const auto& test : TestPairs2) {
            auto in = ParseFromText<NQueryData::TFileDescription>(test.first);
            TQDSaaSInputMeta out;
            if (test.second) {
                FillInputMetaFromFileDescription(out, in);
                UNIT_ASSERT_VALUES_EQUAL(out.ShortUtf8DebugString(), test.second);
            } else {
                UNIT_ASSERT_EXCEPTION(FillInputMetaFromFileDescription(out, in), yexception);
            }
        }
    }

    void TestConvertMask() {
        UNIT_ASSERT_VALUES_EQUAL(NQueryDataSaaS::TrieMask2SaaSUrlMask("0---olsen.tk tk.0---olsen.www*"), "www.0---olsen.tk");
        UNIT_ASSERT_VALUES_EQUAL(NQueryDataSaaS::TrieMask2SaaSUrlMask("0---olsen.tk tk.0---olsen.www/*"), "www.0---olsen.tk/");
        UNIT_ASSERT_VALUES_EQUAL(NQueryDataSaaS::TrieMask2SaaSUrlMask("0-love.ru ru.0-love/20clip/*"), "0-love.ru/20clip/");
    }
};

UNIT_TEST_SUITE_REGISTRATION(TSaaSQuerySearchIndexer);
