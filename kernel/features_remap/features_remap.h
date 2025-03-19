#pragma once
#include <kernel/features_remap/metadata/features_remap.pb.h>
#include <kernel/factor_storage/factor_view.h>
#include <util/generic/hash.h>

namespace NFeaturesRemap {
    NProto::TCalculations ParseTxt(TStringBuf in);
    NProto::TCalculations ParseCgi(TStringBuf in, bool escaped);

    void RestoreFullInfo(NProto::TCalculations& in);
    void ClearHrInfo(NProto::TCalculations& in);

    TString ToCgiAsIs(const NProto::TCalculations& in, bool escaped);
    TString ToCgiAutoConvertations(NProto::TCalculations& in, bool escaped);

    struct TCalcApplyReport {
        bool DoneModifications = false;
        bool HasErrors = false;
        TString ErrorsReport;
    };

    TCalcApplyReport DoCalculation(
        const NProto::TCalculations& c,
        TFactorView& view,
        NProto::EStage stageFilter,
        TStringBuf searchType,
        THashMap<TString, double>&& additionalContext
    );

    TCalcApplyReport DoCalculation(
        const NProto::TCalculations& c,
        const TVector<TFactorStorage*>& storages,
        NProto::EStage stageFilter,
        TStringBuf searchType,
        THashMap<TString, double>&& additionalContext
    );
}
