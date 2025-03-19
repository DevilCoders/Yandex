#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

#include <util/stream/file.h>

#include <util/memory/blob.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>

using namespace NNeuralNetApplier;

namespace {
    const TString MODEL_FILENAME{"dssm_sample.bin"};

    void IncreaseVersion(TModel& model, size_t versionIncrease, bool supportPrevious) {
        for (size_t i = 0; i < versionIncrease; ++i) {
            model.IncreaseVersion(supportPrevious);
        }
    }
}

Y_UNIT_TEST_SUITE(VersionTest) {
    Y_UNIT_TEST(IncreaseVersionSupportPrevious) {
        auto model = NNeuralNetApplier::TModel{};
        auto blob = TBlob{};
        {
            TFileInput inputFile(MODEL_FILENAME);
            blob = TBlob::FromStream(inputFile);
            model.Load(blob);
        }
        model.Init();

        const ui32 initialVersion = model.GetVersion();
        constexpr size_t versionIncrease = 10;

        IncreaseVersion(model, versionIncrease, true);
        UNIT_ASSERT_EQUAL(model.GetVersion(), initialVersion + versionIncrease);
        UNIT_ASSERT_EQUAL(TVersionRange(initialVersion, initialVersion + versionIncrease), model.GetSupportedVersions());

    } // IncreaseVersionSupportPrevious

    Y_UNIT_TEST(IncreaseVersionNotSupportPrevious) {
        auto model = NNeuralNetApplier::TModel{};
        auto blob = TBlob{};
        {
            TFileInput inputFile(MODEL_FILENAME);
            blob = TBlob::FromStream(inputFile);
            model.Load(blob);
        }
        model.Init();

        const ui32 initialVersion = model.GetVersion();
        constexpr size_t versionIncrease = 10;

        IncreaseVersion(model, versionIncrease, false);
        UNIT_ASSERT_EQUAL(model.GetVersion(), initialVersion + versionIncrease);
        UNIT_ASSERT_EQUAL(TVersionRange(model.GetVersion()), model.GetSupportedVersions());

    } // IncreaseVersionNotSupportPrevious

    Y_UNIT_TEST(IncreaseVersionMixed) {
        auto model = NNeuralNetApplier::TModel{};
        auto blob = TBlob{};
        {
            TFileInput inputFile(MODEL_FILENAME);
            blob = TBlob::FromStream(inputFile);
            model.Load(blob);
        }
        model.Init();

        constexpr size_t versionIncrease = 5;
        const ui32 initialVersion = model.GetVersion();
        IncreaseVersion(model, versionIncrease, false);
        IncreaseVersion(model, versionIncrease, true);

        UNIT_ASSERT_EQUAL(model.GetVersion(), initialVersion + 2 * versionIncrease);
        UNIT_ASSERT_EQUAL(TVersionRange(initialVersion + versionIncrease, initialVersion + 2 * versionIncrease), model.GetSupportedVersions());

    } // IncreaseVersionMixed

    Y_UNIT_TEST(SaveLoad) {
        auto model = NNeuralNetApplier::TModel{};
        auto blob = TBlob{};
        {
            TFileInput inputFile(MODEL_FILENAME);
            blob = TBlob::FromStream(inputFile);
            model.Load(blob);
        }
        model.Init();

        constexpr size_t versionIncrease = 10;
        IncreaseVersion(model, versionIncrease, true);

        {
            TStringStream stream;
            model.Save(&stream);
            NNeuralNetApplier::TModel loaded;
            loaded.Load(TBlob::FromStream(stream));
            UNIT_ASSERT_EQUAL(loaded.GetVersion(), model.GetVersion());
            UNIT_ASSERT_EQUAL(loaded.GetSupportedVersions(), model.GetSupportedVersions());
        }

        IncreaseVersion(model, versionIncrease, false);

        {
            TStringStream stream;
            model.Save(&stream);
            NNeuralNetApplier::TModel loaded;
            loaded.Load(TBlob::FromStream(stream));
            UNIT_ASSERT_EQUAL(loaded.GetVersion(), model.GetVersion());
            UNIT_ASSERT_EQUAL(loaded.GetSupportedVersions(), model.GetSupportedVersions());
        }

    } // SaveLoad

    Y_UNIT_TEST(CopyModel) {
        auto model = NNeuralNetApplier::TModel{};
        auto blob = TBlob{};
        {
            TFileInput inputFile(MODEL_FILENAME);
            blob = TBlob::FromStream(inputFile);
            model.Load(blob);
        }
        model.Init();

        constexpr size_t versionIncrease = 10;
        IncreaseVersion(model, versionIncrease, true);

        {
            NNeuralNetApplier::TModel copy(model);
            UNIT_ASSERT_EQUAL(copy.GetVersion(), model.GetVersion());
            UNIT_ASSERT_EQUAL(copy.GetSupportedVersions(), model.GetSupportedVersions());
        }

        IncreaseVersion(model, versionIncrease, false);

        {
            NNeuralNetApplier::TModel copy(model);
            UNIT_ASSERT_EQUAL(copy.GetVersion(), model.GetVersion());
            UNIT_ASSERT_EQUAL(copy.GetSupportedVersions(), model.GetSupportedVersions());
        }

    } // CopyModel

    Y_UNIT_TEST(SetSupportedVersions) {
        auto model = NNeuralNetApplier::TModel{};
        auto blob = TBlob{};
        {
            TFileInput inputFile(MODEL_FILENAME);
            blob = TBlob::FromStream(inputFile);
            model.Load(blob);
        }
        model.Init();

        IncreaseVersion(model, 3, true);

        auto versions = TVersionRange(1, 10);
        model.SetSupportedVersions(versions);
        UNIT_ASSERT_EQUAL(model.GetVersion(), versions.GetEnd());
        UNIT_ASSERT_EQUAL(model.GetSupportedVersions(), versions);


    } // CopyModel
} // VersionTest
