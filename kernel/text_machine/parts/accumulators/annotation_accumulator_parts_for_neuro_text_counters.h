#pragma once
#include "annotation_accumulator_parts.h"


namespace NTextMachine::NCore {

#include <kernel/text_machine/module/module_def.inc>

MACHINE_PARTS(AnnotationAccumulator) {

    struct TNeuroTextCounterLevel1Group {
        template<class T>
        struct TWordMatrixData {
            TPoolPodHolder<T> Data;
            size_t WordsNum = 0;

            void Init(TMemoryPool& pool, TQueryWordId wordNum) {
                WordsNum = wordNum;
                Data.Init(pool, wordNum * wordNum, EStorageMode::Full);
            }

            void NewDoc() {
                Data.FillZeroes();
            }

            T& GetRef(TQueryWordId c1, TQueryWordId c2) {
                size_t id = c1 * WordsNum + c2;
                return Data[id];
            }

            TArrayRef<T> GetRange(TQueryWordId c1) {
                T* start = Data.Data() + (c1 * WordsNum);
                return {start, start + WordsNum};
            }

            TArrayRef<const T> GetRange(TQueryWordId c1) const {
                const T* start = Data.Data() + (c1 * WordsNum);
                return {start, start + WordsNum};
            }
        };

        struct TWordCounters {
            ui32 TermFrequence = 0;
            ui32 ExactTermFrequence = 0;
            TQueryWordId MaxQueryCooucrance = 0;

            void Update(TBreakWordId matchesNum, TBreakWordId exactMatchesNum, TQueryWordId queryWordsCovered) {
                TermFrequence += matchesNum;
                ExactTermFrequence += exactMatchesNum;
                MaxQueryCooucrance = Max(MaxQueryCooucrance, queryWordsCovered);
            }
        };

        struct TBigramCounters {
            TBreakId SentecesWithBothWords = 0;
            TBreakId SentencesWithBothWordsAnyFormNeighboring = 0;
            TBreakId SentencesWithBothWordsExactNeighboring = 0;
            TQueryWordId MaxQueryWordsInOneSentenceFoundWithCurrentWords = 0;
            TQueryWordId MaxQueryWordsInOneSentenceFoundWithCurrentWordsNeighboring = 0;

            float InverseDistanceSum = 0;

            void Update(
                bool exactMatchGot,
                bool exactMatchSecondGot,
                TQueryWordId wordsFromQueryCovered,
                float invDist)
            {
                InverseDistanceSum += invDist;
                SentecesWithBothWords += 1;
                MaxQueryWordsInOneSentenceFoundWithCurrentWords =
                    Max(MaxQueryWordsInOneSentenceFoundWithCurrentWords, wordsFromQueryCovered);

                if (invDist == 1.0f) {
                    SentencesWithBothWordsAnyFormNeighboring += 1;
                    MaxQueryWordsInOneSentenceFoundWithCurrentWordsNeighboring =
                        Max(MaxQueryWordsInOneSentenceFoundWithCurrentWordsNeighboring, wordsFromQueryCovered);
                    if (exactMatchGot && exactMatchSecondGot) {
                        SentencesWithBothWordsExactNeighboring += 1;
                    }
                }
            }
        };

        UNIT(TNeuroTextCounterLevel1Unit) {
            UNIT_STATE {
                struct TWordInfo {
                    TBreakWordId LastPos = 0;
                    TBreakWordId TermFrequence = 0;
                };
                using TPositionsAccumulator = TGenerativeOptionalArrayAccumulator<TWordInfo, TQueryWordId, ui32>;

                TWordMatrixData<float> InvDistances;
                TValueCollection<TWordCounters> WordCounters;
                TValueCollection<TBigramCounters> BigramCounters;
                TPositionsAccumulator LastPositionsAny;
                TPositionsAccumulator LastPositionsExact;

                void SaveToJson(NJson::TJsonValue&) const {
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                REQUIRE_UNIT(
                    TCoreUnit
                );

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                    Y_UNUSED(pool, info);
                    Vars().InvDistances.Init(pool, info.NumWords);
                    Vars().WordCounters.Init(pool, info.NumWords);
                    if (info.NumWords > 1) {
                        Vars().BigramCounters.Init(pool, info.NumWords - 1);
                    }
                    Vars().LastPositionsAny.Init(pool, info.NumWords);
                    Vars().LastPositionsExact.Init(pool, info.NumWords);
                }
                Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                    Vars().InvDistances.NewDoc();
                    Vars().WordCounters.MemSetZeroes();
                    if (Vars().BigramCounters.Size() > 0) {
                        Vars().BigramCounters.MemSetZeroes();
                    }
                }

                Y_FORCE_INLINE void StartAnnotation(const TAnnotationInfo&) {
                    Vars().LastPositionsAny.Clear();
                    Vars().LastPositionsExact.Clear();
                }

                Y_FORCE_INLINE void FinishHit(const THitInfo& info) {
                    {
                        auto& ref = Vars().LastPositionsAny.GetRefWithDefault(info.WordId, {});
                        ref.TermFrequence += 1;
                        ref.LastPos = info.LeftPosition;
                    }
                    if (Vars<TCoreUnit>().Flags.IsExact) {
                        auto& ref = Vars().LastPositionsExact.GetRefWithDefault(info.WordId, {});
                        ref.TermFrequence += 1;
                        ref.LastPos = info.LeftPosition;
                    }
                }

                static constexpr float CalcInvDistnace(TBreakWordId first, TBreakWordId second) {
                    float res = 0;
                    if (first < second) {
                        res = second - first;
                    } else {
                        res = first - second;
                    }
                    if (res > 1.0f) {
                        return 1.0f / res;
                    } else {
                        return 1.0f;
                    }
                }

                Y_FORCE_INLINE void FinishAnnotation(const TAnnotationInfo&) {
                    const auto& listOfOccuredInAnnotationAny = Vars().LastPositionsAny;
                    const auto& listOfOccuredInAnnotationExact = Vars().LastPositionsExact;

                    TQueryWordId wordsFromQueryCovered = listOfOccuredInAnnotationAny.Count();
                    for(auto [wordId, posInfo] : listOfOccuredInAnnotationAny) {
                        TBreakWordId exactMatchesNumGot = listOfOccuredInAnnotationExact.GetValWithDefault(wordId, {}).TermFrequence;
                        bool exactMatchGot = exactMatchesNumGot > 0;
                        Vars().WordCounters[wordId].Update(
                            posInfo.TermFrequence,
                            exactMatchesNumGot,
                            wordsFromQueryCovered
                        );

                        TBreakWordId lastPos = posInfo.LastPos;
                        TArrayRef<float> invDistancesView = InvDistances.GetRange(wordId);
                        for(auto [secondWorId, secondPosInfo] : listOfOccuredInAnnotationAny) {
                            TBreakWordId secondLastPos = secondPosInfo.LastPos;
                            float invDist = CalcInvDistnace(lastPos, secondLastPos);
                            invDistancesView[secondWorId] = Max(invDist, invDistancesView[secondWorId]);

                            if (wordId + 1  == secondWorId) {//got bigram starting from wordId
                                bool exactMatchSecondGot = listOfOccuredInAnnotationExact.Contain(secondWorId);
                                Vars().BigramCounters[wordId].Update(
                                    exactMatchGot, exactMatchSecondGot,
                                    wordsFromQueryCovered,
                                    invDist
                                );
                            }
                        }
                    }
                }
            };
        };
    };

    using TNeuroTextCounterLevel1Unit = typename TNeuroTextCounterLevel1Group::TNeuroTextCounterLevel1Unit;

} // MACHINE_PARTS(AnnotationAccumulator)
} // namespace NTextMachine::NCore

#include <kernel/text_machine/module/module_undef.inc>
