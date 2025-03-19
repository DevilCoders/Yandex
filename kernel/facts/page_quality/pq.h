#pragma once

#include <kernel/querydata/idl/querydata_structs.pb.h>
#include <kernel/querydata/server/qd_server.h>

namespace NFacts {

class TPageQualityDatabase {
public:
    TPageQualityDatabase(const TString& hostTrieFileName);
    bool TryGetPqForHost(const TStringBuf owner, float& pq, float filterLevel=0.f) const;
    bool TryGetPqForUrl(const TStringBuf url, float& pq, float filterLevel=0.f) const;

private:
    THolder<NQueryData::TQueryDatabase> QueryDatabase;
    TOwnerCanonizer Canonizer;
};

float NormalizePq(float pq);

}
