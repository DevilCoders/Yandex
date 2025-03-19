#include "finaldump.h"

#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/factors/factors.h>
#include <kernel/snippets/factors/factor_storage.h>

#include <util/string/cast.h>

namespace NSnippets {
    void TFinalFactorsDump::OnBestFinal(const TSnip& snip, bool isByLink) {
        if (isByLink && Result.size()) {
            return;
        }
        Result.clear();
        for (TFactorDomain::TIterator it = snip.Factors.GetDomain().Begin(); it.Valid(); it.Next()) {
            if (it.GetFactorInfo().IsUnusedFactor())
                continue;
            if (Result.size()) {
                Result += "\t";
            }
            Result += it.GetFactorInfo().GetFactorName();
            Result += ":";
            Result += ToString(snip.Factors[it.GetIndex()]);
        }
        Result += "\tweight:" + ToString(snip.Weight);
    }
}
