#include <kernel/matrixnet/mn_dynamic.h>
#include <kernel/matrixnet/mn_multi_categ.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/buffer.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/stream/mem.h>
#include <util/stream/buffer.h>

using namespace NMatrixnet;

Y_UNIT_TEST_SUITE(TMnMultiCategTest) {
    namespace {
        extern "C" {
        extern const unsigned char TestModels[];
        extern const ui32 TestModelsSize;
        };
        TVector<TString> TestModelNames = {"/Ru.info", "/RuFreshExtRelev.info", "/ip25.info", "/quark_slices.info"};

        void SetModel(TMnSseDynamic& model, const TString& name) {
            TArchiveReader archive(TBlob::NoCopy(TestModels, TestModelsSize));
            TBlob modelBlob = archive.ObjectBlobByKey(name);
            TMemoryInput input(modelBlob.AsCharPtr(), modelBlob.Size());
            ::Load(&input, model);
        }
    }

    Y_UNIT_TEST(TestMultiCategModel) {
        for (const auto& modelName : TestModelNames) {
            TBuffer buf;
            {
                TAtomicSharedPtr<TMnSseDynamic> modelHolder = new TMnSseDynamic;
                SetModel(*modelHolder, modelName);
                TVector<TMnSseDynamicPtr> tmpVec = {modelHolder};
                TVector<double> tmpClassVec = {1.0};
                TMnMultiCateg multiCategModelSolid(std::move(tmpVec), std::move(tmpClassVec));
                multiCategModelSolid.SetInfo("testSaveInit", "true");
                UNIT_ASSERT_STRINGS_EQUAL(multiCategModelSolid.GetInfo()->at("testSaveInit"), "true");
                TBufferOutput out(buf);
                multiCategModelSolid.Save(&out);
            }
            // Test deserialization
            {
                TMnMultiCateg multiCategSolid;
                TBufferInput inp(buf);
                multiCategSolid.Load(&inp);
                UNIT_ASSERT_STRINGS_EQUAL(multiCategSolid.GetInfo()->at("testSaveInit"), "true");
            }
            // Test thin initialization
            {
                TMnMultiCateg multiCategThin(buf.Data(), buf.Size());
                UNIT_ASSERT_STRINGS_EQUAL(multiCategThin.GetInfo()->at("testSaveInit"), "true");
            }
        }
    }
}
