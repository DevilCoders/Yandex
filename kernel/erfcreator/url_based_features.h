#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>

namespace NUrlRegNavClassifier {
    class TUrlTokenizer;
    class THierarcicalUrlGeoClassifier;
} // namespace NUrlRegNavClassifier

namespace NErfCreator {
    bool IsNotCgi(TStringBuf url);

    struct TUrlRegionFeatures {
        ui32 RegionFromUrl;
        ui8 RegionFromUrlProbability;
    };

    class TUrlRegionFeaturesCalculator {
        public:
            TUrlRegionFeaturesCalculator(const TString& rnsModelsTsvPath);
            ~TUrlRegionFeaturesCalculator();

            TUrlRegionFeatures GetUrlRegionFeatures(const TString& url) const;

        private:
            THolder<NUrlRegNavClassifier::TUrlTokenizer> UrlTokenizer;
            THolder<NUrlRegNavClassifier::THierarcicalUrlGeoClassifier> Classifier;
    };
}
