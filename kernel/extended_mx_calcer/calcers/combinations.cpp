#include "combinations.h"

#include <library/cpp/scheme/scheme_cast.h>

#include <util/string/builder.h>

#include <algorithm>

namespace NExtendedMx {
namespace NCombinator {

    template <typename TCont>
    static TCombination LoadCombinationFromProto(const TCont& comb) {
        TCombination tmp(comb.Size());
        Copy(comb.begin(), comb.end(), tmp.begin());
        return tmp;
    }

    template <typename TCont>
    static void LoadCombinationFromProto(const TCont& comb, TCombinations& dst) {
        dst.emplace_back(LoadCombinationFromProto(comb));
    }

    NSc::TValue TFeatInfo::ToTValue() const {
        NSc::TValue res;
        res["Name"] = Name;
        res["IsBinary"] = IsBinary;
        return res;
    }

    void TFeatInfo::FromTValue(const NSc::TValue&, const bool) {
        ythrow yexception() << "no need for this function";
    }

    void TCombinator::LoadAvailCombinations() {
        const auto& allowedCombs = Scheme().AllowedCombinations();
        AllCombinations.reserve(allowedCombs.Size());
        for (const auto& comb : allowedCombs) {
            LoadCombinationFromProto(comb, AllCombinations);
        }
        if (IsNoShowUsedInFeats()) {
            LoadCombinationFromProto(Scheme().NoShow(), AllCombinations);
        }
        NoShowCombination = LoadCombinationFromProto(Scheme().NoShow());
    }

    void TCombinator::LoadAvailCombinationValues() {
        for (const auto& comb : AllCombinations) {
            Y_ENSURE(comb.size() == FeaturesOrder.size(), "should be equal size");
            size_t featIdx = 0;
            TVector<TString> vals;
            for (const auto& featValIdx : comb) {
                const auto& featName = FeaturesOrder[featIdx].Name;
                const auto& knownFeatValues = Feature2Idx2Value[featName];
                Y_ENSURE(featValIdx < knownFeatValues.size(), "feature value idx should be less than vector with all feature values" );
                const auto& featValue = knownFeatValues[featValIdx];
                vals.push_back(featValue);
                ++featIdx;
            }
            AllCombinationValues.emplace_back(std::move(vals));
        }
    }

    void TCombinator::LoadAndEnumerateFeats() {
        size_t featIdx = 0;
        FeaturesOrder.reserve(Scheme().Features().Size());
        for (const auto& feature : Scheme().Features()) {
            const TString featName = TString{feature.Name().Get()};
            size_t valIdx = 0;
            for (const auto& value : feature.Values()) {
                const TString valStr = value.Get()->ForceString();
                Feature2Value2Idx[featName][valStr] = valIdx;
                Feature2Idx2Value[featName].push_back(valStr);
                ++valIdx;
            }
            FeaturesOrder.push_back({featName, feature.Binary()});
            ++featIdx;
        }
    }

    void TCombinator::EnsureValidInit() const {
        Y_ENSURE(AllCombinations.size(), "smth goes wrong");
        Y_ENSURE(AllCombinations.size() == AllCombinationValues.size(), "smth goes wrong");
        THashSet<TString> allCombs;
        for (size_t i = 0; i < AllCombinations.size(); ++i) {
            Y_ENSURE(AllCombinations[i].size() == FeaturesOrder.size(), "invalid combination length");
            Y_ENSURE(AllCombinationValues[i].size() == FeaturesOrder.size(), "invalid combination length");
            allCombs.insert(JoinSeq(" ", AllCombinationValues[i]));
        }
        Y_ENSURE(allCombs.size() == (AllCombinations.size()), "duplicate combinations detected");
        Y_ENSURE(NoShowCombination.size() == FeaturesOrder.size(), "invalid noshow comb length");
        for (size_t i = 0; i < FeaturesOrder.size(); ++i) {
            const auto& featName = FeaturesOrder[i].Name;
            Y_ENSURE(NoShowCombination[i] < Feature2Idx2Value.at(featName).size(), "invalid index in noshow");
        }

        THashSet<TStringBuf> featNames;
        for (const auto& featName : FeaturesOrder) {
            featNames.insert(featName.Name);
        }
        Y_ENSURE(featNames.size() == FeaturesOrder.size(), "duplicate feature names");
    }

    size_t TCombinator::GetOffsetForCombination() const {
        size_t res = 0;
        for (const auto& featInfo : FeaturesOrder) {
            const auto& vals = Feature2Idx2Value.at(featInfo.Name);
            res += featInfo.IsBinary ? vals.size() : 1;
        }
        return res;
    }

    TCombinator::TCombinator(const NSc::TValue& scheme)
        : TBasedOn(scheme) {
        ValidateProtoThrow();
        LoadAndEnumerateFeats();
        LoadAvailCombinations();
        LoadAvailCombinationValues();
        EnsureValidInit();
    }

    size_t TCombinator::FindFeatIdx(const TVector<TFeatInfo>& fi, const TStringBuf& featName) const {
        for (size_t i = 0; i < fi.size(); ++i) {
            if (fi[i].Name == featName) {
                return i;
            }
        }
        ythrow yexception() << "not found: " << featName;
    }

    TCombinationIdxs TCombinator::Intersect(const TAvailVTCombinationsConst& availCombs) const {
        Y_ENSURE(FeaturesOrder.size() == 3, "should contain viewtype, placement and position");
        static const auto viewTypeIdx = 0;
        static const auto placementIdx = 1;
        static const auto positionIdx = 2;

        NSc::TValue viewtype2Placement2Pos;
        for (const auto& vt2place2pos : availCombs) {
            const auto viewType = vt2place2pos.Key();
            for (const auto& place2pos : vt2place2pos.Value()) {
                const auto place = place2pos.Key();
                const auto positions = place2pos.Value();
                if (positions->IsString() && positions->GetString() == "*") {
                    viewtype2Placement2Pos[viewType][place] = "*";
                } else if (positions->IsArray()) {
                    for (const auto& pos : positions->GetArray()) {
                        viewtype2Placement2Pos[viewType][place][ToString(pos)];
                    }
                }
            }
        }

        TCombinationIdxs res;
        for (size_t i = 0; i < AllCombinationValues.size(); ++i) {
            const auto& values = AllCombinationValues[i];
            const auto& viewType = values[viewTypeIdx];
            const auto& placement = values[placementIdx];
            const auto& pos = values[positionIdx];
            const auto& availPoses = viewtype2Placement2Pos[viewType][placement];
            const bool validPos = (availPoses.IsString() && availPoses.GetString() == "*")
                               || (availPoses.IsDict() && availPoses.Has(pos))
                               || (IsNoShowUsedInFeats() && (i + 1) == AllCombinations.size());
            if (validPos) {
                res.push_back(i);
            }
        }
        return res;
    }

    void TCombinator::Combination2Vector(const TCombination& comb, TVector<float>& dst) const {
        size_t curOffset = 0;
        for (size_t i = 0; i < comb.size(); ++i) {
            bool isBinary = FeaturesOrder[i].IsBinary;
            auto valuesCount = Feature2Idx2Value.at(FeaturesOrder[i].Name).size();
            auto featIdx = comb[i];
            if (isBinary) {
                dst[curOffset + featIdx] = 1.;
                curOffset += valuesCount;
            } else {
                dst[curOffset] = featIdx;
                ++curOffset;
            }
        }
    }

    NExtendedMx::TFactors TCombinator::FillWithFeatures(const float* features, const size_t featuresCount) const {
        TCombinationIdxs idxs(AllCombinations.size());
        std::iota(idxs.begin(), idxs.end(), 0);
        return FillWithFeatures(idxs, features, featuresCount);
    }

    NExtendedMx::TFactors TCombinator::FillWithFeatures(const TCombinationIdxs& combinations, const float* features, const size_t featuresCount) const {
        NExtendedMx::TFactors res;
        res.reserve(combinations.size());

        const auto combinationOffset = GetOffsetForCombination();
        for (const auto& combId : combinations) {
            const auto& comb = AllCombinations[combId];
            TVector<float> row(combinationOffset + featuresCount, 0.f);
            Combination2Vector(comb, row);
            if (featuresCount) {
                Copy(features, features + featuresCount, row.begin() + combinationOffset);
            }
            res.emplace_back(std::move(row));
        }
        return res;
    }

    NSc::TValue TCombinator::ToJsonDebug() const {
        NSc::TValue res;
        res["AllCombinations"] = NJsonConverters::ToTValue(AllCombinations);
        res["AllCombinationValues"] = NJsonConverters::ToTValue(AllCombinationValues);
        res["FeaturesOrder"] = NJsonConverters::ToTValue(FeaturesOrder);
        res["Feature2Value2Idx"] = NJsonConverters::ToTValue(Feature2Value2Idx);
        res["Feature2Idx2Value"] = NJsonConverters::ToTValue(Feature2Idx2Value);
        return res;
    }

    bool TCombinator::IsNoShowUsedInFeats() const {
        return Scheme().UseNoShowAsCombination();
    }

    NSc::TValue TCombinator::GetCombinationValues(size_t idx) const {
        Y_ENSURE(idx < AllCombinations.size(), "invalid combination idx");
        return GetValues(AllCombinations[idx]);
    }

    NSc::TValue TCombinator::GetNoShowValues() const {
        return GetValues(NoShowCombination);
    }

    NSc::TValue TCombinator::GetValues(const TCombination& comb) const {
        NSc::TValue res;
        for (size_t i = 0; i < FeaturesOrder.size(); ++i) {
            const auto& featName = FeaturesOrder[i].Name;
            res[featName] = *Scheme().Features()[i].Values()[comb[i]];
        }
        return res;
    }

} // NCombinator
} // NExtendedMx
