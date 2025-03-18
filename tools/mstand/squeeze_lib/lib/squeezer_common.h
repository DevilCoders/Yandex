#pragma once

#include "common.h"

#include <mapreduce/yt/interface/common.h>
#include <quality/user_sessions/request_aggregate_lib/all.h>


namespace NMstand {

const ui32 COMMON_VERSION = 1;

class TActionsSqueezer
{
public:
    virtual NYT::TTableSchema GetSchema() const = 0;
    virtual ui32 GetVersion() const = 0;
    virtual bool CheckRequest(const NRA::TRequest* request) const = 0;
    virtual void GetActions(TActionSqueezerArguments& args) const = 0;
    virtual ~TActionsSqueezer() {}

protected:
    TExpBucketInfo CheckExperiments(TActionSqueezerArguments& args, const NRA::TRequest* request) const;
    bool IsTestidMatch(const NRA::TRequest* request, const TExperimentForSqueeze& exp) const;
    bool IsFilterMatch(const NRA::TRequest* request, const TExperimentForSqueeze& exp, const THashMap<TString, NRA::TFilterPack>& hash2Filters) const;
    i32 GetBucket(const NRA::TRequest* request, const TExperimentForSqueeze& exp) const;
};

NYT::TNode BuildRequestData(const NRA::TRequest* request);

};
