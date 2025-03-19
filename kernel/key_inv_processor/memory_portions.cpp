#include "memory_portions.h"

#include <kernel/walrus/advmerger.h>

TMemoryPortionUsage::TMemoryPortionUsage(IYndexStorage::FORMAT format, int maxDocs, TString prefix, TString suffix, TString dir, const bool buildByKeysFlag)
    : BuildByKeysFlag(buildByKeysFlag)
{
    Dir = dir;
    Format = format;
    Counter = 0;
    MaxDocs = maxDocs;
    PortionNum = 0;
    Prefix = prefix;
    Suffix = suffix;
    Portions.push_back(new NIndexerCore::TMemoryPortion(Format));
}

bool TMemoryPortionUsage::IncDoc() {
    bool result = false;
    if (++Counter == MaxDocs) {
        StoreResult();
        result = true;
    }
    Portions.push_back(new NIndexerCore::TMemoryPortion(Format));
    return result;
}

bool TMemoryPortionUsage::StoreResult() {
    if (Counter == 0)
        return false;
    TString portionPrefix = Prefix + ToString(PortionNum) + "p";
    TString portionName = portionPrefix + Suffix;
    TAtomicSharedPtr<NIndexerCore::TMemoryPortion> mp = GetResultAndFlush();
    mp->Flush();
    TString fileKey = Dir + "/" + portionName + "k";
    TString fileInv = Dir + "/" + portionName + "i";
    TFixedBufferFileOutput foKeys(fileKey);
    foKeys.Write(mp->GetKeyBuffer().Data(), mp->GetKeyBuffer().Size());
    TFixedBufferFileOutput foInvs(fileInv);
    foInvs.Write(mp->GetInvBuffer().Data(), mp->GetInvBuffer().Size());

    PortionNum++;
    Counter = 0;
    return true;
}

TAtomicSharedPtr<NIndexerCore::TMemoryPortion> TMemoryPortionUsage::GetResultAndFlush() {
    TVector<TPortionBuffers> pbufs;

    for (size_t i = 0; i < Portions.size(); i++) {
        Portions[i]->Flush();
        pbufs.push_back(TPortionBuffers(
            Portions[i]->GetKeyBuffer().Data(),
            Portions[i]->GetKeyBuffer().Size(),
            Portions[i]->GetInvBuffer().Data(),
            Portions[i]->GetInvBuffer().Size()));
    }

    TAtomicSharedPtr<NIndexerCore::TMemoryPortion> result = new NIndexerCore::TMemoryPortion(Format);
    MergeMemoryPortions(&pbufs[0], Portions.size(), Format, nullptr, BuildByKeysFlag, *result);
    Portions.clear();
    return result;
}
