#pragma once

#include "bit_mask_accumulator.h"
#include "coordination_accumulator.h"

#include <kernel/text_machine/parts/common/bag_of_words.h>
#include <kernel/text_machine/parts/common/break_detector.h>
#include <kernel/text_machine/parts/common/types.h>
#include <kernel/text_machine/interface/hit.h>

#include <kernel/text_machine/module/module_def.inc>

#include <array>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(BagOfWordsAccumulator) {
        constexpr ui16 MaxAnnotationLength = 256;

        struct TBagOfWordsInfo {
            const TCoreSharedState& CoreState;
            const TBagOfWords& BagOfWords;
        };

        struct THitInfo {
            EMatchPrecisionType MatchPrecision = TMatchPrecision::Unknown;
            EStreamType StreamType = TStream::StreamMax;
            TQueryId ExternalQueryId = Max<TQueryId>();
            TQueryWordId ExternalWordId = Max<TQueryWordId>();
            TQueryId BagQueryId = Max<TQueryId>();
            TQueryWordId BagWordId = Max<TQueryWordId>();
            TBreakId AnnotationId = Max<ui16>();
            TBreakWordId LeftPosition = Max<ui16>();
            TBreakWordId RightPosition = Max<ui16>();
            TStreamWordId Offset = Max<ui32>();
        };

        struct TAnnotationInfo {
            float Value;
            ui16 Length;
        };

        class TExactPassFilter {
        public:
            bool operator()(EMatchPrecisionType matchPrecision) {
                return (matchPrecision == TMatchPrecision::Exact);
            }
        };

        class TExactOrLemmaPassFilter {
        public:
            bool operator()(EMatchPrecisionType matchPrecision) {
                return (matchPrecision == TMatchPrecision::Exact || matchPrecision == TMatchPrecision::Lemma);
            }
        };

        class TAllPassFilter {
        public:
            bool operator()(EMatchPrecisionType /*matchPrecision*/) {
                return true;
            }
        };

        UNIT_FAMILY(TCosineMatchFamily)

        template <typename Accumulator>
        UNIT(TCosineMatchStub) {
            UNIT_FAMILY_STATE(TCosineMatchFamily) {
                const TBagOfWords* BagOfWords = nullptr;

                TCoordinationAccumulator QueryWordPosAcc;
                TCoordinationAccumulator AffectedQueriesPosAcc;
                TPoolPodHolder<ui16> AffectedQueries;
                TPoolPodHolder<TBitMaskAccumulator> AnnWordPosMasks;
                TPoolPodHolder<float> QueryWcmSum;

                Accumulator CosineSimilarityAccW2S2;
                float MaxMatchW1S2 = NAN;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, QueryWordPosAcc);
                    SAVE_JSON_VAR(value, AffectedQueriesPosAcc);
                    SAVE_JSON_VAR(value, AffectedQueries);
                    SAVE_JSON_VAR(value, AnnWordPosMasks);
                    SAVE_JSON_VAR(value, QueryWcmSum);
                    SAVE_JSON_VAR(value, CosineSimilarityAccW2S2);
                    SAVE_JSON_VAR(value, MaxMatchW1S2);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewBagOfWords(TMemoryPool& pool, const TBagOfWordsInfo& info) {
                    Vars().BagOfWords = &info.BagOfWords;

                    Vars().QueryWordPosAcc.Init(pool, info.BagOfWords.GetWordCount());
                    Vars().AnnWordPosMasks.Init(pool, info.BagOfWords.GetQueryCount(), EStorageMode::Full);
                    Vars().QueryWcmSum.Init(pool, info.BagOfWords.GetQueryCount(), EStorageMode::Full);
                    Vars().AffectedQueriesPosAcc.Init(pool, info.BagOfWords.GetQueryCount());
                    Vars().AffectedQueries.Init(pool, info.BagOfWords.GetQueryCount());
                }
                Y_FORCE_INLINE void NewDoc() {
                    Vars().CosineSimilarityAccW2S2.Clear();
                    Vars().MaxMatchW1S2 = 0;
                }
                Y_FORCE_INLINE void StartAnnotation(const TAnnotationInfo& /*info*/) {
                    Vars().QueryWordPosAcc.Clear();
                    Vars().AffectedQueriesPosAcc.Clear();
                    Vars().AffectedQueries.Clear();
                }
                Y_FORCE_INLINE void AddHit(const THitInfo& hitInfo) {
                    if (!Vars().AffectedQueriesPosAcc.Contain(hitInfo.BagQueryId)) {
                        Vars().AffectedQueriesPosAcc.Update(hitInfo.BagQueryId);
                        Vars().AffectedQueries.Add(hitInfo.BagQueryId);
                        Vars().AnnWordPosMasks[hitInfo.BagQueryId].Clear();
                        Vars().QueryWcmSum[hitInfo.BagQueryId] = 0.0f;
                    }

                    if (!Vars().QueryWordPosAcc.Contain(hitInfo.BagWordId)) {
                        Vars().QueryWordPosAcc.Update(hitInfo.BagWordId);
                        Vars().QueryWcmSum[hitInfo.BagQueryId] +=
                            Vars().BagOfWords->GetWordWeight(hitInfo.BagWordId);
                    }

                    for (ui16 i = hitInfo.LeftPosition; i <= hitInfo.RightPosition; ++i) {
                        Vars().AnnWordPosMasks[hitInfo.BagQueryId].Update(i);
                    }
                }

                Y_FORCE_INLINE void FinishAnnotation(const TAnnotationInfo& info) {
                    float maxCosSimW1S2 = 0.0f;
                    float maxCosSimW2S2 = 0.0f;
                    float annotationLenInv = info.Length > 0 ? 1.0f / info.Length : 0.0f;
                    for (const auto& bagQueryId : Vars().AffectedQueries) {
                        float queryWcmSum = Vars().QueryWcmSum[bagQueryId];
                        size_t annWordCovered = Vars().AnnWordPosMasks[bagQueryId].GetCount();
                        float cosSimSqr = CalcCosineSimilaritySqr(queryWcmSum,
                            annWordCovered, annotationLenInv);
                        float weight = Vars().BagOfWords->GetQueryWeight(bagQueryId);

                        maxCosSimW1S2 = Max(maxCosSimW1S2, weight * cosSimSqr);
                        maxCosSimW2S2 = Max(maxCosSimW2S2, weight * weight * cosSimSqr);
                    }

                    Vars().MaxMatchW1S2 = Max(maxCosSimW1S2, Vars().MaxMatchW1S2);
                    Vars().CosineSimilarityAccW2S2.Update(maxCosSimW2S2, info.Value);
                }
                Y_FORCE_INLINE float CalcCosineSimilaritySqr(const float wcm,
                    const size_t annWordCovered,
                    const float annotationLenInv)
                {
                    Y_VERIFY_DEBUG(wcm < 1.0f + FloatEpsilon
                        && annWordCovered > 0
                        && (float)annWordCovered * annotationLenInv < 1.0f + FloatEpsilon,
                        "%f %lu %f", wcm, annWordCovered, annotationLenInv);

                    float k = (float)annWordCovered * annotationLenInv;
                    return wcm * k;
                }
                Y_FORCE_INLINE float CalcSorensenSimilaritySqr(const float wcm,
                    const size_t annWordCovered,
                    const size_t annotationLength)
                {
                    Y_VERIFY_DEBUG(wcm < 1.0f + FloatEpsilon
                        && annWordCovered > 0
                        && annWordCovered <= annotationLength,
                        "%f %lu %lu", wcm, annWordCovered, annotationLength);

                    float k = (float)annWordCovered / (float)annotationLength;
                    float result = 2.0 * wcm / (1.0 + wcm / k);
                    Y_ASSERT(result < 1.0f + FloatEpsilon);
                    return result * result;
                }
            };
        };

        UNIT_FAMILY(TFullMatchFamily)

        template <typename Accumulator>
        UNIT(TFullMatchStub) {
            UNIT_FAMILY_STATE(TFullMatchFamily) {
                const TBagOfWords* BagOfWords = nullptr;

                TCoordinationAccumulator AffectedQueriesPosAcc;
                TPoolPodHolder<ui16> AffectedQueries;
                TPoolPodHolder<ui16> FullMatchCount;

                Accumulator FullMatchAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, AffectedQueriesPosAcc);
                    SAVE_JSON_VAR(value, AffectedQueries);
                    SAVE_JSON_VAR(value, FullMatchCount);
                    SAVE_JSON_VAR(value, FullMatchAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewBagOfWords(TMemoryPool& pool, const TBagOfWordsInfo& info) {
                    Vars().BagOfWords = &info.BagOfWords;

                    Vars().AffectedQueriesPosAcc.Init(pool, info.BagOfWords.GetQueryCount());
                    Vars().FullMatchCount.Init(pool, info.BagOfWords.GetQueryCount(), EStorageMode::Full);
                    Vars().AffectedQueries.Init(pool, info.BagOfWords.GetQueryCount());
                }
                Y_FORCE_INLINE void NewDoc() {
                    Vars().FullMatchAcc.Clear();
                }
                Y_FORCE_INLINE void AddHit(const THitInfo& hitInfo) {
                    if (hitInfo.MatchPrecision == TMatchPrecision::Exact && hitInfo.LeftPosition == hitInfo.ExternalWordId) {
                        if (!Vars().AffectedQueriesPosAcc.Contain(hitInfo.BagQueryId)) {
                            Vars().AffectedQueriesPosAcc.Update(hitInfo.BagQueryId);
                            Vars().AffectedQueries.Add(hitInfo.BagQueryId);
                            Vars().FullMatchCount[hitInfo.BagQueryId] = 0;
                        }

                        ++Vars().FullMatchCount[hitInfo.BagQueryId];
                    }
                }
                Y_FORCE_INLINE void StartAnnotation(const TAnnotationInfo& /*info*/) {
                    Vars().AffectedQueriesPosAcc.Clear();
                    Vars().AffectedQueries.Clear();
                }
                Y_FORCE_INLINE void FinishAnnotation(const TAnnotationInfo& info) {
                    for (const auto& bagQueryId : Vars().AffectedQueries) {
                        if (info.Length == Vars().FullMatchCount[bagQueryId] &&
                            info.Length == Vars().BagOfWords->GetQuerySize(bagQueryId)) {
                                float weight = Vars().BagOfWords->GetQueryWeight(bagQueryId);
                                Vars().FullMatchAcc.Update(1.0, weight * info.Value);
                        }
                    }
                }
            };
        };

        UNIT_FAMILY(TAnnotationMatchFamily)

        template <typename Accumulator>
        UNIT(TAnnotationMatchStub) {
            UNIT_FAMILY_STATE(TAnnotationMatchFamily) {
                Accumulator AnnotationMatchAccumulator;
                TCoordinationAccumulator AnnWordPosAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, AnnotationMatchAccumulator);
                    SAVE_JSON_VAR(value, AnnWordPosAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewBagOfWords(TMemoryPool& pool, const TBagOfWordsInfo& /*info*/) {
                    Vars().AnnWordPosAcc.Init(pool, MaxAnnotationLength);
                }
                Y_FORCE_INLINE void NewDoc() {
                    Vars().AnnotationMatchAccumulator.Clear();
                }
                Y_FORCE_INLINE void AddHit(const THitInfo& hitInfo) {
                    for (ui16 i = hitInfo.LeftPosition; i <= hitInfo.RightPosition; ++i) {
                        Vars().AnnWordPosAcc.Update(i);
                    }
                }
                Y_FORCE_INLINE void StartAnnotation(const TAnnotationInfo& /*info*/) {
                    Vars().AnnWordPosAcc.Clear();
                }
                Y_FORCE_INLINE void FinishAnnotation(const TAnnotationInfo& info) {
                    if (Vars().AnnWordPosAcc.Count() == info.Length) {
                        Vars().AnnotationMatchAccumulator.Update(1.0, info.Value);
                    }
                }
            };
        };

        class TBagFractionCalculator : public NModule::TJsonSerializable {
        private:
            struct TStreamState : public NModule::TJsonSerializable {
                ui32 Offset = Max<ui32>();
                ui16 AnnId = Max<ui16>();
                TPoolPodHolder<size_t> BagWordIds;
                bool IsOriginalRequestWord = false;

                void Clear() {
                    Offset = Max<ui32>();
                    AnnId = Max<ui16>();
                    BagWordIds.Clear();
                    IsOriginalRequestWord = false;
                }

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, Offset);
                    SAVE_JSON_VAR(value, AnnId);
                    SAVE_JSON_VAR(value, BagWordIds);
                    SAVE_JSON_VAR(value, IsOriginalRequestWord);
                }
            };
            using TStreamStatesMap = TPoolableCompactEnumMap<TStream, TStreamState>;

            Y_FORCE_INLINE void UpdateOriginalRequestWords(const TStreamState& streamState) {
                if (streamState.IsOriginalRequestWord) {
                    for (auto bagWordId : streamState.BagWordIds) {
                        OriginalRequestWordFlags[bagWordId] = true;
                    }
                }
            }

            const TBagOfWords* BagOfWords = nullptr;
            TStreamStatesMap StreamStates;
            TPoolPodHolder<ui16> OriginalRequestWordFlags;

        public:
            Y_FORCE_INLINE void Init(TMemoryPool& pool,
                const TStreamRemap& streamRemap,
                const TBagOfWords& bagOfWords)
            {
                BagOfWords = &bagOfWords;

                OriginalRequestWordFlags.Init(pool, bagOfWords.GetWordCount(), EStorageMode::Full);
                StreamStates = TStreamStatesMap(pool, streamRemap);
                for (auto entry : StreamStates) {
                    entry.Value().BagWordIds.Init(pool, bagOfWords.GetWordCount());
                }
            }
            Y_FORCE_INLINE void SaveToJson(NJson::TJsonValue& value) const {
                SAVE_JSON_VAR(value, StreamStates);
                SAVE_JSON_VAR(value, OriginalRequestWordFlags);
            }
            Y_FORCE_INLINE void NewDoc() {
                for (auto entry : StreamStates) {
                    entry.Value().Clear();
                }
                OriginalRequestWordFlags.Fill(false);
            }
            Y_FORCE_INLINE void FinishDoc() {
                for (auto entry : StreamStates) {
                    UpdateOriginalRequestWords(entry.Value());
                }
            }
            Y_FORCE_INLINE void AddHit(const THitInfo& hitInfo) {
                TStreamState& streamState = StreamStates[hitInfo.StreamType];
                if (streamState.Offset != hitInfo.Offset || streamState.AnnId != hitInfo.AnnotationId ||
                    streamState.BagWordIds.Count() == BagOfWords->GetWordCount()) {
                    UpdateOriginalRequestWords(streamState);

                    streamState.Offset = hitInfo.Offset;
                    streamState.AnnId = hitInfo.AnnotationId;
                    streamState.BagWordIds.Clear();
                    streamState.IsOriginalRequestWord = false;
                }
                if (hitInfo.BagQueryId == BagOfWords->GetOriginalQueryId()) {
                    streamState.IsOriginalRequestWord = true;
                }
                streamState.BagWordIds.Add(hitInfo.BagWordId);
            }
            Y_FORCE_INLINE float CalcOriginalRequestBagFraction() const {
                float originalRequestBagFraction = 0.0;
                for (size_t i = 0; i < BagOfWords->GetWordCount(); ++i) {
                    if (OriginalRequestWordFlags[i]) {
                        originalRequestBagFraction += BagOfWords->GetBagWordWeight(i);
                    }
                }
                return originalRequestBagFraction;
            }
        };

        UNIT(TBagFractionExactUnit) {
            UNIT_STATE {
                TExactPassFilter HitFilter;
                TBagFractionCalculator BagFractionCalculator;
                float OriginalRequestBagFraction = NAN;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, BagFractionCalculator);
                    SAVE_JSON_VAR(value, OriginalRequestBagFraction);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewBagOfWords(TMemoryPool& pool, const TBagOfWordsInfo& info) {
                    BagFractionCalculator.Init(pool, info.CoreState.StreamRemap, info.BagOfWords);
                }
                Y_FORCE_INLINE void NewDoc() {
                    BagFractionCalculator.NewDoc();
                }
                Y_FORCE_INLINE void FinishDoc() {
                    BagFractionCalculator.FinishDoc();
                    OriginalRequestBagFraction = BagFractionCalculator.CalcOriginalRequestBagFraction();
                }
                Y_FORCE_INLINE void AddHit(const THitInfo& hitInfo) {
                    if (!HitFilter(hitInfo.MatchPrecision)) {
                        return;
                    }
                    BagFractionCalculator.AddHit(hitInfo);
                }
            };
        };

        UNIT(TBagFractionUnit) {
            UNIT_STATE {
                TBagFractionCalculator BagFractionCalculator;
                float OriginalRequestBagFraction = NAN;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, BagFractionCalculator);
                    SAVE_JSON_VAR(value, OriginalRequestBagFraction);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewBagOfWords(TMemoryPool& pool, const TBagOfWordsInfo& info) {
                    BagFractionCalculator.Init(pool, info.CoreState.StreamRemap, info.BagOfWords);
                }
                Y_FORCE_INLINE void NewDoc() {
                    BagFractionCalculator.NewDoc();
                }
                Y_FORCE_INLINE void FinishDoc() {
                    BagFractionCalculator.FinishDoc();
                    OriginalRequestBagFraction = BagFractionCalculator.CalcOriginalRequestBagFraction();
                }
                Y_FORCE_INLINE void AddHit(const THitInfo& hitInfo) {
                    BagFractionCalculator.AddHit(hitInfo);
                }
            };
        };

        template <typename M>
        class TMotor : public M {
        public:
            using TModule = M;

        public:
            void NewMultiQuery(TMemoryPool& pool,
                const TCoreSharedState& coreState,
                const TMultiQuery& multiQuery,
                const TBagOfWords& bagOfWords)
            {
                MultiQuery = &multiQuery;
                BagOfWords = &bagOfWords;
                TBagOfWordsInfo info{coreState, bagOfWords};
                TModule::NewBagOfWords(pool, info);
            }
            void NewDoc() {
                BreakDetector.Reset();
                TModule::NewDoc();
            }
            void AddHit(const THit& hit) {
                BreakDetector.Add(hit.Position);
                if (BreakDetector.IsFinishBreak()) {
                    FinishAnnotation();
                }
                if (BreakDetector.IsStartBreak()) {
                    StartAnnotation(
                        hit.Position.Annotation->Value,
                        hit.Position.Annotation->Length);
                }

                THitInfo info;
                PrepareHitInfo(hit, info);
                TModule::AddHit(info);
            }
            void FinishDoc() {
                if (BreakDetector.IsInBreak()) {
                    FinishAnnotation();
                }

                TModule::FinishDoc();
            }

        public:
            Y_FORCE_INLINE const typename M::template TGetState<TAnnotationMatchFamily>& GetAnnotationMatchState() const {
                return M::template Vars<TAnnotationMatchFamily>();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TCosineMatchFamily>& GetCosineMatchState() const {
                return M::template Vars<TCosineMatchFamily>();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TFullMatchFamily>& GetFullMatchState() const {
                return M::template Vars<TFullMatchFamily>();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TBagFractionExactUnit>& GetBagFractionExactState() const {
                return M::template Vars<TBagFractionExactUnit>();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TBagFractionUnit>& GetBagFractionState() const {
                return M::template Vars<TBagFractionUnit>();
            }

        private:
            Y_FORCE_INLINE void StartAnnotation(const float value, const ui16 length) {
                LastAnnInfo = TAnnotationInfo{value, Min<ui16>(length, MaxAnnotationLength)};
                TModule::StartAnnotation(LastAnnInfo);
            }
            Y_FORCE_INLINE void FinishAnnotation() {
                TModule::FinishAnnotation(LastAnnInfo);
            }
            Y_FORCE_INLINE void PrepareHitInfo(const THit& hit, THitInfo& info) {
                info.ExternalQueryId = hit.Word.QueryId;
                info.ExternalWordId = hit.Word.WordId;

                BagOfWords->GetBagIds(info.ExternalQueryId,
                    info.ExternalWordId,
                    info.BagQueryId,
                    info.BagWordId);

                const TQueryWordMatch* form = MultiQuery->Queries[info.ExternalQueryId].Words[info.ExternalWordId].Forms[hit.Word.FormId];
                info.MatchPrecision = form->MatchPrecision;
                if (form->MatchType != TMatch::OriginalWord) {
                    info.MatchPrecision = TMatchPrecision::Unknown;
                }

                info.StreamType = hit.Position.Annotation->Stream->Type;
                info.AnnotationId = hit.Position.Annotation->BreakNumber;
                info.LeftPosition = hit.Position.LeftWordPos;
                info.RightPosition = Min<ui16>(hit.Position.RightWordPos, MaxAnnotationLength - 1);
                info.Offset = hit.Position.Annotation->FirstWordPos + (ui32)info.LeftPosition;
            }

        private:
            const TMultiQuery* MultiQuery = nullptr;
            const TBagOfWords* BagOfWords;

            TBreakDetector BreakDetector;
            TAnnotationInfo LastAnnInfo;
        };
    }; //MACHINE_PARTS(BagOfWordsAccumulator)
}; //NCore
}; //NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
