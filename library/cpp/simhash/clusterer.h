#pragma once

#include "compbittrie.h"
#include "math_util.h"
#include "time_util.h"
#include "clusterer_stats.h"

#include <util/generic/vector.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/generic/utility.h>
#include <util/generic/yexception.h>
#include <util/generic/set.h>

#include <util/datetime/base.h>
#include <util/stream/output.h>

#include <util/system/defaults.h>
#include <util/system/yassert.h>

#include <util/generic/bt_exception.h>

#include <memory.h>

namespace NNearDuplicates {
#pragma pack(push, 1)

    struct TClustererConfig {
        enum EClusteringAlgorithm {
            CA_Auto = 0,
            CA_SmallGroup = 1,
            CA_NeighborsFullBruteForce = 2,
            CA_NeighborsExactBruteForce = 3,
            CA_OnlyByMainrankBruteForce = 4,
            CA_NoSortingBruteForce = 5,
            CA_NeighborsFullBitTrie = 6,
            CA_NeighborsExactBitTrie = 7,
            CA_OnlyByMainrankBitTrie = 8,
            CA_NoSortingBitTrie = 9
        };

        struct TDocumentCompareConfig {
            static TDocumentCompareConfig Strong() {
                return TDocumentCompareConfig(1, 0.08f);
            }

            static TDocumentCompareConfig Normal() {
                return TDocumentCompareConfig(3, 0.1f);
            }

            static TDocumentCompareConfig Weak() {
                return TDocumentCompareConfig(5, 0.12f);
            }

            ui32 SimhashTreshold;
            float LengthTreshold;

            TDocumentCompareConfig()
                : SimhashTreshold(3)
                , LengthTreshold(0.1f)
            {
            }

            TDocumentCompareConfig(
                ui32 simhashTreshold,
                float lengthTreshold)
                : SimhashTreshold(simhashTreshold)
                , LengthTreshold(lengthTreshold)
            {
            }
        };

        class TRegroupingConfig {
        public:
            static TRegroupingConfig No() {
                return TRegroupingConfig(0).SetNo();
            }

            static TRegroupingConfig Weak() {
                return TRegroupingConfig(100000).SetWeak();
            }

            static TRegroupingConfig Normal() {
                return TRegroupingConfig(1000000).SetNormal();
            }

            static TRegroupingConfig Strong() {
                return TRegroupingConfig(10000000).SetStrong();
            }

            static TRegroupingConfig Full() {
                return TRegroupingConfig(1u << 31).SetFull();
            }

        public:
            size_t MaxRegroupableHypothesisSize;

        public:
            TRegroupingConfig()
                : MaxRegroupableHypothesisSize(100000)
            {
                SetWeak();
            }

            TRegroupingConfig(size_t maxRegroupableHypothesisSize)
                : MaxRegroupableHypothesisSize(maxRegroupableHypothesisSize)
            {
                SetWeak();
            }

        public:
            size_t GetSteps(const size_t size) const {
                if (size <= Sizes[0]) {
                    return Steps[0];
                } else if (size <= Sizes[1]) {
                    return Steps[1];
                } else if (size <= Sizes[2]) {
                    return Steps[2];
                } else if (size <= Sizes[3]) {
                    return Steps[3];
                } else {
                    return Steps[4];
                }
                ythrow TWithBackTrace<yexception>() << "Unreachable code reached. Your program is wrong";
            }

        private:
            TRegroupingConfig& SetNo() {
                Steps[0] = Steps[1] = Steps[2] = Steps[3] = Steps[4] = 0;
                Sizes[0] = Sizes[1] = Sizes[2] = Sizes[3] = 0;

                return *this;
            }

            TRegroupingConfig& SetWeak() {
                Steps[0] = 3;
                Steps[1] = 2;
                Steps[2] = 1;
                Steps[3] = 0;
                Steps[4] = 0;

                Sizes[0] = 64;
                Sizes[1] = 128;
                Sizes[2] = 256;
                Sizes[3] = 512;

                return *this;
            }

            TRegroupingConfig& SetNormal() {
                Steps[0] = 8;
                Steps[1] = 4;
                Steps[2] = 2;
                Steps[3] = 1;
                Steps[4] = 0;

                Sizes[0] = 128;
                Sizes[1] = 256;
                Sizes[2] = 512;
                Sizes[3] = 1024;

                return *this;
            }

            TRegroupingConfig& SetStrong() {
                Steps[0] = 8;
                Steps[1] = 4;
                Steps[2] = 2;
                Steps[3] = 1;
                Steps[4] = 0;

                Sizes[0] = 512;
                Sizes[1] = 1024;
                Sizes[2] = 2048;
                Sizes[3] = 4096;

                return *this;
            }

            TRegroupingConfig& SetFull() {
                Steps[0] = Steps[1] = Steps[2] = Steps[3] = Steps[4] = 128;
                Sizes[0] = Sizes[1] = Sizes[2] = Sizes[3] = 1u << 31;

                return *this;
            }

        private:
            size_t Steps[5];
            size_t Sizes[4];
        };

        struct TAlgorithmSelectionConfig {
            static TAlgorithmSelectionConfig _1GB() {
                return TAlgorithmSelectionConfig(
                    80000,
                    10000,
                    15000,
                    12000,
                    200000,
                    350000,
                    500000,
                    13000000);
            }

            static TAlgorithmSelectionConfig _2GB() {
                return TAlgorithmSelectionConfig(
                    80000,
                    10000,
                    15000,
                    12000,
                    200000,
                    550000,
                    900000,
                    26000000);
            }

            static TAlgorithmSelectionConfig _3GB() {
                return TAlgorithmSelectionConfig(
                    80000,
                    10000,
                    15000,
                    12000,
                    200000,
                    750000,
                    1300000,
                    39000000);
            }

            static TAlgorithmSelectionConfig _4GB() {
                return TAlgorithmSelectionConfig(
                    80000,
                    10000,
                    15000,
                    12000,
                    200000,
                    1000000,
                    1800000,
                    52000000);
            }

            static TAlgorithmSelectionConfig _5GB() {
                return TAlgorithmSelectionConfig(
                    80000,
                    10000,
                    15000,
                    12000,
                    200000,
                    1000000,
                    1800000,
                    65000000);
            }

            static TAlgorithmSelectionConfig _6GB() {
                return TAlgorithmSelectionConfig(
                    80000,
                    10000,
                    15000,
                    12000,
                    200000,
                    1400000,
                    2600000,
                    78000000);
            }

            static TAlgorithmSelectionConfig _7GB() {
                return TAlgorithmSelectionConfig(
                    80000,
                    10000,
                    15000,
                    12000,
                    200000,
                    1650000,
                    3100000,
                    91000000);
            }

            size_t BruteForceBitTrieTradeoffForFullNeighbors;
            size_t BruteForceBitTrieTradeoffForExactNeighbors;
            size_t BruteForceBitTrieTradeoffForMainrank;
            size_t BruteForceBitTrieTradeoffForNoSorting;

            size_t MaxFullNeighborsHypothesisSize;
            size_t MaxExactNeighborsHypothesisSize;
            size_t MaxMainrankHypothesisSize;
            size_t MaxNoSortingHypothesisSize;

            TAlgorithmSelectionConfig(
                size_t bruteForceBitTrieTradeoffForFullNeighbors = 80000,
                size_t bruteForceBitTrieTradeoffForExactNeighbors = 10000,
                size_t bruteForceBitTrieTradeoffForMainrank = 15000,
                size_t bruteForceBitTrieTradeoffForNoSorting = 12000,
                size_t maxFullNeighborsHypothesisSize = 200000,
                size_t maxExactNeighborsHypothesisSize = 1650000,
                size_t maxMainrankHypothesisSize = 3100000,
                size_t maxNoSortingHypothesisSize = 91000000)
                : BruteForceBitTrieTradeoffForFullNeighbors(bruteForceBitTrieTradeoffForFullNeighbors)
                , BruteForceBitTrieTradeoffForExactNeighbors(bruteForceBitTrieTradeoffForExactNeighbors)
                , BruteForceBitTrieTradeoffForMainrank(bruteForceBitTrieTradeoffForMainrank)
                , BruteForceBitTrieTradeoffForNoSorting(bruteForceBitTrieTradeoffForNoSorting)
                , MaxFullNeighborsHypothesisSize(maxFullNeighborsHypothesisSize)
                , MaxExactNeighborsHypothesisSize(maxExactNeighborsHypothesisSize)
                , MaxMainrankHypothesisSize(maxMainrankHypothesisSize)
                , MaxNoSortingHypothesisSize(maxNoSortingHypothesisSize)
            {
            }
        };

        EClusteringAlgorithm ClusteringAlgorithm;
        TDocumentCompareConfig CompareConfig;
        TRegroupingConfig RegroupingConfig;
        TAlgorithmSelectionConfig AlgorithmSelectionConfig;

        TClustererConfig()
            : ClusteringAlgorithm(CA_Auto)
            , CompareConfig(TDocumentCompareConfig::Normal())
            , RegroupingConfig(TRegroupingConfig::Weak())
            , AlgorithmSelectionConfig(TAlgorithmSelectionConfig::_2GB())
        {
        }

        inline void SaveToVector(TVector<char>* data);
        inline void LoadFromVector(const TVector<char>& data);

        static EClusteringAlgorithm SelectAlgorithm(const TClustererConfig& config, const size_t groupSize) {
            if (Y_UNLIKELY(config.ClusteringAlgorithm != TClustererConfig::CA_Auto)) {
                ythrow TWithBackTrace<yexception>() << "Algorithm selecting allowed only in CA_Auto mode. Your program is wrong";
            }
            if (groupSize <= config.AlgorithmSelectionConfig.MaxFullNeighborsHypothesisSize) {
                if (groupSize < config.AlgorithmSelectionConfig.BruteForceBitTrieTradeoffForFullNeighbors) {
                    return TClustererConfig::CA_NeighborsFullBruteForce;
                } else {
                    return TClustererConfig::CA_NeighborsFullBitTrie;
                }
            } else if (groupSize <= config.AlgorithmSelectionConfig.MaxExactNeighborsHypothesisSize) {
                if (groupSize < config.AlgorithmSelectionConfig.BruteForceBitTrieTradeoffForExactNeighbors) {
                    return TClustererConfig::CA_NeighborsExactBruteForce;
                } else {
                    return TClustererConfig::CA_NeighborsExactBitTrie;
                }
            } else if (groupSize <= config.AlgorithmSelectionConfig.MaxMainrankHypothesisSize) {
                if (groupSize < config.AlgorithmSelectionConfig.BruteForceBitTrieTradeoffForMainrank) {
                    return TClustererConfig::CA_OnlyByMainrankBruteForce;
                } else {
                    return TClustererConfig::CA_OnlyByMainrankBitTrie;
                }
            } else if (groupSize <= config.AlgorithmSelectionConfig.MaxNoSortingHypothesisSize) {
                if (groupSize < config.AlgorithmSelectionConfig.BruteForceBitTrieTradeoffForNoSorting) {
                    return TClustererConfig::CA_NoSortingBruteForce;
                } else {
                    return TClustererConfig::CA_NoSortingBitTrie;
                }
            } else {
                ythrow TWithBackTrace<yexception>() << "Unexpected online algorithm. Your program is wrong";
            }
        }
    };

    inline void TClustererConfig::SaveToVector(TVector<char>* data) {
        data->clear();
        data->resize(sizeof(TClustererConfig), 0);
        memcpy(&data->at(0), this, sizeof(TClustererConfig));
    }

    inline void TClustererConfig::LoadFromVector(const TVector<char>& data) {
        if (data.size() != sizeof(TClustererConfig)) {
            ythrow TWithBackTrace<yexception>() << "Wrong deserialization data";
        }
        memcpy(this, &data.at(0), sizeof(TClustererConfig));
    }

#pragma pack(pop)

    template <typename TSimhash>
    struct TDocumentSketch {
        TSimhash Simhash;
        ui32 DocLength;

        TDocumentSketch(const TSimhash& simhash, ui32 docLength)
            : Simhash(simhash)
            , DocLength(docLength)
        {
        }
    };

    template <typename TSimhash>
    inline bool CheckPair(
        const TDocumentSketch<TSimhash>& left,
        const TDocumentSketch<TSimhash>& right,
        const ui32 simhashTreshold,
        const float lengthTreshold) {
        return (HammingDistance(left.Simhash, right.Simhash) <= simhashTreshold) &&
               (DifferenceRatio(left.DocLength, right.DocLength) <= lengthTreshold);
    }

    // Document data aggregator
    template <typename TSimhashType, typename TMainrankType, typename TPayloadType>
    struct TDocument {
        typedef TSimhashType TSimhash;
        typedef TMainrankType TMainrank;
        typedef TPayloadType TPayload;
        typedef TDocumentSketch<TSimhash> TSketch;

        TSketch Sketch;
        TMainrank Mainrank;
        mutable TPayload Payload;
        mutable ui32 Temprank;

        TDocument(const TDocumentSketch<TSimhash>& sketch, const TMainrank& mainrank, const TPayload& payload)
            : Sketch(sketch)
            , Mainrank(mainrank)
            , Payload(payload)
            , Temprank(0)
        {
        }
    };

    /////////////////////////////////////////////////////////////////////
    // Interface hints
    template <typename TSimhash, typename TMainrank, typename TPayload>
    struct THandlerInterface: private TNonCopyable {
        typedef TDocument<TSimhash, TMainrank, TPayload> TDoc;

        void OnHypothesisStart(bool online);
        void OnUnprocessedDocument(const TDoc& doc);
        void OnDocument(const TDoc& doc, const TDoc* mainDoc);
        void OnHypothesisEnd();
    };

    template <
        typename TSimhash,
        typename TMainrank,
        typename TPayload,
        bool CheckInvariants,
        bool GatherMemStat>
    struct TDefaultHandler: public THandlerInterface<TSimhash, TMainrank, TPayload> {
        typedef TDocument<TSimhash, TMainrank, TPayload> TDoc;

        TDefaultHandler(const TClustererConfig& config)
            : Config(config)
            , HypothesisCount(0)
            , InHypothesis(false)
            , UnprocessedCount(0)
            , ProcessedCount(0)
            , MemStat()
        {
        }

        void OnHypothesisStart() {
            ++HypothesisCount;

            if (Y_UNLIKELY(InHypothesis)) {
                ythrow TWithBackTrace<yexception>() << "InHypothesis must be false. Your program is wrong";
            }

            InHypothesis = true;
            UnprocessedCount = 0;
            ProcessedCount = 0;
        }

        void OnUnprocessedDocument(const TDoc& /*doc*/) {
            if (Y_UNLIKELY(!InHypothesis)) {
                ythrow TWithBackTrace<yexception>() << "InHypothesis must be true. Your program is wrong";
            }

            ++UnprocessedCount;
        }

        void OnDocument(const TDoc& doc, const TDoc* mainDoc) {
            if (Y_UNLIKELY(!InHypothesis)) {
                ythrow TWithBackTrace<yexception>() << "InHypothesis must be true. Your program is wrong";
            }

            if (Y_UNLIKELY(&doc == mainDoc)) {
                ythrow TWithBackTrace<yexception>() << "doc == mainDoc. Your program is wrong";
            }

            if (CheckInvariants && mainDoc != nullptr) {
                if (Y_UNLIKELY(!CheckPair(
                        doc.Sketch,
                        mainDoc->Sketch,
                        Config.CompareConfig.SimhashTreshold,
                        Config.CompareConfig.LengthTreshold)))
                {
                    ythrow TWithBackTrace<yexception>() << "Wrong duplicates. Your program is wrong";
                }
                if (Y_UNLIKELY(doc.Mainrank > mainDoc->Mainrank)) {
                    ythrow TWithBackTrace<yexception>() << "Wrong clusterization. Your program is wrong";
                }
            }

            ++ProcessedCount;
        }

        void OnHypothesisEnd() {
            if (Y_UNLIKELY(!InHypothesis)) {
                ythrow TWithBackTrace<yexception>() << "InHypothesis must be true. Your program is wrong";
            }

            if (Y_UNLIKELY(ProcessedCount != UnprocessedCount)) {
                ythrow TWithBackTrace<yexception>() << "ProcessedCount != UnprocessedCount. Your program is wrong";
            }

            InHypothesis = false;

            if (GatherMemStat && (ProcessedCount >= 1000)) {
                MemStat.Add(ProcessedCount);
            }
        }

        void ResetAfterClustererCrash() {
            InHypothesis = false;
            UnprocessedCount = 0;
            ProcessedCount = 0;
        }

    public:
        ui64 GetHypothesisCount() const {
            return HypothesisCount;
        }

        ui64 GetHypothesisSize() const {
            return ProcessedCount;
        }

        const TMemStat& GetMemStat() const {
            return MemStat;
        }

    private:
        const TClustererConfig& Config;

        ui64 HypothesisCount;

        bool InHypothesis;
        ui64 UnprocessedCount;
        ui64 ProcessedCount;

        TMemStat MemStat;
    };

    namespace NPrivate {
        template <typename TDoc, bool Regroup, bool CorrectNeighbors>
        struct TSortingAlgorithmInterface {
            enum {
                NeedRegroupByMainrank = (Regroup ? 1 : 0)
            };

            enum {
                CorrectNeighborCount = (CorrectNeighbors ? 1 : 0)
            };

            static void Sort(TVector<const TDoc*>* hypothesis) {
                ::Sort(hypothesis->begin(), hypothesis->end(), Compare);
            }

        private:
            static bool Compare(const TDoc* left, const TDoc* right) {
                return right->Temprank < left->Temprank;
            }
        };

        template <typename TDoc>
        struct TSearchingAlgorithmInterface: private TNonCopyable {
            TSearchingAlgorithmInterface(
                const TClustererConfig& /*config*/,
                const TVector<const TDoc*>& /*hypothesis*/,
                const TVector<bool>& /*processed*/) {
            }

            void PostLoad();
            size_t GetNeighborCountFull(size_t mainIndex) const;
            size_t GetNeighborCountExact(size_t mainIndex) const;
            void FindNeighbors(size_t mainIndex, TVector<size_t>* groupIndexes);
            void Remove(const TVector<size_t>& indexes);
            size_t Count() const;
        };
    }
    //
    /////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////
    // Internal algorithms
    namespace NPrivate {
        template <typename TDoc>
        struct TNeighborsFull: public TSortingAlgorithmInterface<TDoc, true, true> {
            using TSortingAlgorithmInterface<TDoc, true, true>::NeedRegroupByMainrank;
            using TSortingAlgorithmInterface<TDoc, true, true>::CorrectNeighborCount;

            template <typename TSearch>
            static void Sort(
                const TSearch& search,
                TVector<const TDoc*>* hypothesis) {
                const size_t s = hypothesis->size();
                for (size_t i = 0; i < s; ++i) {
                    hypothesis->operator[](i)->Temprank = search.GetNeighborCountFull(i) - 1;
                }
                TSortingAlgorithmInterface<TDoc, true, true>::Sort(hypothesis);
            }
        };

        template <typename TDoc>
        struct TNeighborsExact: public TSortingAlgorithmInterface<TDoc, true, false> {
            using TSortingAlgorithmInterface<TDoc, true, false>::NeedRegroupByMainrank;
            using TSortingAlgorithmInterface<TDoc, true, false>::CorrectNeighborCount;

            template <typename TSearch>
            static void Sort(
                const TSearch& search,
                TVector<const TDoc*>* hypothesis) {
                const size_t s = hypothesis->size();
                for (size_t i = 0; i < s; ++i) {
                    hypothesis->operator[](i)->Temprank = search.GetNeighborCountExact(i) - 1;
                }
                TSortingAlgorithmInterface<TDoc, true, false>::Sort(hypothesis);
            }
        };

        template <typename TDoc>
        struct TOnlyByMainrank: public TSortingAlgorithmInterface<TDoc, false, false> {
            using TSortingAlgorithmInterface<TDoc, false, false>::NeedRegroupByMainrank;
            using TSortingAlgorithmInterface<TDoc, false, false>::CorrectNeighborCount;

            template <typename TSearch>
            static void Sort(
                const TSearch& /*search*/,
                TVector<const TDoc*>* hypothesis) {
                ::Sort(hypothesis->begin(), hypothesis->end(), Compare);
            }

        private:
            static bool Compare(const TDoc* left, const TDoc* right) {
                return right->Mainrank < left->Mainrank;
            }
        };

        template <typename TDoc>
        struct TNoSorting: public TSortingAlgorithmInterface<TDoc, true, false> {
            using TSortingAlgorithmInterface<TDoc, true, false>::NeedRegroupByMainrank;
            using TSortingAlgorithmInterface<TDoc, true, false>::CorrectNeighborCount;

            template <typename TSearch>
            static void Sort(
                const TSearch& /*search*/,
                TVector<const TDoc*>* /*hypothesis*/) {
                // Nothing to do here
            }
        };

        template <typename TDoc>
        struct TBruteForceSearch: public TSearchingAlgorithmInterface<TDoc> {
            TBruteForceSearch(
                TClustererConfig& config,
                const TVector<const TDoc*>& hypothesis,
                const TVector<bool>& processed)
                : TSearchingAlgorithmInterface<TDoc>(config, hypothesis, processed)
                , Config(config)
                , Hypothesis(hypothesis)
                , Processed(processed)
                , Unprocessed(hypothesis.size())
            {
            }

        public:
            void PostLoad() {
                // Nothing to do here
            }

            size_t GetNeighborCountFull(size_t mainIndex) const {
                size_t res = 0;
                const size_t s = Hypothesis.size();
                const TDoc& main = *Hypothesis[mainIndex];
                for (size_t i = 0; i < s; ++i) {
                    if (HammingDistance(main.Sketch.Simhash, Hypothesis[i]->Sketch.Simhash) <= Config.CompareConfig.SimhashTreshold) {
                        ++res;
                    }
                }
                if (res == 0) {
                    ythrow TWithBackTrace<yexception>() << "No neighbors. Your program is wrong";
                }
                return res;
            }

            size_t GetNeighborCountExact(size_t mainIndex) const {
                size_t res = 0;
                const size_t s = Hypothesis.size();
                const TDoc& main = *Hypothesis[mainIndex];
                for (size_t i = 0; i < s; ++i) {
                    if (main.Sketch.Simhash == Hypothesis[i]->Sketch.Simhash) {
                        ++res;
                    }
                }
                if (res == 0) {
                    ythrow TWithBackTrace<yexception>() << "No neighbors. Your program is wrong";
                }
                return res;
            }

            void FindNeighbors(size_t mainIndex, TVector<size_t>* groupIndexes) {
                if (Y_UNLIKELY(Processed[mainIndex])) {
                    ythrow TWithBackTrace<yexception>() << "Main is processed. Your program is wrong";
                }
                groupIndexes->clear();
                if (Unprocessed == 0) {
                    return;
                }
                const size_t s = Hypothesis.size();
                const TDoc& main = *Hypothesis[mainIndex];
                for (size_t i = 0; i < s; ++i) {
                    if (Processed[i]) {
                        continue;
                    }
                    if (i == mainIndex) {
                        continue;
                    }
                    if (CheckPair(main, i)) {
                        groupIndexes->push_back(i);
                    }
                }
            }

            void Remove(const TVector<size_t>& indexes) {
                const size_t s = indexes.size();
                if (Y_UNLIKELY(s > Unprocessed)) {
                    ythrow TWithBackTrace<yexception>() << "Too much for remove. Your program is wrong";
                }
                Unprocessed -= s;
            }

            size_t Count() const {
                return Unprocessed;
            }

        private:
            inline bool CheckPair(const TDoc& main, const size_t right) {
                if (Y_UNLIKELY(Processed[right])) {
                    ythrow TWithBackTrace<yexception>() << "Touching processed document. Your program is wrong";
                }
                return (HammingDistance(main.Sketch.Simhash, Hypothesis[right]->Sketch.Simhash) <= Config.CompareConfig.SimhashTreshold) &&
                       (DifferenceRatio(main.Sketch.DocLength, Hypothesis[right]->Sketch.DocLength) <= Config.CompareConfig.LengthTreshold);
            }

        private:
            TClustererConfig& Config;
            const TVector<const TDoc*>& Hypothesis;
            const TVector<bool>& Processed;
            size_t Unprocessed;
        };

        template <typename TDoc>
        class TBitTrieSearch: public TSearchingAlgorithmInterface<TDoc> {
            typedef typename TDoc::TSimhash TSimhash;

        public:
            TBitTrieSearch(
                TClustererConfig& config,
                const TVector<const TDoc*>& hypothesis,
                const TVector<bool>& processed)
                : TSearchingAlgorithmInterface<TDoc>(config, hypothesis, processed)
                , Config(config)
                , Hypothesis(hypothesis)
                , Processed(processed)
                , SortedBySimhash()
                , BackMap()
                , Unprocessed(hypothesis.size())
                , BitTrie()
                , TempGroup()
            {
                const size_t s = Hypothesis.size();
                for (size_t i = 0; i < s; ++i) {
                    std::pair<ui32, ui32>* group;
                    BitTrie.Insert(Hypothesis[i]->Sketch.Simhash,
                                   std::pair<ui32, ui32>(0, 0),
                                   &group);
                    group->second = group->second + 1;
                }
            }

        public:
            void PostLoad() {
                const size_t s = Hypothesis.size();
                SortedBySimhash.reserve(s);
                for (size_t i = 0; i < s; ++i) {
                    SortedBySimhash.push_back((ui32)i);
                }
                ::Sort(SortedBySimhash.begin(), SortedBySimhash.end(), TCompareBySimhash(Hypothesis));
                BackMap.resize(s, 0xFFFFFFFF);

                ui32 groupStart = 0;
                ui32 groupSize = 0;
                TSimhash simhash = 0;
                for (ui32 i = 0; i < s; ++i) {
                    const TDoc* doc = Hypothesis[SortedBySimhash[i]];
                    if (BackMap[SortedBySimhash[i]] != 0xFFFFFFFF) {
                        ythrow TWithBackTrace<yexception>() << "Inconsistent state";
                    }
                    BackMap[SortedBySimhash[i]] = i;
                    if (groupSize == 0) {
                        groupStart = i;
                        groupSize = 1;
                        simhash = doc->Sketch.Simhash;
                    } else {
                        if (simhash != doc->Sketch.Simhash) {
                            std::pair<ui32, ui32>& group = BitTrie.Get(simhash);
                            group.first = groupStart;
                            if (group.second != groupSize) {
                                ythrow TWithBackTrace<yexception>() << "Inconsistent state";
                            }

                            groupStart = i;
                            groupSize = 1;
                            simhash = doc->Sketch.Simhash;
                        } else {
                            ++groupSize;
                        }
                    }
                }
                if (groupSize == 0) {
                    ythrow TWithBackTrace<yexception>() << "empty group";
                }
                std::pair<ui32, ui32>& group = BitTrie.Get(simhash);
                group.first = groupStart;
                if (group.second != groupSize) {
                    ythrow TWithBackTrace<yexception>() << "Inconsistent state";
                }
            }

            size_t GetNeighborCountFull(size_t mainIndex) const {
                size_t res = 0;
                BitTrie.GetNeighbors(
                    Hypothesis[mainIndex]->Sketch.Simhash,
                    Config.CompareConfig.SimhashTreshold,
                    &TempGroup);
                const size_t s = TempGroup.size();
                for (size_t i = 0; i < s; ++i) {
                    if (Y_UNLIKELY(TempGroup[i]->second == 0)) {
                        ythrow TWithBackTrace<yexception>() << "No neighbors. Your program is wrong";
                    }
                    res += TempGroup[i]->second;
                }
                if (res == 0) {
                    ythrow TWithBackTrace<yexception>() << "No neighbors. Your program is wrong";
                }
                return res;
            }

            size_t GetNeighborCountExact(size_t mainIndex) const {
                return BitTrie.Get(Hypothesis[mainIndex]->Sketch.Simhash).second;
            }

            void FindNeighbors(size_t mainIndex, TVector<size_t>* groupIndexes) const {
                if (Y_UNLIKELY(Processed[mainIndex])) {
                    ythrow TWithBackTrace<yexception>() << "Main is processed";
                }
                groupIndexes->clear();
                if (Unprocessed == 0) {
                    return;
                }
                const TDoc& main = *Hypothesis[mainIndex];
                BitTrie.GetNeighbors(main.Sketch.Simhash, Config.CompareConfig.SimhashTreshold, &TempGroup);
                const size_t s = TempGroup.size();
                for (size_t i = 0; i < s; ++i) {
                    const ui32 subgroupStart = TempGroup[i]->first;
                    const ui32 subgroupSize = TempGroup[i]->second;
                    if (subgroupSize == 0) {
                        ythrow TWithBackTrace<yexception>() << "No neighbors";
                    }
                    for (ui32 j = 0; j < subgroupSize; ++j) {
                        const size_t index = SortedBySimhash[subgroupStart + j];
                        if (index == 0xFFFFFFFF) {
                            ythrow TWithBackTrace<yexception>() << "Inconsistent state";
                        }
                        if (BackMap[index] == 0xFFFFFFFF) {
                            ythrow TWithBackTrace<yexception>() << "Inconsistent state";
                        }
                        if (index == mainIndex) {
                            continue;
                        }
                        if (Y_UNLIKELY(Processed[index])) {
                            ythrow TWithBackTrace<yexception>() << "Not main is processed";
                        }
                        if (CheckPair(main, index)) {
                            groupIndexes->push_back(index);
                        }
                    }
                }
            }

            void Remove(const TVector<size_t>& indexes) {
                const size_t s = indexes.size();
                if (Y_UNLIKELY(s > Unprocessed)) {
                    ythrow TWithBackTrace<yexception>() << "Too much for remove";
                }
                for (size_t i = 0; i < s; ++i) {
                    if (Y_UNLIKELY(Processed[indexes[i]])) {
                        ythrow TWithBackTrace<yexception>() << "Doc is processed i = " << i;
                    }
                    const TDoc* doc = Hypothesis[indexes[i]];
                    std::pair<ui32, ui32>& group = BitTrie.Get(doc->Sketch.Simhash);
                    const ui32 groupStart = group.first;
                    const ui32 groupSize = group.second;
                    const ui32 indexToDelete = BackMap[indexes[i]];
                    if (indexToDelete == 0xFFFFFFFF) {
                        ythrow TWithBackTrace<yexception>() << "Inconsistent statei = " << i;
                    }
                    if (SortedBySimhash[indexToDelete] != indexes[i]) {
                        ythrow TWithBackTrace<yexception>() << "Inconsistent statei = " << i;
                    }
                    if (Hypothesis[SortedBySimhash[indexToDelete]] != doc) {
                        ythrow TWithBackTrace<yexception>() << "Inconsistent statei = " << i;
                    }
                    if (indexToDelete < groupStart || indexToDelete >= groupStart + groupSize) {
                        ythrow TWithBackTrace<yexception>() << "Index out of range i = " << i;
                    }
                    if (SortedBySimhash[groupStart] == 0xFFFFFFFF) {
                        ythrow TWithBackTrace<yexception>() << "Inconsistent statei = " << i;
                    }
                    if (Hypothesis[SortedBySimhash[groupStart]]->Sketch.Simhash != doc->Sketch.Simhash) {
                        ythrow TWithBackTrace<yexception>() << "Simhash mismatchi = " << i;
                    }

                    if (groupSize == 0) {
                        ythrow TWithBackTrace<yexception>() << "No neighborsi = " << i;
                    } else if (groupSize == 1) {
                        if (indexToDelete != groupStart) {
                            ythrow TWithBackTrace<yexception>() << "Index mismatchi = " << i;
                        }
                        if (BackMap[SortedBySimhash[groupStart]] == 0xFFFFFFFF) {
                            ythrow TWithBackTrace<yexception>() << "Inconsistent statei = " << i;
                        }
                        BackMap[SortedBySimhash[groupStart]] = 0xFFFFFFFF;
                        SortedBySimhash[groupStart] = 0xFFFFFFFF;
                        if (!BitTrie.Delete(doc->Sketch.Simhash)) {
                            ythrow TWithBackTrace<yexception>() << "Nothing to deletei = " << i;
                        }
                    } else {
                        if (Hypothesis[SortedBySimhash[indexToDelete]]->Sketch.Simhash != doc->Sketch.Simhash) {
                            ythrow TWithBackTrace<yexception>() << "Simhash mismatchi = " << i;
                        }
                        if (indexToDelete != groupStart + groupSize - 1) {
                            if (SortedBySimhash[groupStart + groupSize - 1] == 0xFFFFFFFF) {
                                ythrow TWithBackTrace<yexception>() << "Inconsistent statei = " << i;
                            }
                            BackMap[SortedBySimhash[groupStart + groupSize - 1]] = indexToDelete;
                            SortedBySimhash[indexToDelete] = SortedBySimhash[groupStart + groupSize - 1];

                            if (Hypothesis[SortedBySimhash[indexToDelete]]->Sketch.Simhash != doc->Sketch.Simhash) {
                                ythrow TWithBackTrace<yexception>() << "Simhash mismatchi = " << i;
                            }
                        }
                        BackMap[indexes[i]] = 0xFFFFFFFF;
                        SortedBySimhash[groupStart + groupSize - 1] = 0xFFFFFFFF;

                        group = std::make_pair(groupStart, groupSize - 1);
                    }
                }

                Unprocessed -= s;
            }

            size_t Count() const {
                return Unprocessed;
            }

            //            void CheckConsistensy() const {
            //                const size_t s = Hypothesis.size();
            //                for (size_t i = 0; i < s; ++i) {
            //                    if (Processed[i]) {
            //                        if (BackMap[i] != 0xFFFFFFFF) {
            //                            ythrow TWithBackTrace<yexception>() << "Inconsistent state";
            //                        }
            //                    } else {
            //                        if (BackMap[i] == 0xFFFFFFFF) {
            //                            ythrow TWithBackTrace<yexception>() << "Inconsistent state";
            //                        }
            //                        if (SortedBySimhash[BackMap[i]] != i) {
            //                            ythrow TWithBackTrace<yexception>() << "Inconsistent state " <<
            //                                                            SortedBySimhash[BackMap[i]] <<
            //                                                            " " << BackMap[i];
            //                        }
            //                    }
            //                }
            //                typename TCompBitTrie< TSimhash, std::pair<ui32, ui32> >::TConstIt it = BitTrie.CMin();
            //                while (!it.IsEndReached()) {
            //                    TSimhash simhash = it.GetCurrentKey();
            //                    std::pair<ui32, ui32> group = it.GetCurrentValue();
            //                    ui32 groupStart = group.first;
            //                    ui32 groupSize = group.second;
            //                    for (ui32 i = 0; i < groupSize; ++i) {
            //                        if (SortedBySimhash[groupStart + i] == 0xFFFFFFFF) {
            //                            ythrow TWithBackTrace<yexception>() << "Inconsistent state";
            //                        }
            //                        if (BackMap[SortedBySimhash[groupStart + i]] == 0xFFFFFFFF) {
            //                            ythrow TWithBackTrace<yexception>() << "Inconsistent state";
            //                        }
            //                        if (Hypothesis[SortedBySimhash[groupStart + i]]->Sketch.Simhash != simhash) {
            //                            ythrow TWithBackTrace<yexception>() << "Inconsistent state";
            //                        }
            //                        if (BackMap[SortedBySimhash[groupStart + i]] != groupStart + i) {
            //                            ythrow TWithBackTrace<yexception>() << "Inconsistent state";
            //                        }
            //                    }
            //                    it.Inc();
            //                }
            //            }

        private:
            struct TCompareBySimhash {
                TCompareBySimhash(const TVector<const TDoc*>& hypothesis)
                    : Hypothesis(hypothesis)
                {
                }

                bool operator()(ui32 left, ui32 right) const {
                    return Hypothesis[left]->Sketch.Simhash <
                           Hypothesis[right]->Sketch.Simhash;
                }

                const TVector<const TDoc*>& Hypothesis;
            };

            inline bool CheckPair(const TDoc& main, const size_t right) const {
                if (Y_UNLIKELY(Processed[right])) {
                    ythrow TWithBackTrace<yexception>() << "Touching processed document. Your program is wrong";
                }
                return (DifferenceRatio(main.Sketch.DocLength, Hypothesis[right]->Sketch.DocLength) <= Config.CompareConfig.LengthTreshold);
            }

        private:
            TClustererConfig& Config;
            const TVector<const TDoc*>& Hypothesis;
            const TVector<bool>& Processed;
            TVector<ui32> SortedBySimhash;
            TVector<ui32> BackMap;
            size_t Unprocessed;
            TCompBitTrie<TSimhash, std::pair<ui32, ui32>> BitTrie;
            mutable TVector<const std::pair<ui32, ui32>*> TempGroup;
        };

        template <typename TSortingAlgorithm, typename TSearchingAlgorithm>
        struct TAlgorithmIndexSelector;

        template <typename TDoc>
        struct TAlgorithmIndexSelector<TNeighborsFull<TDoc>, TBruteForceSearch<TDoc>> {
            enum {
                Algorithm = TClustererConfig::CA_NeighborsFullBruteForce
            };
        };

        template <typename TDoc>
        struct TAlgorithmIndexSelector<TNeighborsExact<TDoc>, TBruteForceSearch<TDoc>> {
            enum {
                Algorithm = TClustererConfig::CA_NeighborsExactBruteForce
            };
        };

        template <typename TDoc>
        struct TAlgorithmIndexSelector<TOnlyByMainrank<TDoc>, TBruteForceSearch<TDoc>> {
            enum {
                Algorithm = TClustererConfig::CA_OnlyByMainrankBruteForce
            };
        };

        template <typename TDoc>
        struct TAlgorithmIndexSelector<TNoSorting<TDoc>, TBruteForceSearch<TDoc>> {
            enum {
                Algorithm = TClustererConfig::CA_NoSortingBruteForce
            };
        };

        template <typename TDoc>
        struct TAlgorithmIndexSelector<TNeighborsFull<TDoc>, TBitTrieSearch<TDoc>> {
            enum {
                Algorithm = TClustererConfig::CA_NeighborsFullBitTrie
            };
        };

        template <typename TDoc>
        struct TAlgorithmIndexSelector<TNeighborsExact<TDoc>, TBitTrieSearch<TDoc>> {
            enum {
                Algorithm = TClustererConfig::CA_NeighborsExactBitTrie
            };
        };

        template <typename TDoc>
        struct TAlgorithmIndexSelector<TOnlyByMainrank<TDoc>, TBitTrieSearch<TDoc>> {
            enum {
                Algorithm = TClustererConfig::CA_OnlyByMainrankBitTrie
            };
        };

        template <typename TDoc>
        struct TAlgorithmIndexSelector<TNoSorting<TDoc>, TBitTrieSearch<TDoc>> {
            enum {
                Algorithm = TClustererConfig::CA_NoSortingBitTrie
            };
        };
    }
    //
    /////////////////////////////////////////////////////////////////////

    template <typename TSimhash, typename TMainrank, typename TPayload, typename THandler>
    class TClusterer: private TNonCopyable {
    public:
        typedef TDocument<TSimhash, TMainrank, TPayload> TDoc;
        typedef typename TDoc::TSketch TSketch;

    private:
#include "clusterer.small_group.h"

#include "clusterer.ordinar.h"

    public:
        TClusterer(const TClustererConfig& config, THandler* handler)
            : Config(config)
            , Handler(handler)
            , Hypothesis()
            , SmallGroupClusterer(Config, handler)
        {
        }

    public:
        TClustererConfig& GetConfig() {
            return Config;
        }

        void Reset() {
            Handler->ResetAfterClustererCrash();

            Hypothesis.clear();
            Hypothesis.shrink_to_fit();
        }

        inline void OnHypothesisStart() {
            if (Y_UNLIKELY(Hypothesis.size())) {
                ythrow TWithBackTrace<yexception>() << "Wrong internal state. Hypothesis.size() != 0. Call to OnHypothesisEnd may help";
            }

            Handler->OnHypothesisStart();
        }

        inline void OnDocument(TSimhash simhash, ui32 docLength, const TMainrank& mainrank, const TPayload& payload) {
            OnDocument(TSketch(simhash, docLength), mainrank, payload);
        }

        inline void OnDocument(const TSketch& sketch, const TMainrank& mainrank, const TPayload& payload) {
            OnDocument(TDoc(sketch, mainrank, payload));
        }

        inline void OnDocument(const TDoc& doc) {
            Hypothesis.push_back(doc);

            const size_t s = Hypothesis.size();

            if (s > Config.AlgorithmSelectionConfig.MaxNoSortingHypothesisSize) {
                for (size_t i = 0; i < s; ++i) {
                    Handler->OnUnprocessedDocument(Hypothesis[i]);
                }
                TOrdinarClusterer<NPrivate::TNoSorting, NPrivate::TBitTrieSearch> clusterer(Config, Handler);
                clusterer.OnHypothesis(Hypothesis);
                Hypothesis.clear();
                Handler->OnHypothesisEnd();
                Handler->OnHypothesisStart();
            }
        }

        void OnHypothesisEnd() {
            const size_t s = Hypothesis.size();
            if (Y_LIKELY(s != 0)) {
                for (size_t i = 0; i < s; ++i) {
                    Handler->OnUnprocessedDocument(Hypothesis[i]);
                }
                TClustererConfig::EClusteringAlgorithm algorithm = Config.ClusteringAlgorithm;
                if (Y_LIKELY(algorithm == TClustererConfig::CA_Auto)) {
                    if (Y_LIKELY(s <= 3)) {
                        algorithm = TClustererConfig::CA_SmallGroup;
                    } else {
                        algorithm = TClustererConfig::SelectAlgorithm(Config, s);
                    }
                }
                switch (algorithm) {
                    case TClustererConfig::CA_Auto:
                        ythrow TWithBackTrace<yexception>() << "Not expected auto algorithm. Your program is wrong";
                        break;
                    case TClustererConfig::CA_NeighborsExactBitTrie: {
                        TOrdinarClusterer<NPrivate::TNeighborsExact, NPrivate::TBitTrieSearch> clusterer(Config, Handler);
                        clusterer.OnHypothesis(Hypothesis);
                    } break;
                    case TClustererConfig::CA_NeighborsExactBruteForce: {
                        TOrdinarClusterer<NPrivate::TNeighborsExact, NPrivate::TBruteForceSearch> clusterer(Config, Handler);
                        clusterer.OnHypothesis(Hypothesis);
                    } break;
                    case TClustererConfig::CA_NeighborsFullBitTrie: {
                        TOrdinarClusterer<NPrivate::TNeighborsFull, NPrivate::TBitTrieSearch> clusterer(Config, Handler);
                        clusterer.OnHypothesis(Hypothesis);
                    } break;
                    case TClustererConfig::CA_NeighborsFullBruteForce: {
                        TOrdinarClusterer<NPrivate::TNeighborsFull, NPrivate::TBruteForceSearch> clusterer(Config, Handler);
                        clusterer.OnHypothesis(Hypothesis);
                    } break;
                    case TClustererConfig::CA_NoSortingBitTrie: {
                        TOrdinarClusterer<NPrivate::TNoSorting, NPrivate::TBitTrieSearch> clusterer(Config, Handler);
                        clusterer.OnHypothesis(Hypothesis);
                    } break;
                    case TClustererConfig::CA_NoSortingBruteForce: {
                        TOrdinarClusterer<NPrivate::TNoSorting, NPrivate::TBruteForceSearch> clusterer(Config, Handler);
                        clusterer.OnHypothesis(Hypothesis);
                    } break;
                    case TClustererConfig::CA_OnlyByMainrankBitTrie: {
                        TOrdinarClusterer<NPrivate::TOnlyByMainrank, NPrivate::TBitTrieSearch> clusterer(Config, Handler);
                        clusterer.OnHypothesis(Hypothesis);
                    } break;
                    case TClustererConfig::CA_OnlyByMainrankBruteForce: {
                        TOrdinarClusterer<NPrivate::TOnlyByMainrank, NPrivate::TBruteForceSearch> clusterer(Config, Handler);
                        clusterer.OnHypothesis(Hypothesis);
                    } break;
                    case TClustererConfig::CA_SmallGroup: {
                        SmallGroupClusterer.OnHypothesis(Hypothesis);
                    } break;
                    default:
                        ythrow TWithBackTrace<yexception>() << "Unknown clustering algorithm";
                }
                Hypothesis.clear();
            }

            Handler->OnHypothesisEnd();
        }

        size_t GetInternalHypothesisSize() const {
            return Hypothesis.size();
        }

    private:
        TClustererConfig Config;
        THandler* Handler;
        TVector<TDoc> Hypothesis;

        TSmallGroupClusterer SmallGroupClusterer;
    };
}
