#pragma once
#include <kernel/dssm_applier/nn_applier/lib/layers.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace NNeuralNetApplier {

    enum class EDssmModel {
        Unknown = 0,
        Multiclick = 1,
        Uta = 2,
        CtrNoMiner = 3,
        LogDtBigramsAMHardQueriesNoClicks = 4,
        MulticosStreams = 5,
        BoostingXfOne = 6,
        BoostingXfOneSE = 7,
        BoostingXfOneSEHard = 8,
        BoostingCtr = 9,
        BoostingXfWtd = 10,
        LogDwellTimeBigrams = 11,
        MainContentKeywords = 12,
        LogDtBigramsAMHardQueriesNoClicksMixed = 13,
        BoostingXfOneSeAmSsHard = 14,
        DwellTimeRegChainEmbedding = 15,
        QueryWordTitleEmbedding = 16,
        LogDwellTime50Tokenized = 17,
        PantherTerms = 18,
        BoostingSerpSimilarityHard = 19,
        NNOverDssm = 20,
        DwelltimeBigrams = 21,
        CtrEngSsHard = 22,
        DwelltimeBigramsRelevance = 23,
        FpsSpylogAggregatedQueryPart = 25,
        ReformulationsLongestClickLogDt = 26,
        QueryToImageV8Layer = 27,
        QueryExtensions = 28,
        ReformulationsQueryEmbedderMini = 29,
        BertDistillL2 = 30,
        SinsigL2 = 31,
        FullSplitBert = 32
    };

    struct TTypeEmbeddingPair {
        EDssmModel Type;
        TString EncodedEmbedding;

        TTypeEmbeddingPair(EDssmModel type, TString encodedEmbedding)
            : Type(type)
            , EncodedEmbedding(std::move(encodedEmbedding))
        {
        }
    };

    const TVector<EDssmModel> GetBegemotProductionModels();
    const TVector<EDssmModel> GetZeroEmbeddingProductionModels();
    const TString GetModelQueryEmbeddingName(EDssmModel type);
    const TString GetModelEmbeddingSizeName(EDssmModel type);

    class IBegemotDssmData {
    public:
        virtual void AddData(TVersionRange supportedVersions, TVector<float> embedding) = 0;
        virtual bool TryGetEmbedding(ui32 preferredVersion, TVector<float>& result) const = 0;
        virtual bool TryGetLatestEmbedding(TVector<float>& result) const = 0;
        virtual void Save(IOutputStream* s) const = 0;
        virtual void Load(IInputStream* s) = 0;
        virtual ~IBegemotDssmData() {}
    };

    using IBegemotDssmDataPtr = THolder<IBegemotDssmData>;

    IBegemotDssmDataPtr GetDssmDataSerializer(EDssmModel type);

    size_t GetDefaultEmbeddingSize(EDssmModel type, ui64 version = 0);

} // namespace NNeuralNetApplier
