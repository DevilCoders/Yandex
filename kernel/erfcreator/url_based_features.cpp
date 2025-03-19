#include "url_based_features.h"

#include <yweb/robot/urlgeo_ml/url_regnav_lin_classifier.h>

#include <library/cpp/packedtypes/packedfloat.h>

#include <library/cpp/string_utils/url/url.h>

namespace NErfCreator {

    bool IsNotCgi(TStringBuf url) {
        const TStringBuf path = GetPathAndQuery(url);
        return !path.Contains('?');
    }

    TUrlRegionFeaturesCalculator::TUrlRegionFeaturesCalculator(const TString& rnsModelsTsvPath)
        : UrlTokenizer(new NUrlRegNavClassifier::TUrlTokenizer)
        , Classifier(new NUrlRegNavClassifier::THierarcicalUrlGeoClassifier)
    {
        Classifier->ReadModels(rnsModelsTsvPath);
    }

    TUrlRegionFeaturesCalculator::~TUrlRegionFeaturesCalculator()
    {}

    TUrlRegionFeatures TUrlRegionFeaturesCalculator::GetUrlRegionFeatures(const TString& url) const {
        TSet<TString> tokens;
        UrlTokenizer->Tokenize(TString(url), tokens);

        auto classifyResult = Classifier->Classify(tokens);

        TUrlRegionFeatures result;
        result.RegionFromUrl = classifyResult.Region;
        result.RegionFromUrlProbability = Float2Frac<ui8>(classifyResult.Probability);
        return result;
    }

} // namespace NErfCreator
