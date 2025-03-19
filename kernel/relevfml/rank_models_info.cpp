#include "rank_models_info.h"
#include "rank_models_factory.h"

const TString TRankingModelInfo::DEFAULT_VALUE = "None";

void FormatRankingModelsInfo(const TRankModelsMapFactory& rankModels, IRankingModelInfoFormatter& formatter, bool calcMD5) {
    for (const auto& rankModel : rankModels) {
        TRankingModelInfo rankingModelInfo;
        const NMatrixnet::TModelInfo* modelInfoPtr = nullptr;

        rankingModelInfo.ModelName = rankModel.first;

        auto getProp = [](const NMatrixnet::TModelInfo* modelInfo, const TString& name) -> TString {
            if (modelInfo) {
                if (const auto prop = modelInfo->FindPtr(name)) {
                    return *prop;
                }
            }
            return TRankingModelInfo::DEFAULT_VALUE;
        };

        if (const auto matrixnetPtr = std::get_if<NMatrixnet::TMnSsePtr>(&rankModel.second)) {
            const auto& matrixnet = **matrixnetPtr;
            modelInfoPtr = &matrixnet.Info;

            if (calcMD5) {
                rankingModelInfo.MatrixnetMD5 = matrixnet.MD5();
            }
        }

        if (const auto bundleDescription = std::get_if<NMatrixnet::TBundleDescription>(&rankModel.second)) {
            modelInfoPtr = &bundleDescription->Info;
        }

        rankingModelInfo.MatrixnetID = getProp(modelInfoPtr, "formula-id");
        rankingModelInfo.MatrixnetName = getProp(modelInfoPtr, "fname");
        rankingModelInfo.MatrixnetMemorySource = (modelInfoPtr->find("dynamic_source") == modelInfoPtr->end()) ? "static" : "dynamic";
        const auto currentRankModel = rankModels.GetMatrixnet(rankingModelInfo.ModelName);
        if (currentRankModel) {
            rankingModelInfo.GetModelMatrixnet = getProp(&(currentRankModel->Info), "fname");
        }

        formatter.FormatModelInfo(rankingModelInfo);
    }
}
