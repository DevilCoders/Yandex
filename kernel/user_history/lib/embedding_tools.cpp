#include "embedding_tools.h"

#include <kernel/user_history/user_history.h>
#include <kernel/user_history/proto/user_history.pb.h>

#include <kernel/dssm_applier/utils/utils.h>

#include <google/protobuf/util/message_differencer.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/url/url.h>

#include <ysite/yandex/reqanalysis/normalize.h>

#include <util/generic/maybe.h>
#include <util/generic/ymath.h>

namespace NPersonalization {
    bool CheckEmbeddingOpts(
        const NProto::TEmbeddingOptions& opts,
        const NProto::EModels model,
        const NProto::EEmbeddingType type,
        const ui64 dwt,
        const bool lessThanThreshold)
    {
        return opts.GetModel() == model &&
            opts.GetEmbeddingType() == type &&
            opts.GetDwelltimeThreshold() == dwt &&
            opts.GetLessThanThreshold() == lessThanThreshold;
    }

    bool CheckFadingEmbeddingOpts(
        const NProto::TFadingEmbedding& fe,
        const NProto::EModels model,
        const NProto::EEmbeddingType type,
        const ui64 dwt,
        const float fadingCoefDays,
        const bool lessThanThreshold)
    {
        return CheckEmbeddingOpts(fe.GetOptions(), model, type, dwt, lessThanThreshold) &&
            FuzzyEquals(fe.GetFadingCoefDays(), fadingCoefDays);
    }

    bool FadingEmbeddingsOfTheSameType(const NProto::TFadingEmbedding& f1, const NProto::TFadingEmbedding& f2) {
        const auto& opts2 = f2.GetOptions();
        return CheckFadingEmbeddingOpts(
            f1,
            opts2.GetModel(),
            opts2.GetEmbeddingType(),
            opts2.GetDwelltimeThreshold(),
            f2.GetFadingCoefDays(),
            opts2.GetLessThanThreshold());
    }

    NNeuralNetApplier::EDssmModel ToDssmModel(const NProto::EModels model) {
        using namespace NProto;
        switch (model) {
            case LogDwelltimeBigrams:
                return NNeuralNetApplier::EDssmModel::LogDwellTimeBigrams;
            case BoostingXfOneSeAmSsHard:
                return NNeuralNetApplier::EDssmModel::BoostingXfOneSeAmSsHard;
            case BoostingCtr:
                return NNeuralNetApplier::EDssmModel::BoostingCtr;
            case BoostingXfOne:
                return NNeuralNetApplier::EDssmModel::BoostingXfOne;
            case BoostingXfOneSE:
                return NNeuralNetApplier::EDssmModel::BoostingXfOneSE;
            case BoostingXfWtd:
                return NNeuralNetApplier::EDssmModel::BoostingXfWtd;
            case CtrNoMiner:
                return NNeuralNetApplier::EDssmModel::CtrNoMiner;
            case LogDtBigramsAMHardQueriesNoClicks:
                return NNeuralNetApplier::EDssmModel::LogDtBigramsAMHardQueriesNoClicks;
            case LogDtBigramsAMHardQueriesNoClicksMixed:
                return NNeuralNetApplier::EDssmModel::LogDtBigramsAMHardQueriesNoClicksMixed;
            case MainContentKeywords:
                return NNeuralNetApplier::EDssmModel::MainContentKeywords;
            case Multiclick:
                return NNeuralNetApplier::EDssmModel::Multiclick;
            case MulticosStreams:
                return NNeuralNetApplier::EDssmModel::MulticosStreams;
            case CtrEngSsHard:
                return NNeuralNetApplier::EDssmModel::CtrEngSsHard;
            case FpsSpylogAggregatedQueryPart:
                return NNeuralNetApplier::EDssmModel::FpsSpylogAggregatedQueryPart;
            case ReformulationsLongestClickLogDt:
                return NNeuralNetApplier::EDssmModel::ReformulationsLongestClickLogDt;
            case QueryToImageV8Layer:
                return NNeuralNetApplier::EDssmModel::QueryToImageV8Layer;
            case LogDtBigramsQueryPart:
                return NNeuralNetApplier::EDssmModel::LogDwellTimeBigrams;
            case ReformulationsQueryEmbedderMini:
                return NNeuralNetApplier::EDssmModel::ReformulationsQueryEmbedderMini;
        }
    }

    TMaybe<NPersonalization::NProto::EModels> ToProtoEnum(const NDssmApplier::EDssmModelType model) {
        using namespace NDssmApplier;
        using namespace NPersonalization::NProto;
        switch(model) {
            case EDssmModelType::FpsSpylogAggregatedQueryPart:
                return EModels::FpsSpylogAggregatedQueryPart;
            case EDssmModelType::ReformulationsLongestClickLogDt:
                return EModels::ReformulationsLongestClickLogDt;
            case EDssmModelType::LogDwellTimeBigrams:
                return EModels::LogDwelltimeBigrams;
            case EDssmModelType::ReformulationsQueryEmbedderMini:
                return EModels::ReformulationsQueryEmbedderMini;
            default:
                return Nothing();
        }
    }

    TMaybe<NDssmApplier::EDssmModelType> ToDssmModelType(const NProto::EModels model) {
        switch (model) {
            case NProto::LogDwelltimeBigrams:
                return NDssmApplier::EDssmModelType::LogDwellTimeBigrams;
            case NProto::FpsSpylogAggregatedQueryPart:
                return NDssmApplier::EDssmModelType::FpsSpylogAggregatedQueryPart;
            case NProto::ReformulationsLongestClickLogDt:
                return NDssmApplier::EDssmModelType::ReformulationsLongestClickLogDt;
            case NProto::LogDtBigramsQueryPart:
                return NDssmApplier::EDssmModelType::LogDtBigramsQueryPart;
            case NProto::ReformulationsQueryEmbedderMini:
                return NDssmApplier::EDssmModelType::ReformulationsQueryEmbedderMini;
            default:
                return Nothing();
        }
    }

    void GetQueryEmbedding(const TString& relevParamValue, const NNeuralNetApplier::EDssmModel model, TVector<float>& result) {
        if (!relevParamValue.empty()) {
            const TString decodedParamValue = Base64Decode(relevParamValue);
            TStringInput input(decodedParamValue);
            auto dssmDataPtr = GetDssmDataSerializer(model);
            dssmDataPtr->Load(&input);
            dssmDataPtr->TryGetLatestEmbedding(result);
            NNeuralNetApplier::TryNormalize(result);
        } else {
            const auto embSize = NNeuralNetApplier::GetDefaultEmbeddingSize(model);
            result.resize(embSize, 0.f);
        }
    }

    bool CompareUserRecordsDescriptions(const NProto::TUserRecordsDescription& lhs, const NProto::TUserRecordsDescription& rhs, bool ignoreMaxRecords) {
        if (lhs.HasContainerId() && rhs.HasContainerId()) {
            return lhs.GetContainerId() == rhs.GetContainerId();
        }
        // Legacy fallback
        const NProto::TUserRecordsDescription desc;
        google::protobuf::util::MessageDifferencer differencer;
        differencer.IgnoreField(desc.GetDescriptor()->FindFieldByNumber(NProto::TUserRecordsDescription::kSortOrderOnMirrorFieldNumber));
        differencer.IgnoreField(desc.GetDescriptor()->FindFieldByNumber(NProto::TUserRecordsDescription::kStoreOptionsFieldNumber));
        differencer.IgnoreField(desc.GetDescriptor()->FindFieldByNumber(NProto::TUserRecordsDescription::kRecordLifetimeFieldNumber));
        differencer.IgnoreField(desc.GetDescriptor()->FindFieldByNumber(NProto::TUserRecordsDescription::kContainerIdFieldNumber));
        if (ignoreMaxRecords) {
            differencer.IgnoreField(desc.GetDescriptor()->FindFieldByNumber(NProto::TUserRecordsDescription::kMaxRecordsFieldNumber));
        }
        return differencer.Compare(lhs, rhs);
    }

    void CalcHashes(TUserHistoryRecord& record) {
        static const NQueryNorm::TDoppNormalizer dopper;

        if (!record.Query.empty()) {
            const TString queryDNorm = dopper.NormalizeSafe(record.Query, NLanguageMasks::BasicLanguages());
            record.QueryHash = MurmurHash<ui64>(queryDNorm.data(), queryDNorm.size());
        }
        if (!record.Url.empty()) {
            record.UrlHash = MurmurHash<ui64>(record.Url.data(), record.Url.size());
            const TStringBuf host = GetOnlyHost(record.Url);
            record.HostHash = MurmurHash<ui64>(host.data(), host.size());
        }
    }
}
