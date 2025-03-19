#include "random_log_parse.h"

#include <kernel/factor_slices/factor_domain.h>
#include <kernel/factor_slices/factor_slices.h>
#include <kernel/factor_slices/slice_iterator.h>
#include <kernel/factor_storage/factor_storage.h>
#include <kernel/factors_util/factors_util.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/string_utils/base64/base64.h>

bool ExtractRankingFactors(const NJson::TJsonValue& randomLogData,
    TVector<float>& rankingFactors,
    const NFactorSlices::EFactorSlice slice)
{
    const NJson::TJsonValue* vField;
    if (slice == NFactorSlices::EFactorSlice::WEB && randomLogData.GetValuePointer("Factors", &vField)) {
        TString base64str;
        if (!vField->GetString(&base64str)) {
            return false;
        }
        NFactors::ExtractCompressedFactors(base64str, rankingFactors);
    } else if (slice == NFactorSlices::EFactorSlice::WEB && randomLogData.GetValuePointer("HuffFactors", &vField)) {
        TString base64str;
        if (!vField->GetString(&base64str)) {
            return false;
        }
        NFactors::ExtractHuffCompressedFactors(base64str, rankingFactors);
    } else if (randomLogData.GetValuePointer("AllFactorsCrate", &vField)) {
        TString base64str;
        if (!vField->GetString(&base64str)) {
            return false;
        }
        TString crateStr;
        Base64Decode(base64str, crateStr);
        TSlicesMetaInfo allInfo;
        TStringStream ss(crateStr);
        const THolder<TFactorStorage> fs = NFSSaveLoad::Deserialize(&ss, allInfo);
        if (!fs) {
            return false;
        }
        const TConstFactorView view = fs->CreateConstViewFor(slice);
        rankingFactors.assign(view.begin(), view.end());
    } else {
        return false;
    }
    return true;
}

bool ExtractRankingFactorsFromMarker(const TStringBuf randomLogMarker,
    TVector<float>& rankingFactors,
    const NFactorSlices::EFactorSlice slice)
{
    NJson::TJsonValue randomLogData;
    if (!ReadJsonFastTree(Base64Decode(randomLogMarker), &randomLogData, true)) {
        return false;
    }
    return ExtractRankingFactors(randomLogData, rankingFactors, slice);
}

namespace {
    bool InnerFillRankingFactorsFromRandomLog(const NJson::TJsonValue& randomLogData,
        const NFactorSlices::TFactorDomain& factorDomain,
        TVector<float>& rankingFactors)
    {
        Y_ENSURE(factorDomain.Size() > 0,
            "Empty factor domain: probably you need to fill it with slices or "
            "link your binary with code generated factors info for each used factor slice");

        rankingFactors.clear();

        bool slicelessFormat = true;
        TVector<float> buffer;
        for (auto itr = factorDomain.Begin(); itr != factorDomain.End(); itr.NextLeaf()) {
            const auto slice = itr.GetLeaf();

            buffer.clear();
            if (ExtractRankingFactors(randomLogData, buffer, slice) && !buffer.empty()) {
                if (rankingFactors.empty()) {
                    rankingFactors.assign(factorDomain.Size(), 0.0);
                }

                slicelessFormat = false;

                for (NFactorSlices::TFactorIndex relFactorId = 0; static_cast<ui32>(relFactorId) < buffer.size(); ++relFactorId) {
                    const auto fullRelFactorId = NFactorSlices::TFullFactorIndex(slice, relFactorId);
                    if (factorDomain.HasIndex(fullRelFactorId)) {
                        const auto absFactorId = factorDomain.GetIndex(NFactorSlices::TFullFactorIndex(slice, relFactorId));
                        rankingFactors[absFactorId] = buffer[relFactorId];
                    }
                }
            }
        }

        if (slicelessFormat) {
            const bool initialized = (ExtractRankingFactors(randomLogData, rankingFactors) && !rankingFactors.empty());

            if (initialized) {
                rankingFactors.resize(factorDomain.Size(), 0.0);
            } else {
                rankingFactors.clear();
            }
        }

        return !rankingFactors.empty();
    }
} // namespace

bool FillRankingFactorsFromRandomLogMarker(const TStringBuf randomLogMarker,
        const NFactorSlices::TFactorDomain& factorDomain,
        TVector<float>& rankingFactors)
{
    NJson::TJsonValue randomLogData;
    if (!ReadJsonFastTree(Base64Decode(randomLogMarker), &randomLogData, true)) {
        return false;
    }
    return FillRankingFactorsFromRandomLog(randomLogData, factorDomain, rankingFactors);
}

bool FillRankingFactorsFromRandomLog(const NJson::TJsonValue& randomLogData,
        const NFactorSlices::TFactorDomain& factorDomain,
        TVector<float>& rankingFactors)
{
    return InnerFillRankingFactorsFromRandomLog(randomLogData, factorDomain, rankingFactors);
}
