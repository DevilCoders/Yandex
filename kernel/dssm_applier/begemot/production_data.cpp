#include "production_data.h"
#include <kernel/dssm_applier/utils/utils.h>
#include <util/string/cast.h>
#include <util/ysaveload.h>

namespace NNeuralNetApplier {

    const TVector<EDssmModel> GetBegemotProductionModels() {
        return {
            EDssmModel::Multiclick,
            EDssmModel::CtrNoMiner,
            EDssmModel::MulticosStreams,
            EDssmModel::LogDtBigramsAMHardQueriesNoClicks,
            EDssmModel::LogDtBigramsAMHardQueriesNoClicksMixed,
            EDssmModel::CtrEngSsHard,
            EDssmModel::DwelltimeBigramsRelevance,
            EDssmModel::ReformulationsLongestClickLogDt,
            EDssmModel::QueryExtensions
        };
    }

    const TVector<EDssmModel> GetZeroEmbeddingProductionModels() {
        return {
            EDssmModel::DwellTimeRegChainEmbedding,
            EDssmModel::QueryWordTitleEmbedding,
        };
    }

    const TString GetModelQueryEmbeddingName(EDssmModel type) {
        switch (type) {
            case EDssmModel::Multiclick:
                return "query_embedding_multiclick";
            case EDssmModel::Uta:
                return "query_embedding_uta";
            case EDssmModel::CtrNoMiner:
                return "query_embedding_ctr_no_miner";
            case EDssmModel::MulticosStreams:
                return "query_embedding_multicos_streams";
            case EDssmModel::LogDwellTimeBigrams:
                return "query_embedding_log_dt_bigrams";
            case EDssmModel::MainContentKeywords:
                return "query_embedding_main_content_keywords";
            case EDssmModel::LogDtBigramsAMHardQueriesNoClicks:
                return "query_embedding_log_dt_bigrams_am_hard_queries_no_clicks";
            case EDssmModel::LogDtBigramsAMHardQueriesNoClicksMixed:
                return "query_embedding_log_dt_bigrams_am_hard_queries_no_clicks_mixed";
            case EDssmModel::DwellTimeRegChainEmbedding:
                return "query_embedding_dwelltime_reg_chain_embedding";
            case EDssmModel::QueryWordTitleEmbedding:
                return "query_embedding_query_word_title_embedding";
            case EDssmModel::BoostingCtr:
                return "query_embedding_boosting_ctr";
            case EDssmModel::BoostingXfOne:
                return "query_embedding_boosting_xf_one";
            case EDssmModel::BoostingXfOneSE:
                return "query_embedding_boosting_xf_one_se";
            case EDssmModel::BoostingXfOneSeAmSsHard:
                return "query_embedding_boosting_xf_one_se_am_ss_hard";
            case EDssmModel::BoostingXfWtd:
                return "query_embedding_boosting_xf_wtd";
            case EDssmModel::LogDwellTime50Tokenized:
                return "query_embedding_log_dt_50_tokenized";
            case EDssmModel::BoostingSerpSimilarityHard:
                return "query_embedding_boosting_serp_similarity_hard";
            case EDssmModel::CtrEngSsHard:
                return "query_embedding_ctr_eng_ss_hard";
            case EDssmModel::DwelltimeBigramsRelevance:
                return "query_embedding_dwelltime_bigrams_relevance";
            case EDssmModel::ReformulationsLongestClickLogDt:
                return "query_embedding_reformulations_longest_click_log_dt";
            case EDssmModel::QueryExtensions:
                return "query_embedding_with_extensions";
            default:
                ythrow yexception() << "DSSM model doesn't support GetModelQueryEmbeddingName()";
        }
    }

    template <typename DataType>
    class TBegemotDssmData : public IBegemotDssmData {
    private:
        class TModelEmbedding {
        private:
            TVersionRange SupportedVersions;
            TVector<DataType> Data;

        public:
            TModelEmbedding() = default;
            TModelEmbedding(TVersionRange versions, TVector<DataType> data)
                : SupportedVersions(std::move(versions))
                , Data(std::move(data))
            {
            }
            const TVector<DataType>& GetData() const {
                return Data;
            }
            bool SupportsVersion(ui32 version) const {
                return SupportedVersions.Contains(version);
            }

            inline void Save(IOutputStream* s) const {
                ::SaveMany(s, SupportedVersions, Data);
            }

            inline void Load(IInputStream* s) {
                ::Load(s, SupportedVersions);
                const size_t dataSize = ::LoadSize(s);
                Y_ENSURE(dataSize < 1024, "Huge load size for DssmData: " + ToString(dataSize));
                Data.resize(dataSize);
                ::LoadRange(s, Data.begin(), Data.end());
            }
        };

    private:
        TVector<TModelEmbedding> Embeddings;
        EDssmModel Type = EDssmModel::Unknown;

    public:
        TBegemotDssmData(EDssmModel type)
            : Type(type)
        {
        }
        void AddData(TVersionRange supportedVersions, TVector<float> embedding) override {
            Y_ENSURE(Type != EDssmModel::Unknown, "Model type shouldn't be 'Unknown'");
            Embeddings.emplace_back(std::move(supportedVersions), ToSerializedVector(std::move(embedding)));
        }
        bool TryGetEmbedding(ui32 preferredVersion, TVector<float>& result) const override {
            Y_ENSURE(Type != EDssmModel::Unknown, "Model type shouldn't be 'Unknown'");
            const auto it = std::find_if(Embeddings.begin(), Embeddings.end(), [preferredVersion](const auto& emb) {
                return emb.SupportsVersion(preferredVersion);
            });

            if (Y_UNLIKELY(it == Embeddings.end())) {
                return false;
            }

            result = ToFloatVector(it->GetData());
            return true;
        }
        bool TryGetLatestEmbedding(TVector<float>& result) const override {
            Y_ENSURE(Type != EDssmModel::Unknown, "Model type shouldn't be 'Unknown'");
            if (Y_LIKELY(Embeddings)) {
                result = ToFloatVector(Embeddings.front().GetData());
                return true;
            }
            return false;
        }
        void Save(IOutputStream* s) const override {
            ::Save(s, Embeddings);
            ::Save(s, Type);
        }
        void Load(IInputStream* s) override {
            const size_t embeddingsSize = ::LoadSize(s);
            Y_ENSURE(embeddingsSize < 1024, "Huge load size for embeddings: " + ToString(embeddingsSize));
            Embeddings.resize(embeddingsSize);
            ::LoadRange(s, Embeddings.begin(), Embeddings.end());
            EDssmModel type;
            ::Load(s, type);
            Y_ENSURE(type == Type, "Trying to load: " + ToString(Type) + ", actual type: " + ToString(type));
        }

        static TVector<DataType> ToSerializedVector(TVector<float> vect);
        static TVector<float> ToFloatVector(TVector<DataType> vect);
    };

    template <>
    TVector<ui8> TBegemotDssmData<ui8>::ToSerializedVector(TVector<float> vect) {
        return NDssmApplier::NUtils::TFloat2UI8Compressor::Compress(vect);
    }

    template <>
    TVector<float> TBegemotDssmData<ui8>::ToFloatVector(TVector<ui8> vect) {
        return NDssmApplier::NUtils::TFloat2UI8Compressor::Decompress(vect);
    }

    template <>
    TVector<float> TBegemotDssmData<float>::ToSerializedVector(TVector<float> vect) {
        return vect;
    }

    template <>
    TVector<float> TBegemotDssmData<float>::ToFloatVector(TVector<float> vect) {
        return vect;
    }

    THolder<IBegemotDssmData> GetDssmDataSerializer(EDssmModel type) {
        using NNeuralNetApplier::EDssmModel;
        switch (type) {
            case EDssmModel::Multiclick:
            case EDssmModel::Uta:
            case EDssmModel::CtrEngSsHard:
            case EDssmModel::CtrNoMiner:
            case EDssmModel::DwelltimeBigramsRelevance:
            case EDssmModel::MulticosStreams:
            case EDssmModel::BoostingXfOne:
            case EDssmModel::BoostingXfOneSE:
            case EDssmModel::BoostingXfOneSEHard:
            case EDssmModel::BoostingCtr:
            case EDssmModel::BoostingXfWtd:
            case EDssmModel::LogDwellTime50Tokenized:
            case EDssmModel::LogDwellTimeBigrams:
            case EDssmModel::MainContentKeywords:
            case EDssmModel::LogDtBigramsAMHardQueriesNoClicks:
            case EDssmModel::LogDtBigramsAMHardQueriesNoClicksMixed:
            case EDssmModel::BoostingXfOneSeAmSsHard:
            case EDssmModel::PantherTerms:
            case EDssmModel::BoostingSerpSimilarityHard:
            case EDssmModel::ReformulationsLongestClickLogDt:
            case EDssmModel::QueryExtensions:
                return MakeHolder<TBegemotDssmData<ui8>>(type);
            case EDssmModel::NNOverDssm:
                return MakeHolder<TBegemotDssmData<float>>(type);
            default:
                ythrow yexception() << "Unknown DSSM production model type";
        }
    }

    size_t GetDefaultEmbeddingSize(EDssmModel type, ui64 version) {
        switch (type) {
            case EDssmModel::LogDtBigramsAMHardQueriesNoClicks:
            case EDssmModel::LogDtBigramsAMHardQueriesNoClicksMixed:
            case EDssmModel::BoostingCtr:
                return 25;
            case EDssmModel::CtrEngSsHard:
            case EDssmModel::DwelltimeBigramsRelevance:
            case EDssmModel::Multiclick:
            case EDssmModel::LogDwellTime50Tokenized:
            case EDssmModel::LogDwellTimeBigrams:
                return 50;
            case EDssmModel::CtrNoMiner:
                return version == 0 ? 40 : 50;
            case EDssmModel::MulticosStreams:
                return 75;
            case EDssmModel::DwellTimeRegChainEmbedding:
                return 30;
            case EDssmModel::QueryWordTitleEmbedding:
                return 100;
            case EDssmModel::MainContentKeywords:
            case EDssmModel::BoostingXfWtd:
            case EDssmModel::BoostingXfOneSE:
            case EDssmModel::BoostingXfOneSeAmSsHard:
            case EDssmModel::BertDistillL2:
                return 40;
            case EDssmModel::DwelltimeBigrams:
                return 300;
            case EDssmModel::PantherTerms:
                return 72;
            case EDssmModel::ReformulationsLongestClickLogDt:
            case EDssmModel::QueryExtensions:
                return 120;
            case EDssmModel::SinsigL2:
            case EDssmModel::FullSplitBert:
                return 64;
            default:
                ythrow yexception() << "DSSM model doesn't support GetDefaultEmbeddingSize() " << ToString(type);
        }
    }

} // namespace NNeuralNetApplier
