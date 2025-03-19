#pragma once

#include "document.h"
#include "options.h"

namespace NEthos {

TFloatFeatureVector FeatureVectorFromHashes(const TVector<ui64>& unigramHashes, const TTextClassifierModelOptions& options);

TFloatFeatureVector FeatureVectorFromDocument(const TDocument& document,
                                              const TTextClassifierModelOptions& options);

TBinaryLabelFloatFeatureVectors FeatureVectorsFromBinaryLabelDocuments(TAnyConstIterator<TBinaryLabelDocument> begin,
                                                                       TAnyConstIterator<TBinaryLabelDocument> end,
                                                                       const TTextClassifierModelOptions& options);

TMultiBinaryLabelFloatFeatureVectors FeatureVectorsFromMultiLabelDocuments(TAnyConstIterator<TMultiLabelDocument> begin,
                                                                           TAnyConstIterator<TMultiLabelDocument> end,
                                                                           const TVector<TString>& allLabels,
                                                                           const TTextClassifierModelOptions& options);

THashMap<ui32, TString> HostsFromIdsAsUrls(const TBinaryLabelDocuments& documents);

}
