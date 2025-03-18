#include <library/cpp/vowpalwabbit/readable_repr.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/system/tempfile.h>
#include <util/stream/file.h>

Y_UNIT_TEST_SUITE(TVowpalWabbitHashReprTest) {
    Y_UNIT_TEST(TestHashReprApplicaton) {
        TString readableModelResource = NResource::Find("/library/cpp/vowpalwabbit/ut/vw-model-readable");
        Y_VERIFY(!readableModelResource.Empty());
        TTempFile readableModelFile("vw-model-readable");
        TFixedBufferFileOutput out(readableModelFile.Name());
        out.Write(readableModelResource.Data());
        out.Finish();

        const auto model = NVowpalWabbit::LoadReadableModel(readableModelFile.Name());
        const TVector<TString> sampleOne = {"a", "b", "c"};
        const auto resOne = NVowpalWabbit::ApplyReadableModel(model, sampleOne, 3);
        UNIT_ASSERT_EQUAL(resOne, 7.);

        const TVector<TString> sampleTwo = {"a", "a", "a"};
        const auto resTwo = NVowpalWabbit::ApplyReadableModel(model, sampleTwo, 3);
        UNIT_ASSERT_EQUAL(resTwo, 124.);
    }
}
