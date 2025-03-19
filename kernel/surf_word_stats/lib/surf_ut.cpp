#include "builder.h"
#include "reader.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/vector.h>
#include <util/folder/tempdir.h>

class TSurfBuildAndReadTest : public TTestBase {
    UNIT_TEST_SUITE(TSurfBuildAndReadTest);
        UNIT_TEST(TestSurf)
    UNIT_TEST_SUITE_END();

    void IndexEqual(float x, float y)
    {
        UNIT_ASSERT(fabs(x - y) < 1e-4);
    }

    void CheckExpected(const NWordFeatures::TSurfFeaturesCalcer& calcer, const TString& request, const TVector<float>& expected)
    {
        TVector<float> gained;
        calcer.FillQueryFeatures(request, gained);

        // Cerr << "{" << JoinStrings(gained.begin(), gained.end(), ", ") << "}" << Endl;
        // Y_UNUSED(expected);

        UNIT_ASSERT_VALUES_EQUAL(gained.size(), expected.size());

        for (size_t i = 0; i < expected.size(); ++i) {
            IndexEqual(gained[i], expected[i]);
        }
    }
private:
    void TestSurf()
    {
        TTempDir tmpDirRaw;
        const auto tmpDir = tmpDirRaw.Name();

        const auto srcFile = tmpDir + "/src";
        const auto indexPath = tmpDir + "/index";

        // Creating input file.
        {
            TFixedBufferFileOutput fOut(srcFile);

            fOut << "windows\t0.507\t0.2582\t0.042\t0.0282\t0.3314\t0.2217\t0.0645\t0.0606\t0.327\t0.2176\t0.0665\t0.0617\t0.2131\t0.0807\t0.0311\t0.0283\t0.0283\t0.0018" << Endl;
            fOut << "10\t0.5076\t0.2591\t0.0422\t0.0282\t0.2898\t0.1911\t0.0596\t0.0475\t0.2849\t0.1865\t0.0621\t0.05\t0.1772\t0.0782\t0.0315\t0.0282\t0.0248\t0.001" << Endl;
            fOut << "apple\t0.5104\t0.2623\t0.0518\t0.0565\t0.3177\t0.2145\t0.0729\t0.0793\t0.3109\t0.2084\t0.0766\t0.0814\t0.2049\t0.0897\t0.039\t0.0342\t0.0298\t0.0006" << Endl;
        }

        // Building index.
        {
            NWordFeatures::BuildSurfIndex(srcFile, indexPath);
        }

        // Debugging requests.
        {
            NWordFeatures::TSurfFeaturesCalcer reader(indexPath);

            TVector<float> expected = {0.5073f, 0.507f, 0.5076f, 0.25855f, 0.2581f, 0.259f, 0.042f, 0.0419f, 0.0421f, 0.0282f, 0.0282f, 0.0282f, 0.31055f, 0.2897f, 0.3314f, 0.20635f, 0.1911f, 0.2216f, 0.06195f, 0.0595f, 0.0644f, 0.054f, 0.0474f, 0.0606f, 0.3059f, 0.2849f, 0.3269f, 0.202f, 0.1864f, 0.2176f, 0.0643f, 0.0621f, 0.0665f, 0.05585f, 0.05f, 0.0617f, 0.19515f, 0.1772f, 0.2131f, 0.0794f, 0.0781f, 0.0807f, 0.03125f, 0.031f, 0.0315f, 0.02825f, 0.0282f, 0.0283f, 0.02655f, 0.0248f, 0.0283f, 0.00135f, 0.001f, 0.0017f};
            CheckExpected(reader, "windows 10", expected);
            expected = {0.5073f, 0.507f, 0.5076f, 0.25855f, 0.2581f, 0.259f, 0.042f, 0.0419f, 0.0421f, 0.0282f, 0.0282f, 0.0282f, 0.31055f, 0.2897f, 0.3314f, 0.20635f, 0.1911f, 0.2216f, 0.06195f, 0.0595f, 0.0644f, 0.054f, 0.0474f, 0.0606f, 0.3059f, 0.2849f, 0.3269f, 0.202f, 0.1864f, 0.2176f, 0.0643f, 0.0621f, 0.0665f, 0.05585f, 0.05f, 0.0617f, 0.19515f, 0.1772f, 0.2131f, 0.0794f, 0.0781f, 0.0807f, 0.03125f, 0.031f, 0.0315f, 0.02825f, 0.0282f, 0.0283f, 0.02655f, 0.0248f, 0.0283f, 0.00135f, 0.001f, 0.0017f};
            CheckExpected(reader, "10 windows", expected);
            expected = {0.5081f, 0.507f, 0.5103f, 0.259533f, 0.2581f, 0.2623f, 0.0452333f, 0.0419f, 0.0518f, 0.0376f, 0.0282f, 0.0564f, 0.3268f, 0.3176f, 0.3314f, 0.219233f, 0.2144f, 0.2216f, 0.0672333f, 0.0644f, 0.0728f, 0.0668333f, 0.0606f, 0.0793f, 0.3216f, 0.3109f, 0.3269f, 0.2145f, 0.2083f, 0.2176f, 0.0698667f, 0.0665f, 0.0766f, 0.0682333f, 0.0617f, 0.0813f, 0.210333f, 0.2048f, 0.2131f, 0.0836667f, 0.0807f, 0.0896f, 0.0337f, 0.031f, 0.039f, 0.0302667f, 0.0283f, 0.0342f, 0.0287667f, 0.0283f, 0.0297f, 0.00136667f, 0.0006f, 0.0017f};
            CheckExpected(reader, "windows vs apple", expected);
            expected = {0.507f, 1.0f, 0.0f, 0.2582f, 1.0f, 0.0f, 0.042f, 1.0f, 0.0f, 0.0282f, 1.0f, 0.0f, 0.3314f, 1.0f, 0.0f, 0.2217f, 1.0f, 0.0f, 0.0645f, 1.0f, 0.0f, 0.0606f, 1.0f, 0.0f, 0.327f, 1.0f, 0.0f, 0.2176f, 1.0f, 0.0f, 0.0665f, 1.0f, 0.0f, 0.0617f, 1.0f, 0.0f, 0.2131f, 1.0f, 0.0f, 0.0807f, 1.0f, 0.0f, 0.0311f, 1.0f, 0.0f, 0.0283f, 1.0f, 0.0f, 0.0283f, 1.0f, 0.0f, 0.0018f, 1.0f, 0.0f};
            CheckExpected(reader, "vs", expected);
            expected = {0.5103f, 0.5103f, 0.5103f, 0.2623f, 0.2623f, 0.2623f, 0.0518f, 0.0518f, 0.0518f, 0.0564f, 0.0564f, 0.0564f, 0.3176f, 0.3176f, 0.3176f, 0.2144f, 0.2144f, 0.2144f, 0.0728f, 0.0728f, 0.0728f, 0.0793f, 0.0793f, 0.0793f, 0.3109f, 0.3109f, 0.3109f, 0.2083f, 0.2083f, 0.2083f, 0.0766f, 0.0766f, 0.0766f, 0.0813f, 0.0813f, 0.0813f, 0.2048f, 0.2048f, 0.2048f, 0.0896f, 0.0896f, 0.0896f, 0.039f, 0.039f, 0.039f, 0.0342f, 0.0342f, 0.0342f, 0.0297f, 0.0297f, 0.0297f, 0.0006f, 0.0006f, 0.0006f};
            CheckExpected(reader, "apple", expected);
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TSurfBuildAndReadTest);
