#include "nn_over_dssm_doc_features.h"
#include <extsearch/images/kernel/knn/t2t_knn.h>
#include <extsearch/images/kernel/nnoptions/i2toptions.h>
#include <kernel/images_nn_over_dssm_doc_features/factors/factors_gen.h>
#include <kernel/images_nn_over_dssm_doc_features/fill_factors/fill_factors.h>
#include <util/generic/array_ref.h>
namespace {

    void FillI2TFeatures(const NImages::NIndex::TImgDlErfPB& imgDlErf, const ui32 i2tVersion, TVector<float>& features) {
        for (auto& i2tFeatures : imgDlErf.GetI2tImgFeatures()) {
            if (i2tFeatures.GetVersion() == i2tVersion) {
                const i8* rawFeatures = reinterpret_cast<const i8*>(i2tFeatures.GetFeatures().data());
                const size_t embedLength = i2tFeatures.GetFeatures().size();
                Y_VERIFY(NImages::I2TDimension == embedLength,
                         "bad i2t features size got: %lu, expected: %lu", embedLength, NImages::I2TDimension);
                for (size_t i = 0; i < embedLength; ++i) {
                    features[i] = static_cast<float>(rawFeatures[i]) / Max<i8>();
                }
                return;
            }
        }
    }

    void FillT2TFeatures(const NImages::TText2textPB& t2tPb, TVector<float>& features, size_t offset) {
        const TProtoStringType& serialized = t2tPb.GetImgText2TextFeaturesV2();
        const i8* byteFeatures = reinterpret_cast<const i8*>(serialized.data());
        Y_VERIFY(serialized.size() == NImages::NKNN::T2TV2Dimension, "Unexpected t2t embedding size: %lu, expected size: %lu", serialized.size(), NImages::NKNN::T2TV2Dimension);
        for (size_t indexGet = 0, indexAdd = offset; indexGet < serialized.size(); ++indexGet, ++indexAdd) {
            features[indexAdd] = static_cast<float>(byteFeatures[indexGet]) / Max<i8>();
        }
    }

}

TVector<float> NImages::FillFeatures(const NImages::NIndex::TImgDlErfPB& imgDlErf,
                                     const ui32 i2tVersion,
                                     const NImages::NIndex::TOmniIndexDataPB& omni,
                                     const NImages::NIndex::TErfPB& erf,
                                     const TInstant& buildTime,
                                     const NImages::TText2textPB& t2tPb)
{
    TVector<float> features(NImages::I2TDimension + NImagesNnOverDssmDocFeatures::FI_FACTOR_COUNT + NImages::NKNN::T2TV2Dimension, 0.0f);
    // i2t
    FillI2TFeatures(imgDlErf, i2tVersion, features);
    // static
    TFactorDomain domain(EFactorSlice::IMAGES_NN_OVER_DSSM_DOC_FEATURES);
    TFactorStorage factorStorage(domain);
    TFactorView view = factorStorage.CreateViewFor(EFactorSlice::IMAGES_NN_OVER_DSSM_DOC_FEATURES);
    view.Clear();
    NImagesNnOverDssmDocFeatures::FillImagesNnOverDssmDocFeatures(omni, buildTime, erf, view);
    Y_VERIFY(view.Size() == static_cast<size_t>(NImagesNnOverDssmDocFeatures::FI_FACTOR_COUNT));
    for (size_t i = 0; i < view.Size(); ++i) {
        features[NImages::I2TDimension + i] = view[i];
    }
    // t2t
    FillT2TFeatures(t2tPb, features, NImages::I2TDimension + NImagesNnOverDssmDocFeatures::FI_FACTOR_COUNT);
    return features;
}
