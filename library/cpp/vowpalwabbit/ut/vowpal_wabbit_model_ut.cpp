#include <library/cpp/vowpalwabbit/vowpal_wabbit_model.h>
#include <library/cpp/resource/resource.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/system/tempfile.h>
#include <util/stream/file.h>

Y_UNIT_TEST_SUITE(TVowpalWabbitModelTest) {
    Y_UNIT_TEST(TestParseVwModel811) {
        TString modelData = NResource::Find("/library/cpp/vowpalwabbit/ut/vw-model-811");
        Y_VERIFY(!modelData.Empty());
        TTempFile modelTxtFile("vw-model-811.tmp");
        TTempFile modelFile("vw-model-811.bin");
        TFixedBufferFileOutput out(modelTxtFile.Name());
        out.Write(modelData);
        out.Finish();

        TVowpalWabbitModel::ConvertReadableModel(modelTxtFile.Name(), modelFile.Name());
        TVowpalWabbitModel model(modelFile.Name());
        UNIT_ASSERT_EQUAL(model.GetBits(), 23);
        const TBlob& blob = model.GetBlob();
        UNIT_ASSERT_EQUAL(blob.Size(), model.GetWeightsSize() * sizeof(float));

        UNIT_ASSERT_EQUAL(model.GetWeights()[0], -0.000732f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[1], -0.000251f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[25], 0.000419f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[35], -0.000280f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[36], 0.0f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[37], 0.0f);
    }

    Y_UNIT_TEST(TestParseVwModel820) {
        TString modelData = NResource::Find("/library/cpp/vowpalwabbit/ut/vw-model-820");
        Y_VERIFY(!modelData.Empty());
        TTempFile modelTxtFile("vw-model-820.tmp");
        TTempFile modelFile("vw-model-820.bin");
        TFixedBufferFileOutput out(modelTxtFile.Name());
        out.Write(modelData);
        out.Finish();

        TVowpalWabbitModel::ConvertReadableModel(modelTxtFile.Name(), modelFile.Name());
        TVowpalWabbitModel model(modelFile.Name());
        UNIT_ASSERT_EQUAL(model.GetBits(), 22);
        const TBlob& blob = model.GetBlob();
        UNIT_ASSERT_EQUAL(blob.Size(), model.GetWeightsSize() * sizeof(float));

        UNIT_ASSERT_EQUAL(model.GetWeights()[0], -0.00314021f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[1], -0.000628043f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[29], 0.00314021f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[36], -0.00125609f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[37], 0.0f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[38], 0.0f);
    }

    Y_UNIT_TEST(TestParseVwModel833) {
        TString modelData = NResource::Find("/library/cpp/vowpalwabbit/ut/vw-model-833");
        Y_VERIFY(!modelData.Empty());
        TTempFile modelTxtFile("vw-model-833.tmp");
        TTempFile modelFile("vw-model-833.bin");
        TFixedBufferFileOutput out(modelTxtFile.Name());
        out.Write(modelData);
        out.Finish();

        TVowpalWabbitModel::ConvertReadableModel(modelTxtFile.Name(), modelFile.Name());
        TVowpalWabbitModel model(modelFile.Name());
        UNIT_ASSERT_EQUAL(model.GetBits(), 22);
        const TBlob& blob = model.GetBlob();
        UNIT_ASSERT_EQUAL(blob.Size(), model.GetWeightsSize() * sizeof(float));

        UNIT_ASSERT_EQUAL(model.GetWeights()[0], 0.0585146f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[1], 0.0f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[2], -0.121957f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[21], -0.119943f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[22], 0.113105f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[70], 0.0f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[71], -0.121992f);
        UNIT_ASSERT_EQUAL(model.GetWeights()[72], 0.0f);
    }
}
