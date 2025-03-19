#include "impltime_ut_helper.h"

#include <kernel/factors_info/factors_info.h>

#include <library/cpp/regex/pire/regexp.h>

#include <util/datetime/base.h>
#include <util/string/join.h>

void EnsureImplementationTimeFormat(const IFactorsInfo* factors)
{
    NRegExp::TFsm regexp("[0-9]{4}-[0-9]{2}-[0-9]{2}");
    TVector<ui32> badFactors;
    for (ui32 i = 0; i < factors->GetFactorCount(); i++) {
        NJson::TJsonValue extJson = factors->GetExtJson(i);
        NJson::TJsonValue* pImplTime = extJson.GetMapSafe().FindPtr("ImplementationTime");
        if (!pImplTime)
            continue;
        TString implTime = pImplTime->GetStringSafe();
        // regexp covers the exact format, ParseISO8601DateTime rejects 2018-31-01 and 2019-00-00
        time_t parsed;
        if (!NRegExp::TMatcher(regexp).Match(implTime).Final() || !ParseISO8601DateTime(implTime.data(), implTime.size(), parsed))
            badFactors.push_back(i);
    }
    if (!badFactors.empty()) {
        ythrow yexception() << "bad format of ImplementationTime for factors " + JoinSeq(",", badFactors) + "; YYYY-MM-DD expected";
    }
}

void EnsureImplementationTimePresence(const IFactorsInfo* factors, ui32 startFactor)
{
    TVector<ui32> unfilledFactors;
    TVector<ui32> incorrectFactors;
    for (ui32 i = startFactor; i < factors->GetFactorCount(); i++) {
        NJson::TJsonValue extJson = factors->GetExtJson(i);
        NJson::TJsonValue* pImplTime = extJson.GetMapSafe().FindPtr("ImplementationTime");
        bool needImplTime = !(factors->IsUnimplementedFactor(i) || factors->IsRemovedFactor(i));
        if (needImplTime && !pImplTime) {
            unfilledFactors.push_back(i);
        }
        if (!needImplTime && pImplTime) {
            incorrectFactors.push_back(i);
        }
    }
    if (!unfilledFactors.empty()) {
        ythrow yexception() << "ImplementationTime must be filled for factors " + JoinSeq(",", unfilledFactors);
    }
    if (!incorrectFactors.empty()) {
        ythrow yexception() << "ImplementationTime found for unimplemented or removed factors " + JoinSeq(",", incorrectFactors);
    }
}
