#include "util.h"

#include <library/cpp/string_utils/url/url.h>
#include <util/system/yassert.h>

namespace NEthos {

namespace {

void AddHashes(const TVector<ui64>& hashes, const TIndexWeighter& indexWeighter, TVector<TIndexedFloatFeature>& hashWeights) {
    for (size_t position = 0; position < hashes.size(); ++position) {
        ui64 hash = hashes[position];
        double positionWeight = indexWeighter(position);
        if (positionWeight < 0.1) {
            break;
        }
        hashWeights.push_back(TIndexedFloatFeature(hash, positionWeight));
    }
}

double TransformWordWeight(double value, EWordWeightsTransformation wwTransformation) {
    switch (wwTransformation) {
        case EWordWeightsTransformation::NO_TRANSFORMATION:
            return value;
        case EWordWeightsTransformation::LOG:
            return log(2. + value);
        case EWordWeightsTransformation::SQUARED_ROOT:
            return sqrt(value);
        case EWordWeightsTransformation::CONSTANT:
            return 1.;
    }
    Y_ASSERT(0);
    return 0.;
}

void FinalizeFeatures(TVector<TIndexedFloatFeature>& features, const TTextClassifierModelOptions& options) {
    if (features.empty()) {
        return;
    }

    Sort(features.begin(), features.end());

    TIndexedFloatFeature* write = features.begin();
    TIndexedFloatFeature* read = write + 1;

    for (; read != features.end(); ++read) {
        if (read->Index == write->Index) {
            write->Value += read->Value;
            continue;
        }
        write->Value = TransformWordWeight(write->Value, options.WordWeightsTransformation);

        ++write;
        *write = *read;
    }
    write->Value = TransformWordWeight(write->Value, options.WordWeightsTransformation);

    features.erase(write + 1, features.end());
}

}

TFloatFeatureVector FeatureVectorFromHashes(const TVector<ui64>& unigramHashes, const TTextClassifierModelOptions& options) {
    TVector<TIndexedFloatFeature> features;
    AddHashes(unigramHashes, options.IndexWeighter, features);

    if (options.UseBigrams) {
        TVector<ui64> bigramHashes;
        for (size_t i = 0; i + 1 < unigramHashes.size(); ++i) {
            bigramHashes.push_back(CombineHashes<ui64>(unigramHashes[i], unigramHashes[i + 1]));
        }
        AddHashes(bigramHashes, options.IndexWeighter, features);
    }

    FinalizeFeatures(features, options);

    return features;
}

TFloatFeatureVector FeatureVectorFromDocument(const TDocument& document, const TTextClassifierModelOptions& options) {
    return FeatureVectorFromHashes(document.UnigramHashes, options);
}

TBinaryLabelFloatFeatureVectors FeatureVectorsFromBinaryLabelDocuments(TAnyConstIterator<TBinaryLabelDocument> begin,
                                                                       TAnyConstIterator<TBinaryLabelDocument> end,
                                                                       const TTextClassifierModelOptions& options)
{
    TBinaryLabelFloatFeatureVectors items;

    for (; begin != end; ++begin) {
        const TBinaryLabelDocument& document = *begin;
        TFloatFeatureVector features = FeatureVectorFromDocument(document, options);
        items.push_back(TBinaryLabelFloatFeatureVector(document.Index, std::move(features), document.Label, document.Weight));
    }

    return items;
}

TMultiBinaryLabelFloatFeatureVectors FeatureVectorsFromMultiLabelDocuments(TAnyConstIterator<TMultiLabelDocument> begin,
                                                                           TAnyConstIterator<TMultiLabelDocument> end,
                                                                           const TVector<TString>& allLabels,
                                                                           const TTextClassifierModelOptions& options)
{
    TMultiBinaryLabelFloatFeatureVectors items;

    for (; begin != end; ++begin) {
        const TMultiLabelDocument& document = *begin;

        TFloatFeatureVector features = FeatureVectorFromDocument(document, options);
        TVector<ui32> positiveLabelIndexes;

        for (ui32 labelNumber = 0; labelNumber < allLabels.size(); ++labelNumber) {
            if (IsIn(document.Labels, allLabels[labelNumber])) {
                positiveLabelIndexes.push_back(labelNumber);
            }
        }

        TMultiBinaryLabelFloatFeatureVector item(document.Index, std::move(features),
                                                 std::move(positiveLabelIndexes), document.Weight);

        items.push_back(item);
    }

    return items;
}

THashMap<ui32, TString> HostsFromIdsAsUrls(const TBinaryLabelDocuments& documents) {
    THashMap<ui32, TString> result(documents.size());

    for (const TBinaryLabelDocument& document : documents) {
        if (!document.Id.empty()) {
            result[document.Index] = TString(GetOnlyHost(document.Id));
        }
    }

    return result;
}

}
