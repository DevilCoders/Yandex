#include "dcg_boost.h"

void CalcCategRelevBoosts(
        THashMap<NExtendedMx::NMultiFeatureSoftmax::TCateg, double>& categRelevDCGBoosts,
        NExtendedMx::TDebug& dbgLog,
        const double wizRelev,
        const TVector<double>& docRelevs,
        size_t& bestPosByRelevance,
        const size_t dcgDocCount,
        const THashMap<NExtendedMx::NMultiFeatureSoftmax::TCateg, NExtendedMx::NMultiFeatureSoftmax::TOrganicPosProbs>& organicPosProbs,
        const NExtendedMx::NMultiFeatureSoftmax::TMultiFeatureParams& multiFeatureParams,
        const TVector<NExtendedMx::NMultiFeatureSoftmax::TCateg>& upperCategs,
        const int lossCategShift,
        const THashMap<size_t, std::pair<double, double>>& viewTypeShift,
        const size_t viewTypeFeatureIndex
)
{

    TVector<double> dcgImpacts(dcgDocCount, 0.);
    TVector<double> organicDCGImpacts(dcgDocCount, 0.);
    TVector<double> wizardDCGImpacts(dcgDocCount, 0.);
    TVector<double> wizDCGWeight(dcgDocCount, 0.);

    double organicDcgImpact = 0;
    double wizardDcgImpact = 0;

    double bestWizImpact = 0;
    double totalDCGWeight = 0;
    for (size_t lowerPos = dcgDocCount; lowerPos > 0; --lowerPos) {
        const int upperPos = lowerPos - 1;
        const double upperPosDCGWeight = 1. / (upperPos + 1);
        const double lowerPosDCGWeight = (lowerPos < dcgDocCount) ? 1. / (lowerPos + 1) : 0.;
        double weight = (upperPosDCGWeight - lowerPosDCGWeight);
        organicDcgImpact += -weight * docRelevs[upperPos]; // move document from upperPos to lowerPos
        wizardDcgImpact += weight * wizRelev; // move wizard from lowerPos to upperPos
        totalDCGWeight += weight;
        organicDCGImpacts[upperPos] = organicDcgImpact;
        wizardDCGImpacts[upperPos] = wizardDcgImpact;
        wizDCGWeight[upperPos] = totalDCGWeight;
        double dcgImpact = organicDcgImpact + wizardDcgImpact;
        dcgImpacts[upperPos] = dcgImpact;
        if (dcgImpact > bestWizImpact) {
            bestWizImpact = dcgImpact;
            bestPosByRelevance = upperPos;
        }
    }

    if (dbgLog.IsEnabled()) {
        dbgLog << "Document relevances:\n";
        dbgLog << JoinVectorIntoString(docRelevs, "\t") << '\n';
        dbgLog << "Wizard relevance: " << wizRelev << '\n';
        dbgLog << "DCG impacts of organic positions:\n";
        dbgLog << JoinVectorIntoString(dcgImpacts, "\t") << '\n';
        dbgLog << "Best pos by relevance: " << bestPosByRelevance << ", best wiz impact: " << bestWizImpact << '\n';
    }

    categRelevDCGBoosts.clear();
    for (const auto& categ : upperCategs) {
        if (categ >= lossCategShift) {
            continue;  // iterate real categories like in CalculateUpper
        }
        const NExtendedMx::NMultiFeatureSoftmax::TOrganicPosProbs& posProbs = organicPosProbs.at(categ);
        double wizPosDCGImpactExpectation = 0.;
        std::pair<double, double> shift;
        bool applyShift = false;
        if (!viewTypeShift.empty() && viewTypeShift.contains(multiFeatureParams.GetFeatureCateg(categ, viewTypeFeatureIndex))) {
            applyShift = true;
            shift = viewTypeShift.at(multiFeatureParams.GetFeatureCateg(categ, viewTypeFeatureIndex));
        }
        for (size_t organicPos = 0; organicPos < dcgDocCount; ++organicPos) {
            double impact;
            if (applyShift) {
                impact = organicDCGImpacts[organicPos] + shift.first * wizardDCGImpacts[organicPos] + shift.second * wizDCGWeight[organicPos];
            } else {
                impact = organicDCGImpacts[organicPos] + wizardDCGImpacts[organicPos];
            }
            wizPosDCGImpactExpectation += posProbs.MainPosProbs.at(organicPos) * impact;
        }
        if (posProbs.RightProb > 0) {
            // impact of right column placement is same as zero position, but without shifting organic documents down
            double impact;
            if (applyShift) {
                impact = shift.first * wizardDCGImpacts[0] + shift.second * wizDCGWeight[0];
            } else {
                impact = wizardDCGImpacts[0];
            }
            wizPosDCGImpactExpectation += posProbs.RightProb * impact;
        }
        if (posProbs.WizplaceProb > 0) {
            // impact of wizplace column placement is same as zero position
            double impact;
            if (applyShift) {
                impact = organicDCGImpacts[0] + shift.first * wizardDCGImpacts[0] + shift.second * wizDCGWeight[0];
            } else {
                impact = organicDCGImpacts[0] + wizardDCGImpacts[0];
            }
            wizPosDCGImpactExpectation += posProbs.WizplaceProb * impact;
        }
        categRelevDCGBoosts[categ] = wizPosDCGImpactExpectation;
    }
}
