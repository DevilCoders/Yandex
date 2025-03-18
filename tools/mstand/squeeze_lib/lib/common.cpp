#include "common.h"


namespace NMstand
{

TActionSqueezerArguments::TActionSqueezerArguments(
        const NRA::TRequestsContainer& container,
        const TVector<TExperimentForSqueeze>& experiments,
        const THashMap<TString, NRA::TFilterPack>& hash2filters,
        const TVector<NYT::TNode>& rows,
        const THashMap<TString, NRA::TFilterPack>& filters)
    : Container(container)
    , Experiments(experiments)
    , Filters(filters)
    , Hash2Filters(hash2filters)
    , Rows(rows)
{}

void LoadSqueezeParameters(
    const TString& filename,
    TVector<NMstand::TExperimentForSqueeze>& experiments,
    TYtParams& ytParams,
    TFilterMap& filters
) {
    Cerr << "Read squeeze params from file: " << filename << Endl;

    TUnbufferedFileInput jsonStream(filename);
    auto value = NSc::TValue::FromJsonThrow(jsonStream.ReadAll());
    TSchemaSqueezeParams squeezeParams(&value);

    Y_VERIFY(experiments.size() == 0);
    for (const auto& exp : squeezeParams.Experiments()) {
        experiments.emplace_back(exp);
    }

    ytParams = std::move(TYtParams(squeezeParams.YtParams()));

    Y_VERIFY(filters.size() == 0);
    for (const auto& item : squeezeParams.RawUid2Filter()) {
        TFilterList tmp_filters;
        for (const auto& fltr : item.Value()) {
            tmp_filters.emplace_back(TString(fltr.Name()), TString(fltr.Value()));
        }
        filters[item.Key()] = std::move(tmp_filters);
    }
}

void SaveOuputParameters(const TString& filename, const TString& operationId) {
    NSc::TValue outputJson;
    outputJson.SetDict();
    outputJson["operation_id"] = operationId;

    TUnbufferedFileOutput jsonStream(filename);
    outputJson.ToJsonPretty(jsonStream);

    Cerr << "OperationId '" << operationId << "' was saved successuffly into the file " << filename << Endl;
}

NRA::TEntitiesManager GetEntitiesManagerForSurplus(bool useEntitiesManager) {
    if (!useEntitiesManager) {
        return NRA::GetFullEntitiesConfiguration();
    }
    auto manager = NRA::TEntitiesManager();
    manager.AddEntityByMethod<&NRA::TBlockstatRequestProperties::GetBSBlocks>();
    manager.AddEntityByClass<NRA::TPortalResult>();
    manager.AddEntityByClass<NRA::TPortalZenResult>();
    manager.AddEntity(NRA::TEntityID::WebClicks);
    manager.AddEntityByMethod<&NRA::TRearrRequestProperties::GetReqrearr>();
    manager.AddEntityByMethod<&NRA::TRelevRequestProperties::GetReqrelev>();
    manager.AddEntityByMethod<&NRA::TSearchPropsRequestProperties::GetSearchProps>();
    manager.AddEntityByClass<NRA::TDirectResult>();
    manager.AddEntityByClass<NRA::TYandexNavSuggestClick>();
    manager.AddEntityByClass<NRA::TBlenderWizardResult>();
    manager.AddEntityByClass<NRA::TTVOnlineStreamBlenderWizardResult>();
    manager.AddEntityByClass<NRA::TVideoWizardResult>();
    manager.AddEntityByClass<NRA::TRecommendationResult>();
    manager.AddEntityByClass<NRA::TMarketProducerProperties>();
    manager.AddEntityByClass<NRA::TVideoPlayerEvent>();
    manager.AddEntityByClass<NRA::TTVOnlinePlayerEvent>();
    manager.AddEntityByMethod<&NRA::TServiceDomRegionProperties::GetServiceDomRegion>();
    return manager;
}

};
