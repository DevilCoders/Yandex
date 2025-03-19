#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

#include <util/stream/file.h>

#include <util/memory/blob.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace {
    const TString MODEL_FILENAME{"dssm_sample.bin"};
}

using namespace NNeuralNetApplier;

Y_UNIT_TEST_SUITE(MetaDataTest) {
    Y_UNIT_TEST(SaveLoad) {
        auto model = NNeuralNetApplier::TModel{};
        auto blob = TBlob{};
        {
            TFileInput inputFile(MODEL_FILENAME);
            blob = TBlob::FromStream(inputFile);
            model.Load(blob);
        }
        model.Init();
        model.SetMetaData("meta data");

        {
            TStringStream stream;
            model.Save(&stream);
            NNeuralNetApplier::TModel loaded;
            loaded.Load(TBlob::FromStream(stream));
            UNIT_ASSERT_EQUAL(loaded.GetMetaData(), "meta data");
        }
    } // SaveLoad
}
