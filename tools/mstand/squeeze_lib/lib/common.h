#pragma once

#include "tools/mstand/squeeze_lib/lib/common.sc.h"

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/yson/node/node.h>

#include <quality/user_sessions/request_aggregate_lib/all.h>
#include <quality/user_sessions/request_aggregate_lib/ordered_entities_def.h>

#include <util/digest/multi.h>
#include <util/draft/date.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>


namespace NMstand {

using TSchemaSqueezeParams = NMstand::TSqueezeParams<TSchemeTraits>;
using TSchemaExperimentForSqueeze = NMstand::TSqueezeParams<TSchemeTraits>::TExperimentForSqueezeConst;
using TSchemaYtParams = NMstand::TSqueezeParams<TSchemeTraits>::TYtParams;

struct TFilter;

using TFilterList = TVector<TFilter>;
using TFilterMap = THashMap<TString, TFilterList>;

namespace NActionType {
    const TString REQUEST = "request";
};

namespace NServiceType {
    const TString WEB = "web";
    const TString TOUCH = "touch";
    const TString WEB_DESKTOP_EXTENDED = "web-desktop-extended";
    const TString WEB_TOUCH_EXTENDED = "web-touch-extended";
    const TString WEB_SURVEYS = "web-surveys";
    const TString MARKET_SEARCH_SESSIONS = "market-search-sessions";
    const TString YUID_REQID_TESTID_FILTER = "yuid-reqid-testid-filter";
    const TString VIDEO = "video";
};

struct TFilter {
    TString Name;
    TString Value;

    Y_SAVELOAD_DEFINE(Name, Value);

    TFilter() = default;
    TFilter(const TString& name, const TString& value)
        : Name(name)
        , Value(value)
    {}

    TString ToString() const {
        return TStringBuilder()
            << "Filter<Name: "
            << Name
            << ", Value: "
            << Value
            << ">";
    }
};

struct TExperimentForSqueeze {
    TDate Day;
    TVector<TFilter> Filters;
    TString FilterHash;
    TString Service;
    TString TempTablePath;
    TString Testid;
    ui32 TableIndex;
    bool IsHistoryMode;

    Y_SAVELOAD_DEFINE(Day, Filters, FilterHash, Service, TempTablePath, Testid, TableIndex, IsHistoryMode);

    TExperimentForSqueeze() = default;
    TExperimentForSqueeze(
            const TDate& day,
            const TVector<TFilter>& filters,
            const TString& filterHash,
            const TString& service,
            const TString& tempTablePath,
            const TString& testid,
            const ui32 tableIndex,
            const bool isHistoryMode)
        : Day(day)
        , Filters(filters)
        , FilterHash(filterHash)
        , Service(service)
        , TempTablePath(tempTablePath)
        , Testid(testid)
        , TableIndex(tableIndex)
        , IsHistoryMode(isHistoryMode)
    {}

    explicit TExperimentForSqueeze(const TSchemaExperimentForSqueeze& exp)
        : Day(TString(exp.Day()))
        , FilterHash(exp.FilterHash())
        , Service(exp.Service())
        , TempTablePath(exp.TempTablePath())
        , Testid(exp.Testid())
        , TableIndex(exp.TableIndex())
        , IsHistoryMode(exp.IsHistoryMode())
    {
        for (const auto& fltr : exp.Filters()) {
            Filters.emplace_back(TString(fltr.Name()), TString(fltr.Value()));
        }
    }

    bool operator==(const TExperimentForSqueeze &other) const {
        return Day == other.Day
            && FilterHash == other.FilterHash
            && Service == other.Service
            && Testid == other.Testid;
    }

    struct Hash {
        inline size_t operator()(const TExperimentForSqueeze& k) const {
            return MultiHash(k.Day.ToStroka("%Y%m%d"), k.FilterHash, k.Service, k.Testid);
        }
    };

    TString ToString() const {
        return TStringBuilder()
            << "TExperimentForSqueeze<Testid: "
            << Testid
            << ", Service: "
            << Service
            << ", FilterHash: "
            << FilterHash
            << ">";
    }

    bool IsBro() const {
        return Testid.StartsWith("mbro__");
    }

    bool IsAllUsers() const {
        return Testid == "0";
    }
};

struct TYtParams
{
    bool AddAcl;
    bool TentativeEnable;
    TVector<TString> SourcePaths;
    TVector<TString> YtFiles;
    TVector<TString> YuidPaths;
    TString LowerKey;
    TString Pool;
    TString Server;
    TString TransactionId;
    TString UpperKey;
    ui64 DataSizePerJob;
    ui64 MaxDataSizePerJob;
    ui64 MemoryLimit;

    TYtParams() = default;

    explicit TYtParams(const TSchemaYtParams& params)
        : AddAcl(params.AddAcl())
        , TentativeEnable(params.TentativeEnable())
        , LowerKey(params.LowerKey())
        , Pool(params.Pool())
        , Server(params.Server())
        , TransactionId(params.TransactionId())
        , UpperKey(params.UpperKey())
        , DataSizePerJob(params.DataSizePerJob())
        , MaxDataSizePerJob(params.MaxDataSizePerJob())
        , MemoryLimit(params.MemoryLimit())
    {
        for (const auto& path : params.SourcePaths()) {
            SourcePaths.emplace_back(TString(path));
        }
        for (const auto& path : params.YtFiles()) {
            YtFiles.emplace_back(TString(path));
        }
        for (const auto& path : params.YuidPaths()) {
            YuidPaths.emplace_back(TString(path));
        }
    }

    TString ToString() const {
        return TStringBuilder()
            << "TYtParams<" << Endl
            << "\tPool: " << Pool << Endl
            << "\tServer: " << Server << Endl
            << "\tTransactionId: " << TransactionId << Endl
            << "\tDataSizePerJob: " << DataSizePerJob << Endl
            << "\tMaxDataSizePerJob: " << MaxDataSizePerJob << Endl
            << "\tMemoryLimit: " << MemoryLimit << Endl
            << ">";
    }
};

struct TExpBucketInfo {
    THashMap<TExperimentForSqueeze, i32, TExperimentForSqueeze::Hash> Buckets;
    THashSet<TExperimentForSqueeze, TExperimentForSqueeze::Hash> Matched;

    i32 GetBucket(const TExperimentForSqueeze& exp) const {
        const auto it = Buckets.find(exp);
        if (it == Buckets.end()) {
            return -1;
        }
        return it->second;
    }
};

struct TResultAction
{
    TExperimentForSqueeze Experiment;
    NYT::TNode Action;
    TExpBucketInfo ExpBucketInfo;

    TResultAction() = default;
    TResultAction(const NYT::TNode& action, const TExpBucketInfo& expBucketInfo)
        : Action(action)
        , ExpBucketInfo(expBucketInfo)
    {}
    TResultAction(const TExperimentForSqueeze& experiment, const NYT::TNode& action)
        : Experiment(experiment)
        , Action(action)
    {}
};

struct TActionSqueezerArguments
{
    TActionSqueezerArguments() = default;
    TActionSqueezerArguments(
        const NRA::TRequestsContainer& Container,
        const TVector<TExperimentForSqueeze>& experiments,
        const THashMap<TString, NRA::TFilterPack>& hash2filters,
        const TVector<NYT::TNode>& rows,
        const THashMap<TString, NRA::TFilterPack>& filters);

    const NRA::TRequestsContainer& Container;
    const TVector<TExperimentForSqueeze>& Experiments;
    const THashMap<TString, NRA::TFilterPack>& Filters;
    const THashMap<TString, NRA::TFilterPack>& Hash2Filters;
    const TVector<NYT::TNode>& Rows;
    THashSet<TExperimentForSqueeze, TExperimentForSqueeze::Hash> ResultExperiments;
    TVector<TResultAction> ResultActions;
};

void LoadSqueezeParameters(const TString& filename, TVector<TExperimentForSqueeze>& experiments, TYtParams& ytParams, TFilterMap& filters);
void SaveOuputParameters(const TString& filename, const TString& transactionId);
NRA::TEntitiesManager GetEntitiesManagerForSurplus(bool useEntitiesManager = false);

};
