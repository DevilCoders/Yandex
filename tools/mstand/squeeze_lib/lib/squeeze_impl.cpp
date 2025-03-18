#include "squeeze_impl.h"
#include "squeeze_yt.h"

#include <util/stream/str.h>


namespace NMstand {

TVersions GetSqueezeVersions(const TString& service)
{
    TVersions versions {{"_common", COMMON_VERSION}};

    auto squeezer = TSqueezerFactory::GetSqueezer(service);
    versions[service] = squeezer->GetVersion();

    return versions;
}

TString CreateTempTable(const TString& service)
{
    return "//tmp/table/" + service;
}

TString SqueezeDay(const TVector<TExperimentForSqueeze>& experiments, const TYtParams& ytParams, const TFilterMap& filters)
{
    Cerr << ytParams.ToString() << Endl;
    Cerr << "Got " << experiments.size() << " experiment(s)" << Endl;
    for (const auto& exp : experiments) {
        Cerr << '\t' << exp.ToString() << Endl;
    }

    auto client = NYT::CreateClient(ytParams.Server);
    NYT::ITransactionPtr tx;
    if (ytParams.TransactionId.empty()) {
        tx = client->StartTransaction();
    } else {
        tx = client->AttachTransaction(GetGuid(ytParams.TransactionId));
    }

    NMstand::TUserSessionsSqueezer squeezer(experiments);

    NYT::IOperationPtr operation;
    if (experiments[0].Service == NServiceType::MARKET_SEARCH_SESSIONS) {
        Cerr << "Run ksv-reducer" << Endl;
        operation = tx->Reduce(
            CreateReduceSpec(experiments, ytParams),
            new NMstand::TUserSessionsReducerKSV(squeezer),
            CreateOperationSpec(ytParams)
        );
    }
    else {
        NRA::TEntitiesManager entManager = GetEntitiesManagerForSurplus(/*useEntitiesManager*/ true);
        Cerr << "Run columns-reducer" << Endl;
        operation = tx->Reduce(
            CreateReduceSpec(experiments, ytParams, entManager, tx),
            new NMstand::TUserSessionsReducer(squeezer, entManager, filters),
            CreateOperationSpec(ytParams)
        );
    }

    if (ytParams.TransactionId.empty()) {
        tx->Commit();
    }

    return GetGuidAsString(operation->GetId());
}

};
