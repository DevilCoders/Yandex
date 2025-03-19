#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/resource/resource.h>

#include <kernel/extended_mx_calcer/proto/typedefs.h>
#include <kernel/extended_mx_calcer/multipredict/multipredict.h>

using namespace NExtendedMx;

class TMultiPredictTest : public TTestBase {
    NSc::TValue MultiPredicts;
    //tests
    UNIT_TEST_SUITE(TMultiPredictTest);
    UNIT_TEST(Test);
    UNIT_TEST_SUITE_END();
public:
    void SetUp() override;

    void TestGeneric(size_t multiPredictIndex, const TString& featureContext, const TString& availableCombinations, const TString& expectedResult);
    void Test();
};

UNIT_TEST_SUITE_REGISTRATION(TMultiPredictTest);

void TMultiPredictTest::SetUp() {
    const TString multipredicts = NResource::Find("multipredicts");
    MultiPredicts = NJsonConverters::FromJson<NSc::TValue>(multipredicts, false);
}

///////////////////////////////////////
//             TESTS                 //
///////////////////////////////////////
void TMultiPredictTest::TestGeneric(size_t multiPredictIndex, const TString& featContextJson, const TString& availCombsJson, const TString& expectedResult) {
    auto featContext = NSc::TValue::FromJsonThrow(featContextJson);
    auto availCombs = NSc::TValue::FromJsonThrow(availCombsJson);
    TFeatureContextDictConstProto featContextProto(&featContext);
    TAvailVTCombinationsConst availVtCombProto(&availCombs);
    auto multiPredict = MakeMultiPredict(TMultiPredictConstProto(&MultiPredicts[multiPredictIndex]));
    TDebug debug(false);
    auto result = *multiPredict->GetBestAvailableResult(featContextProto, availVtCombProto, debug).Scheme().Value__;
    UNIT_ASSERT(result.ToJson() == expectedResult);
}


void TMultiPredictTest::Test() {
    TestGeneric(0, "", "", "{\"Place\":{\"Result\":\"Main\"},\"Pos\":{\"Result\":6},\"ViewType\":{\"Result\":\"v4thumbs\"}}");
    TestGeneric(0, "{}", "{}", "{\"Place\":{\"Result\":\"\"},\"Pos\":{\"Result\":100},\"ViewType\":{\"Result\":\"\"}}");
    TestGeneric(0, "{ViewType:{AvailibleValues:{v4thumbs:0, vital:0, xl:0}}}", "{}", "{\"Place\":{\"Result\":\"\"},\"Pos\":{\"Result\":100},\"ViewType\":{\"Result\":\"\"}}");
    TestGeneric(0, "{ViewType:{AvailibleValues:{v4thumbs:1, vital:1, xl:1}}}", "", "{\"Place\":{\"Result\":\"Main\"},\"Pos\":{\"Result\":6},\"ViewType\":{\"Result\":\"v4thumbs\"}}");
    TestGeneric(0, "{ViewType:{AvailibleValues:{xl:1}}}", "", "{\"Place\":{\"Result\":\"\"},\"Pos\":{\"Result\":100},\"ViewType\":{\"Result\":\"\"}}");
    TestGeneric(0, "{ViewType:{AvailibleValues:{v4thumbs:0,vital:1,xl:1}}}", "", "{\"Place\":{\"Result\":\"Main\"},\"Pos\":{\"Result\":3},\"ViewType\":{\"Result\":\"vital\"}}");
    TestGeneric(0, "", "{v4thumbs:{Main:\"*\"},vital:{Main:\"*\"},xl:{Main:\"*\"}}", "{\"Place\":{\"Result\":\"Main\"},\"Pos\":{\"Result\":6},\"ViewType\":{\"Result\":\"v4thumbs\"}}");
    TestGeneric(0, "", "{vital:{Main:\"*\"},xl:{Main:\"*\"}}", "{\"Place\":{\"Result\":\"Main\"},\"Pos\":{\"Result\":3},\"ViewType\":{\"Result\":\"vital\"}}");
    TestGeneric(0, "", "{v4thumbs:{Main:[0,1,2,3,4,5,7,8,9,10]},vital:{Main:\"*\"},xl:{Main:\"*\"}}", "{\"Place\":{\"Result\":\"Main\"},\"Pos\":{\"Result\":8},\"ViewType\":{\"Result\":\"v4thumbs\"}}");
    TestGeneric(0, "", "{v4thumbs:{Right:\"*\"},vital:{Main:[0,1,2,4,5,6,7,8,9,10]}}", "{\"Place\":{\"Result\":\"Main\"},\"Pos\":{\"Result\":8},\"ViewType\":{\"Result\":\"vital\"}}");

}

