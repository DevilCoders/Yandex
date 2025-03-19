#pragma once

#include "common.h"

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>

#include <util/stream/file.h>

namespace NWordFeatures {

    class TSurfFeaturesCalcer {
    private:
        class TImpl;
        THolder<TImpl> Impl;
    public:
        TSurfFeaturesCalcer(const TString& indexFile);

        TSurfFeaturesCalcer(IInputStream& in);

        bool Find(const TStringBuf& word, TSurfFeatures& features) const;

        const TSurfFeatures& GetMean() const;

        // Returns 'true' if found, 'false' if took mean one.
        bool FindOrMean(const TStringBuf& word, TSurfFeatures& features) const;

        static const TVector<TString>& GetQueryFeatureNames();
        static ui32 GetQueryFeaturesCount();

        static ui32 GetFeatureOffset(ESurfFeatureType featureType, ESurfStatisticType statisticType);

        // SURF_STAT_TOTAL * NUM_FEATURES features
        // Expression (@maxFeatures % SURF_STAT_TOTAL == 0) should be true.
        void FillQueryFeatures(const TStringBuf& request, TVector<float>& features) const;
        void FillQueryFeatures(const TStringBuf& request, float* features, size_t maxFeatures) const;

        ~TSurfFeaturesCalcer();
    };

}; //NWordFeatures
