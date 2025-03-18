#pragma once

#include "common.h"
#include "squeezer.h"

#include <quality/user_sessions/createlib/qb3/parser/operation.h>
#include <quality/user_sessions/request_aggregate_lib/all.h>


namespace NUS = NUserSessions;

namespace NMstand {

using TReader = NYT::TTableReader<NYT::TNode>;
using TWriter = NYT::TTableWriter<NYT::TNode>;

class TUserSessionsReducerKSV
    : public NYT::IReducer<TReader, TWriter>
{
public:
    Y_SAVELOAD_JOB(Squeezer);

    TUserSessionsReducerKSV() = default;
    explicit TUserSessionsReducerKSV(const TUserSessionsSqueezer& squeezer);

    void Start(TWriter* /*writer*/) override;
    void Do(TReader* reader, TWriter* writer) override;

private:
    TBlockStatInfo BlockStatDict;
    TUserSessionsSqueezer Squeezer;
};

class TUserSessionsReducer
    : public NUS::ISessionReducer<TWriter>
{
public:
    Y_SAVELOAD_JOB(Squeezer, EntManager, RawUid2Filter);

    TUserSessionsReducer() = default;
    explicit TUserSessionsReducer(const TUserSessionsSqueezer& squeezer, const NRA::TEntitiesManager& entManager, const TFilterMap& rawUid2Filter);

    void Start(TWriter* /*writer*/) override;
    void Do(TProtoSessionReader* reader, TWriter* writer) override;

private:
    TBlockStatInfo BlockStatDict;
    TUserSessionsSqueezer Squeezer;
    NRA::TEntitiesManager EntManager;
    THashMap<TString, NRA::TFilterPack> Filters;
    TFilterMap RawUid2Filter;

private:
    void InitFilters();
};

NYT::TRichYPath GetInputTable(const TString& path, const TString& lowerKey, const TString& upperKey);

NYT::TReduceOperationSpec CreateReduceSpec(const TVector<NMstand::TExperimentForSqueeze>& experiments, const NMstand::TYtParams& ytParams);
NYT::TReduceOperationSpec CreateReduceSpec(
    const TVector<NMstand::TExperimentForSqueeze>& experiments,
    const NMstand::TYtParams& ytParams,
    const NRA::TEntitiesManager& entManager,
    const NYT::ITransactionPtr& tx
);
NYT::TOperationOptions CreateOperationSpec(const TYtParams& ytParams);

};
