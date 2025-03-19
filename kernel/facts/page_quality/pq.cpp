#include "pq.h"

namespace NFacts {

TPageQualityDatabase::TPageQualityDatabase(const TString& hostTrieFileName)
    : QueryDatabase(new NQueryData::TQueryDatabase())
{
    Y_ENSURE(!hostTrieFileName.empty());
    QueryDatabase->AddSourceFile(hostTrieFileName.c_str());
    Canonizer.LoadTrueOwners();
}

bool TPageQualityDatabase::TryGetPqForHost(const TStringBuf owner, float& pq, float filterLevel) const {
    NQueryData::TQueryData qdBuff;
    NQueryData::TRequestRec qdRequest;

    qdRequest.YandexTLD = "ru";
    qdRequest.DocItems.MutableCategs().push_back(owner);

    QueryDatabase->GetQueryData(qdRequest, qdBuff);

    for (const auto& src : qdBuff.GetSourceFactors()) {
        NSc::TValue jsonValue = NSc::TValue::FromJson(src.GetJson());
        if (jsonValue["filt"].IsNumber() && jsonValue["pq"].IsNumber() && jsonValue["filt"].GetNumber() >= filterLevel) {
            pq = jsonValue["pq"].GetNumber();
            return true;
        }
    }

    return false;
}

bool TPageQualityDatabase::TryGetPqForUrl(const TStringBuf url, float& pq, float filterLevel) const {
    const TStringBuf owner = Canonizer.GetUrlOwner(url);
    return TryGetPqForHost(owner, pq, filterLevel);
}

float NormalizePq(float pq) {
    if (pq < 1.0) {
        return 0.0;
    }
    if (pq > 5.0) {
        return 1.0;
    }
    return (pq - 1.0) / 4.0;
}

}
