#pragma once

#include <kernel/factor_storage/factors_reader.h>
#include <kernel/factor_storage/factor_storage.h>

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NFactors {

    void ExtractCompressedFactors(const TStringBuf& encodedFactors, TVector<float>& rankingFactors);
    void ExtractHuffCompressedFactors(const TStringBuf& encodedFactors, TVector<float>& rankingFactors);


    struct TFilteredFactors {
        TString Borders;
        TVector<float> Factors;
    };

    class TFactorsFilter {
    public:
        TFactorsFilter(TFactorStorage* fs)
                : FactorStorage_(fs)
        {
            Y_ENSURE(FactorStorage_);
        }

        TFilteredFactors FilterFactorsWithBorders(TStringBuf borders) {
            TConstFactorView factorView = FactorStorage_->CreateConstView();
            TVector<float> transformedFeatures = NFSSaveLoad::TransformFeaturesVectorToSlices(
                    {factorView.begin(), factorView.end()}, SerializeFactorBorders(FactorStorage_->GetBorders()),
                    borders);
            return {TString(borders), transformedFeatures};
        }

        TFilteredFactors FilterFactorsWithSlices(const TVector<EFactorSlice>& slices) {
            NFactorSlices::TSlicesMetaInfo meta = NFactorSlices::TGlobalSlicesMetaInfo::Instance();
            for (const auto& s : slices) {
                NFactorSlices::EnableSlices(meta, s);
            }

            TFactorDomain domain{NFactorSlices::TFactorBorders(meta)};
            const auto serializedNewBorders = SerializeFactorBorders(domain.GetBorders());
            return FilterFactorsWithBorders(serializedNewBorders);
        }

    private:
        TFactorStorage* FactorStorage_;
    };

} // namespace NFactors

