#pragma once

#include <kernel/web_factors_info/factors_gen.h>
#include <kernel/web_meta_factors_info/factors_gen.h>
#include <kernel/factor_storage/factor_storage.h>

#include <search/rapid_clicks/factors_gen.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>


namespace NPrsLog {

    struct TWebDocument {
        TString DocId;
        TString Url;
        TString OmniTitle;
        TString SerializedFactors;
        float L2Relevance;

        TVector<float> WebFeatures;
        TVector<float> WebMetaFeatures;
        TVector<float> RapidClicksFeatures;
    };

    struct TWebData {
        TVector<NSliceWebProduction::EFactorId> WebFeaturesIds;
        TVector<NSliceWebMeta::EFactorId> WebMetaFeaturesIds;
        TVector<NRapidClicks::EFactorId> RapidClicksFeaturesIds;
        TVector<TWebDocument> Documents;
    };

} // NPrsLog
