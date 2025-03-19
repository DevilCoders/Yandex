#pragma once

#include "bit_mask_accumulator.h"
#include "match_accumulator_parts.h"
#include "coordination_accumulator.h"
#include "sequence_accumulator.h"
#include "word_accumulator.h"

#include <kernel/text_machine/interface/query.h>
#include <kernel/text_machine/interface/hit.h>
#include <kernel/text_machine/parts/common/weights.h>
#include <kernel/text_machine/parts/common/break_detector.h>
#include <kernel/text_machine/parts/common/static_table.h>

#include <library/cpp/pop_count/popcount.h>
#include <util/generic/utility.h>

#include <kernel/text_machine/module/module_def.inc>

// Terms
//
//  - word id = refers to position of word in query
//  - word pos = refers to position of word in annotation
//
//  - proximity = float measure of closeness between positions of two hits in ann
//
//  - form adjust = float measure of hit precision, i.e. closeness between words in query and ann
//  - position adjust = float measure of closeness between position of a hit in query and ann
//  - pos-form adjust = position adjust * form adjust
//
//  - word idf = query word IDF
//  - form idf = word IDF * form adjust
//  - position idf = word IDF * position adjust
//  - pos-form idf = word IDF * pos-form adjust
//
//  - word weight = normalized query word IDF (sum of weights = 1)
//  - form weight = word weight * form adjust
//  - position weight = word weight * position adjust
//  - pos-form weight = word weight * pos-form adjust
//
//  - "any" = taking into account all hits
//  - "exact" = taking into account only exact hits
//  - "original" = taking into account only hits by request words, not by synonyms
//
//  - "match" - in general highly overloaded, here - some similarity between
//      query and annotation, e.g. "same text", "same words", "some common words", etc.
//
//  - match(x, y) = match accumulator with match=x and value=y, where
//      "match" is a float measure of annotation-query similarity (1.0f, number of common words, etc.)
//      "value" is a float measure of annotation "goodness" (typically, some function of ann value)
//      aggregates input pairs (x, y), for details see match_accumulator_parts.h
//
namespace NTextMachine {
namespace NCore {
    enum class EHitAdjust {
        Word,
        Form,
        Position,
        PosForm
    };

    enum class EFullMatch {
        Prefix,
        Full
    };

    enum class EBclmLiteHitFilter {
        None,
        RightOnly
    };

    MACHINE_PARTS(AnnotationAccumulator) {
        Y_CONST_FUNCTION float CalcPositionAdjust(TQueryWordId queryPosition, TBreakWordId annPosition);
        Y_CONST_FUNCTION float CalcProximity(TBreakWordId leftPosition, TBreakWordId rightPosition);
        Y_CONST_FUNCTION int CalcOldWordDistance(TBreakWordId leftPosition, TBreakWordId rightPosition);
        Y_CONST_FUNCTION float CalcOldBclmDist(TBreakId leftBrk, TBreakWordId leftWrd, TBreakId rightBrk, TBreakWordId rightWrd);

        constexpr TBreakWordId MaxAnnLength = 256;

        struct TQueryInfo {
            const TQuery& Query;
            const TWeights& WordWeights;
            size_t NumWords;
        };

        struct TDocInfo {
        };

        struct TAnnotationInfo {
            float Value;
            TBreakWordId Length;
        };

        struct THitInfo {
            TQueryWordId  WordId;
            TWordFormId FormId;
            TBreakWordId LeftPosition;
            TBreakWordId RightPosition;
            TBreakId BreakId;
        };

        // Hit types and filters
        //
        struct THitFlags
            : public NModule::TJsonSerializable
        {
            bool IsExact = false;
            bool IsOriginalWord = false;

            void SaveToJson(NJson::TJsonValue& value) const {
                SAVE_JSON_VAR(value, IsExact);
                SAVE_JSON_VAR(value, IsOriginalWord);
            }
        };

        struct THitFilterAny { // "any"
            bool operator() (const THitFlags&) const {
                return true;
            }
        };

        struct THitFilterExact { // "exact"
            bool operator() (const THitFlags& flags) const {
                return flags.IsExact;
            }
        };

        struct THitFilterOriginal { // "original"
            bool operator() (const THitFlags& flags) const {
                return flags.IsOriginalWord;
            }
        };

        // Adjusted hit values
        //
        using TAdjustValues = TStaticTable<EHitAdjust, float,
            EHitAdjust::Word,
            EHitAdjust::Form,
            EHitAdjust::Position,
            EHitAdjust::PosForm>;

        struct TAdjustValueKey {
            static constexpr auto Word = TAdjustValues::TKeyGen<EHitAdjust::Word>{};
            static constexpr auto Form = TAdjustValues::TKeyGen<EHitAdjust::Form>{};
            static constexpr auto Position = TAdjustValues::TKeyGen<EHitAdjust::Position>{};
            static constexpr auto PosForm = TAdjustValues::TKeyGen<EHitAdjust::PosForm>{};
        };

        using TSimpleAdjustValues = TStaticTable<EHitAdjust, float,
            EHitAdjust::Word>;

        // Full match types
        //
        using TFullMatchValues = TStaticTable<EFullMatch, float,
            EFullMatch::Prefix,
            EFullMatch::Full>;

        struct TFullMatchValueKey {
            static constexpr auto Prefix = TFullMatchValues::TKeyGen<EFullMatch::Prefix>{};
            static constexpr auto Full = TFullMatchValues::TKeyGen<EFullMatch::Full>{};
        };

        UNIT(TCoreUnit) {
            UNIT_STATE {
                const TQuery* Query = nullptr;
                TWeights WordWeights;
                TQueryWordId QueryLength = Max<TQueryWordId>();

                float AnnValue = NAN;
                TBreakWordId AnnLength = Max<TBreakWordId>();

                THitFlags Flags;

                alignas(ui64) TAdjustValues CoeffVals;
                alignas(ui64) TAdjustValues IdfVals;
                alignas(ui64) TAdjustValues WeightVals;

                TBreakWordId LastPosition = Max<TBreakWordId>();
                TBreakWordId LastLeftPosition = Max<TBreakWordId>();
                TQueryWordId LastWordId = Max<TQueryWordId>();
                float LastFormIdf = NAN;

                Y_FORCE_INLINE float GetFormAdjust() const {
                    return CoeffVals[TAdjustValueKey::Form];
                }
                Y_FORCE_INLINE float GetPositionAdjust() const {
                    return CoeffVals[TAdjustValueKey::Position];
                }
                Y_FORCE_INLINE float GetPosFormAdjust() const {
                    return CoeffVals[TAdjustValueKey::PosForm];
                }

                Y_FORCE_INLINE float GetWordWeight() const {
                    return WeightVals[TAdjustValueKey::Word];
                }
                Y_FORCE_INLINE float GetFormWeight() const {
                    return WeightVals[TAdjustValueKey::Form];
                }
                Y_FORCE_INLINE float GetPositionWeight() const {
                    return WeightVals[TAdjustValueKey::Position];
                }
                Y_FORCE_INLINE float GetPosFormWeight() const {
                    return WeightVals[TAdjustValueKey::PosForm];
                }

                Y_FORCE_INLINE float GetWordIdf() const {
                    return IdfVals[TAdjustValueKey::Word];
                }
                Y_FORCE_INLINE float GetFormIdf() const {
                    return IdfVals[TAdjustValueKey::Form];
                }
                Y_FORCE_INLINE float GetPositionIdf() const {
                    return IdfVals[TAdjustValueKey::Position];
                }
                Y_FORCE_INLINE float GetPosFormIdf() const {
                    return IdfVals[TAdjustValueKey::PosForm];
                }

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, QueryLength);
                    SAVE_JSON_VAR(value, AnnValue);
                    SAVE_JSON_VAR(value, AnnLength);
                    SAVE_JSON_VAR(value, Flags);
                    SAVE_JSON_VAR(value, CoeffVals);
                    SAVE_JSON_VAR(value, IdfVals);
                    SAVE_JSON_VAR(value, WeightVals);
                    SAVE_JSON_VAR(value, LastPosition);
                    SAVE_JSON_VAR(value, LastWordId);
                    SAVE_JSON_VAR(value, LastFormIdf);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void PrepareMatch(
                    float wordIdf, float wordWeight,
                    float formMatch, float positionMatch)
                {
                    const float posFormAdjust = positionMatch * formMatch;

                    Vars().CoeffVals[TAdjustValueKey::Form] = formMatch;
                    Vars().CoeffVals[TAdjustValueKey::Position] = positionMatch;
                    Vars().CoeffVals[TAdjustValueKey::PosForm] = posFormAdjust;

                    Vars().IdfVals.CopyFrom(NSeq4f::Mul(
                            NSeq4f::TConstSeq4f(wordIdf, TAdjustValues::Size),
                            Vars().CoeffVals.AsSeq4f()));

                    Vars().WeightVals.CopyFrom(NSeq4f::Mul(
                            NSeq4f::TConstSeq4f(wordWeight, TAdjustValues::Size),
                            Vars().CoeffVals.AsSeq4f()));
                }

                Y_FORCE_INLINE void NewQuery(TMemoryPool&, const TQueryInfo& info) {
                    Vars().Query = &info.Query;
                    Vars().WordWeights = info.WordWeights;
                    Vars().QueryLength = info.NumWords;
                    Vars().CoeffVals[TAdjustValueKey::Word] = 1.0f;
                }
                Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                    Vars().LastWordId = Max<TBreakWordId>();
                }
                Y_FORCE_INLINE void StartAnnotation(const TAnnotationInfo& info) {
                    Vars().AnnValue = info.Value;
                    Vars().AnnLength = (info.Length > 0) ? info.Length : MaxAnnLength;
                }
                Y_FORCE_INLINE void AddHit(const THitInfo& info) {
                    const TQuery* query = Vars().Query;

                    Y_ASSERT(!!query);
                    Y_ASSERT(info.WordId < query->GetNumWords());
                    Y_ASSERT(info.FormId < query->GetNumForms(info.WordId));
                    Y_ASSERT(info.LeftPosition < Vars().AnnLength);
                    Y_ASSERT(info.RightPosition < Vars().AnnLength);

                    const float wordIdf = query->GetIdf(info.WordId);
                    const float wordWeight = Vars().WordWeights[info.WordId];
                    const float positionMatch = CalcPositionAdjust(info.WordId, info.LeftPosition);

                    Vars().Flags.IsExact = false;

                    const EMatchType match = query->GetMatch(info.WordId, info.FormId);
                    const EMatchPrecisionType matchPrecision = query->GetMatchPrecision(info.WordId, info.FormId);

                    if (TMatch::OriginalWord == match) {
                        Vars().Flags.IsOriginalWord = true;
                        if (TMatchPrecision::Exact == matchPrecision) {
                            Vars().Flags.IsExact = true;
                            PrepareMatch(wordIdf, wordWeight, 1.0f, positionMatch);
                        } else if (TMatchPrecision::Lemma == matchPrecision) {
                            PrepareMatch(wordIdf, wordWeight, 0.8f, positionMatch);
                        } else {
                            PrepareMatch(wordIdf, wordWeight, 0.1f, positionMatch);
                        }
                    } else {
                        Vars().Flags.IsOriginalWord = false;
                        PrepareMatch(wordIdf, wordWeight, 0.1f, positionMatch);
                    }
                }

                Y_FORCE_INLINE void FinishHit(const THitInfo& info) {
                    Vars().LastPosition = info.RightPosition;
                    Vars().LastLeftPosition = info.LeftPosition;
                    Vars().LastWordId = info.WordId;
                    Vars().LastFormIdf = Vars().GetFormIdf();
                }
            };
        };

        // Detects and memorizes position of last occurance of
        //    each query word in current annotation
        //
        template <typename HitFilterType>
        struct TCoordinationGroup {
            using THitFilter = HitFilterType;

            UNIT(TCoordinationUnit) {
                UNIT_STATE {
                    THitFilter Filter;
                    TCoordinationAccumulator WordIds;
                    bool IsAcceptedHit = false;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, WordIds);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(TCoreUnit);

                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                        Vars().WordIds.Init(pool, info.NumWords);
                    }
                    Y_FORCE_INLINE void StartAnnotation(const TAnnotationInfo&) {
                        Vars().WordIds.Clear();
                        Vars().IsAcceptedHit = false;
                    }
                    Y_FORCE_INLINE void FinishHit(const THitInfo& info) {
                        if (!Vars().Filter(Vars<TCoreUnit>().Flags)
                            || Vars().WordIds.Contain(info.WordId))
                        {
                            Vars().IsAcceptedHit = false;
                            return;
                        }
                        Vars().WordIds.Update(info.WordId);
                        Vars().IsAcceptedHit = true;
                    }
                };
            };
        };

        using TAnyCoordinationUnit = typename TCoordinationGroup<THitFilterAny>::TCoordinationUnit;
        using TExactCoordinationUnit = typename TCoordinationGroup<THitFilterExact>::TCoordinationUnit;
        using TOriginalCoordinationUnit = typename TCoordinationGroup<THitFilterOriginal>::TCoordinationUnit;

        // Accumulates over hits detected by TCoordinationUnit
        //
        template <typename CoordUnitType>
        struct TAccumulationGroup {
            UNIT(TFullAccumulationUnit) {
                UNIT_STATE {
                    struct TWordInfo
                        : public NModule::TJsonSerializable
                    {
                        TQueryWordId Id = Max<TQueryWordId>();
                        float PosFormAdjust = NAN;

                        TWordInfo() = default;
                        TWordInfo(TQueryWordId id, float posFormAdjust)
                            : Id(id), PosFormAdjust(posFormAdjust)
                        {}

                        void SaveToJson(NJson::TJsonValue& value) const {
                            SAVE_JSON_VAR(value, Id);
                            SAVE_JSON_VAR(value, PosFormAdjust);
                        }
                    };

                    alignas(ui64) TAdjustValues SumIdfVals;
                    alignas(ui64) TAdjustValues SumWeightVals;
                    alignas(ui64) TAdjustValues SumCoeffVals;

                    TPoolPodHolder<TWordInfo> Words;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, SumIdfVals);
                        SAVE_JSON_VAR(value, SumWeightVals);
                        SAVE_JSON_VAR(value, Words);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(
                        TCoreUnit,
                        CoordUnitType
                    );

                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                        Vars().Words.Init(pool, info.NumWords);
                    }
                    Y_FORCE_INLINE void StartAnnotation(const TAnnotationInfo&) {
                        Vars().SumIdfVals.Fill(0.0f);
                        Vars().SumWeightVals.Fill(0.0f);
                        Vars().SumCoeffVals.Fill(0.0f);
                        Vars().Words.Clear();
                    }
                    Y_FORCE_INLINE void FinishHit(const THitInfo& info) {
                        if (!Vars<CoordUnitType>().IsAcceptedHit) {
                            return;
                        }

                        const auto& coreVars = Vars<TCoreUnit>();

                        Vars().Words.Emplace(info.WordId, coreVars.GetPosFormAdjust());
                        Y_ASSERT(Vars().Words.Count() == Vars<CoordUnitType>().WordIds.Count());

                        Vars().SumIdfVals.AddFrom(coreVars.IdfVals);
                        Vars().SumWeightVals.AddFrom(coreVars.WeightVals);
                        Vars().SumCoeffVals.AddFrom(coreVars.CoeffVals);
                    }
                };
            };

            UNIT(TSimpleAccumulationUnit) {
                UNIT_STATE {
                    TSimpleAdjustValues SumWeightVals;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, SumWeightVals);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(CoordUnitType);

                    Y_FORCE_INLINE void StartAnnotation(const TAnnotationInfo&) {
                        Vars().SumWeightVals.Fill(0.0f);
                    }
                    Y_FORCE_INLINE void FinishHit(const THitInfo&) {
                        if (!Vars<CoordUnitType>().IsAcceptedHit) {
                            return;
                        }
                        Vars().SumWeightVals[TAdjustValueKey::Word] += Vars<TCoreUnit>().WeightVals[TAdjustValueKey::Word];
                        Y_ASSERT(Vars().SumWeightVals[TAdjustValueKey::Word] <= 1.0f + FloatEpsilon);
                    }
                };
            };
        };

        using TAnyAccumulationUnit = typename TAccumulationGroup<TAnyCoordinationUnit>::TFullAccumulationUnit;
        using TOriginalAccumulationUnit = typename TAccumulationGroup<TOriginalCoordinationUnit>::TFullAccumulationUnit;
        using TExactAccumulationUnit = typename TAccumulationGroup<TExactCoordinationUnit>::TSimpleAccumulationUnit;

        // Detects and memorizes positions in current annotation
        // that are covered by hits
        //
        template <typename HitFilterType>
        struct TCoverageGroup {
            using THitFilter = HitFilterType;

            UNIT(TCoverageUnit) {
                UNIT_STATE {
                    THitFilter Filter;
                    TCoordinationAccumulator WordPosAcc;
                    TBreakWordId NumPosCoveredByHit = Max<TBreakWordId>();

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, WordPosAcc);
                        SAVE_JSON_VAR(value, NumPosCoveredByHit);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(TCoreUnit);

                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo&) {
                        Vars().WordPosAcc.Init(pool, MaxAnnLength);
                    }
                    Y_FORCE_INLINE void StartAnnotation(const TAnnotationInfo&) {
                        Vars().WordPosAcc.Clear();
                        Vars().NumPosCoveredByHit  = 0;
                    }
                    Y_FORCE_INLINE void AddHit(const THitInfo& info) {
                        if (!Vars().Filter(Vars<TCoreUnit>().Flags)) {
                            return;
                        }

                        Vars().NumPosCoveredByHit = 0;
                        for (TBreakWordId pos : xrange<TBreakWordId>(info.LeftPosition, info.RightPosition + 1)) {
                            if (!Vars().WordPosAcc.Contain(pos)) {
                                Vars().NumPosCoveredByHit += 1;
                                Vars().WordPosAcc.Update(pos);
                                Y_ASSERT(Vars().WordPosAcc.Count() <= Vars<TCoreUnit>().AnnLength);
                            }
                        }
                    }
                };
            };
        };

        using TAnyCoverageUnit = typename TCoverageGroup<THitFilterAny>::TCoverageUnit;
        using TOriginalCoverageUnit = typename TCoverageGroup<THitFilterOriginal>::TCoverageUnit;
        using TExactCoverageUnit = typename TCoverageGroup<THitFilterExact>::TCoverageUnit;

        // Computes max value of annotations that have
        // same words as query and in the same order.
        //
        // TODO: Maybe use match accumulator instead of just Max?
        //
        template <typename HitFilterType, bool IsSimpleMatch = false>
        struct TFullMatchGroup {
            using THitFilter = HitFilterType;
            using TIsSimpleMatch = std::integral_constant<bool, IsSimpleMatch>;

            UNIT(TFullMatchUnit) {
                UNIT_STATE {
                    THitFilter Filter;
                    TBreakWordId NumPosCovered = Max<TBreakWordId>();

                    TFullMatchValues Values;

                    Y_FORCE_INLINE float GetFullValue() const {
                        return Values[TFullMatchValueKey::Full];
                    }
                    Y_FORCE_INLINE float GetPrefixValue() const {
                        return Values[TFullMatchValueKey::Prefix];
                    }

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, NumPosCovered);
                        SAVE_JSON_VAR(value, Values);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(
                        TCoreUnit,
                        NModule::TUnitIf<!IsSimpleMatch, TAnyCoverageUnit>
                    );

                    Y_FORCE_INLINE bool CheckHit(std::true_type, // IsSimpleMatch = true
                        const THitFlags& flags, const THitInfo& info)
                    {
                        return Vars().Filter(flags)
                            && info.LeftPosition == info.WordId
                            && info.LeftPosition == Vars().NumPosCovered;
                    }
                    Y_FORCE_INLINE bool CheckHit(std::false_type, // IsSimpleMatch = false
                        const THitFlags& flags, const THitInfo& info)
                    {
                        return Vars().Filter(flags)
                            && info.LeftPosition == info.WordId
                            && info.RightPosition == info.WordId
                            && info.LeftPosition == Vars().NumPosCovered
                            && Vars<TAnyCoverageUnit>().NumPosCoveredByHit != 0;
                    }

                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().Values.Fill(0.0f);
                    }
                    Y_FORCE_INLINE void StartAnnotation(const TAnnotationInfo&) {
                        Vars().NumPosCovered = 0;
                    }
                    Y_FORCE_INLINE void AddHit(const THitInfo& info) {
                        const auto& flags = Vars<TCoreUnit>().Flags;

                        if (CheckHit(TIsSimpleMatch{}, flags, info)) {
                            ++Vars().NumPosCovered;

                            Y_ASSERT(Vars().NumPosCovered <= Vars<TCoreUnit>().AnnLength);
                            Y_ASSERT(Vars().NumPosCovered <= Vars<TCoreUnit>().QueryLength);
                        }
                    }
                    Y_FORCE_INLINE void FinishAnnotation(const TAnnotationInfo&) {
                        Y_ASSERT(Vars<TCoreUnit>().Query);

                        if (Vars().NumPosCovered == Vars<TCoreUnit>().QueryLength) {
                            const float annValue = Vars<TCoreUnit>().AnnValue;
                            Vars().Values[TFullMatchValueKey::Prefix] = Max(
                                Vars().Values[TFullMatchValueKey::Prefix],
                                annValue);

                            if (Vars().NumPosCovered == Vars<TCoreUnit>().AnnLength) {
                                Vars().Values[TFullMatchValueKey::Full] = Max(
                                    Vars().Values[TFullMatchValueKey::Full],
                                    Vars<TCoreUnit>().AnnValue);
                            }
                        }
                        Y_ASSERT(Vars().GetFullValue() <= Vars().GetPrefixValue());
                    }
                };
            };
        };

        using TAnyFullMatchUnit = typename TFullMatchGroup<THitFilterAny>::TFullMatchUnit;
        using TExactFullMatchUnit = typename TFullMatchGroup<THitFilterExact, true>::TFullMatchUnit;
        using TOriginalFullMatchUnit = typename TFullMatchGroup<THitFilterOriginal>::TFullMatchUnit;

        // Computes match(sum-of-weights(ann), ann value)
        // sum-of-weights(ann) is an output of accumulation unit above
        //
        template <typename AccUnitType, EHitAdjust ValueKey>
        struct TWcmGroup {
            UNIT_FAMILY(TWcmFamily)

            template <typename Accumulator>
            UNIT(TWcmStub) {
                REQUIRE_MACHINE(MatchAccumulator, Accumulator);

                UNIT_FAMILY_STATE(TWcmFamily) {
                    Accumulator WcmAcc;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, WcmAcc);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(
                        TCoreUnit,
                        AccUnitType
                    );

                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().WcmAcc.Clear();
                    }
                    Y_FORCE_INLINE void FinishAnnotation(const TAnnotationInfo&) {
                        Vars().WcmAcc.Update(
                            Vars<AccUnitType>().SumWeightVals[TAdjustValues::TKeyGen<ValueKey>{}],
                            Vars<TCoreUnit>().AnnValue);
                    }
                };
            };
        };

        using TAnyWcmGroup = TWcmGroup<TAnyAccumulationUnit, EHitAdjust::Word>;
        using TAnyWcmFamily = typename TAnyWcmGroup::TWcmFamily;
        template <typename Acc> using TAnyWcmStub = typename TAnyWcmGroup::TWcmStub<Acc>;

        using TExactWcmGroup = TWcmGroup<TExactAccumulationUnit, EHitAdjust::Word>;
        using TExactWcmFamily = typename TExactWcmGroup::TWcmFamily;
        template <typename Acc> using TExactWcmStub = typename TExactWcmGroup::TWcmStub<Acc>;

        // Computes match(1.0, ann-value) if (is-sub-sequence(query, ann))
        //
        template <typename HitFilterType>
        struct TInfixMatchGroup {
            using THitFilter = HitFilterType;
            UNIT_FAMILY(TInfixMatchFamily)

            template <typename Accumulator>
            UNIT(TInfixMatchStub) {
                REQUIRE_MACHINE(MatchAccumulator, Accumulator);

                UNIT_FAMILY_STATE(TInfixMatchFamily) {
                    THitFilter Filter;
                    Accumulator MatchAcc;
                    TSequenceAccumulator SeqAcc;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, MatchAcc);
                        SAVE_JSON_VAR(value, SeqAcc);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(TCoreUnit);

                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                        Vars().SeqAcc.Init(pool, info.NumWords);
                    }
                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().MatchAcc.Clear();
                    }
                    Y_FORCE_INLINE void StartAnnotation(const TAnnotationInfo&) {
                        Vars().SeqAcc.Clear();
                    }
                    Y_FORCE_INLINE void AddHit(const THitInfo& info) {
                        if (Vars().Filter(Vars<TCoreUnit>().Flags)) {
                            Vars().SeqAcc.Update(info.WordId, info.LeftPosition);
                        }
                    }
                    Y_FORCE_INLINE void FinishAnnotation(const TAnnotationInfo&) {
                        Y_ASSERT(Vars<TCoreUnit>().Query);

                        const size_t maxSequenceLength = Vars().SeqAcc.GetMaxSequenceLength();
                        if (Vars<TCoreUnit>().QueryLength == maxSequenceLength) {
                            Vars().MatchAcc.Update(1.0f, Vars<TCoreUnit>().AnnValue);
                        }
                    }
                };
            };
        };

        using TAnyInfixMatchGroup = TInfixMatchGroup<THitFilterAny>;
        using TAnyInfixMatchFamily = typename TAnyInfixMatchGroup::TInfixMatchFamily;
        template <typename Acc> using TAnyInfixMatchStub = typename TAnyInfixMatchGroup::TInfixMatchStub<Acc>;

        using TExactInfixMatchGroup = TInfixMatchGroup<THitFilterExact>;
        using TExactInfixMatchFamily = typename TExactInfixMatchGroup::TInfixMatchFamily;
        template <typename Acc> using TExactInfixMatchStub = typename TExactInfixMatchGroup::TInfixMatchStub<Acc>;

        // Computes
        //    annotation match = match(sum-of-weights, ann value) if (is-ann-covered)
        //    mix match = match(1.0, ann value) if (is-ann-covered && is-query-covered)
        //    cosine match = match(cosine-weight, ann value)
        //      where cosine-weight = num-query-words-covered / sqrt(num-query-words * num-ann-words-corr)
        //          where num-ann-words-corr = num-ann-words - num-ann-words-covered + num-query-words-covered
        //
        template <
            typename CoordUnitType,
            typename AccUnitType,
            typename CoverUnitType,
            EHitAdjust ValueKey>
        struct TAnnotationMatchGroup {
            UNIT_FAMILY(TAnnotationMatchFamily);

            template <typename Accumulator>
            UNIT(TAnnotationMatchStub) {
                REQUIRE_MACHINE(MatchAccumulator, Accumulator);

                UNIT_FAMILY_STATE(TAnnotationMatchFamily) {
                    Accumulator AnnotationMatchAcc;
                    Accumulator MixMatchAcc;
                    Accumulator CosineMatchAcc;
                    ui64 QueryMatchCount = 0;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, AnnotationMatchAcc);
                        SAVE_JSON_VAR(value, MixMatchAcc);
                        SAVE_JSON_VAR(value, CosineMatchAcc);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(
                        TCoreUnit,
                        CoordUnitType,
                        AccUnitType,
                        CoverUnitType
                    );

                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().AnnotationMatchAcc.Clear();
                        Vars().MixMatchAcc.Clear();
                        Vars().CosineMatchAcc.Clear();
                        Vars().QueryMatchCount = 0;
                    }
                    Y_FORCE_INLINE void FinishAnnotation(const TAnnotationInfo&) {
                        Y_ASSERT(Vars<TCoreUnit>().Query);

                        const TQueryWordId queryLength = Vars<TCoreUnit>().QueryLength;
                        const TQueryWordId queryWordCovered = Vars<CoordUnitType>().WordIds.Count();

                        const TBreakWordId annLength = Vars<TCoreUnit>().AnnLength;
                        const TBreakWordId annWordCovered = Vars<CoverUnitType>().WordPosAcc.Count();

                        const float annValue = Vars<TCoreUnit>().AnnValue;

                        if (annLength == annWordCovered) {
                            if (queryLength == queryWordCovered) {
                                Vars().MixMatchAcc.Update(1.0f, annValue);
                            }

                            const float weightSum = Vars<AccUnitType>().SumWeightVals[TAdjustValues::TKeyGen<ValueKey>{}];
                            Vars().AnnotationMatchAcc.Update(weightSum, annValue);
                        }

                        if (queryLength == queryWordCovered) {
                            Vars().QueryMatchCount++;
                        }

                        Y_ASSERT(queryLength >= queryWordCovered);
                        if (Y_LIKELY(annLength >= annWordCovered && queryWordCovered > 0)) {
                            const TBreakWordId correctedAnnLength = annLength - annWordCovered + queryWordCovered;
                            const float cosine = float(queryWordCovered) / sqrt(float(queryLength * correctedAnnLength));
                            Y_ASSERT(cosine >= 0.0f - FloatEpsilon && cosine <= 1.0f + FloatEpsilon);
                            Vars().CosineMatchAcc.Update(cosine, annValue);
                        }
                    }
                };
            };
        };

        using TAnyAnnotationMatchGroup = TAnnotationMatchGroup<
            TAnyCoordinationUnit,
            TAnyAccumulationUnit,
            TAnyCoverageUnit,
            EHitAdjust::Word>;

        using TAnyAnnotationMatchFamily = typename TAnyAnnotationMatchGroup::TAnnotationMatchFamily;
        template <typename Acc> using TAnyAnnotationMatchStub = typename TAnyAnnotationMatchGroup::TAnnotationMatchStub<Acc>;

        using TExactAnnotationMatchGroup = TAnnotationMatchGroup<
            TExactCoordinationUnit,
            TExactAccumulationUnit,
            TExactCoverageUnit,
            EHitAdjust::Word>;

        using TExactAnnotationMatchFamily = typename TExactAnnotationMatchGroup::TAnnotationMatchFamily;
        template <typename Acc> using TExactAnnotationMatchStub = typename TExactAnnotationMatchGroup::TAnnotationMatchStub<Acc>;

        using TOriginalAnnotationMatchGroup = TAnnotationMatchGroup<
            TOriginalCoordinationUnit,
            TOriginalAccumulationUnit,
            TOriginalCoverageUnit,
            EHitAdjust::Word>;

        using TOriginalAnnotationMatchFamily = typename TOriginalAnnotationMatchGroup::TAnnotationMatchFamily;
        template <typename Acc> using TOriginalAnnotationMatchStub = typename TOriginalAnnotationMatchGroup::TAnnotationMatchStub<Acc>;
        // Computes
        //522 BclmLite from old text relevance
        template <typename HitFilterType,
            EBclmLiteHitFilter BclmFilter = EBclmLiteHitFilter::RightOnly>
        struct TBclmLiteGroup {
            using THitFilter = HitFilterType;
            UNIT(TBclmLiteUnit) {
                UNIT_STATE {
                    TAccumulatorsByFloatValue BclmAcc;
                    THitFilter Filter;
                    TBreakId LastWordBreakId;
                    size_t UniqueHits = 0;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, BclmAcc);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(
                        TCoreUnit
                    );

                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                        Vars().BclmAcc.Init(pool, info.NumWords);
                    }
                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().BclmAcc.Assign(0.0f);
                        Vars().LastWordBreakId = Max<TBreakId>();
                        Vars().UniqueHits = 0;
                    }
                    Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                        if (BclmFilter != EBclmLiteHitFilter::None) {
                            if (Vars().UniqueHits == 1) {
                                Vars().BclmAcc.Assign(0.0);
                            }
                        }
                    }
                    Y_FORCE_INLINE void AddHit(const THitInfo& info) {
                        Y_ASSERT(Vars<TCoreUnit>().Query);

                        if (!Vars().Filter(Vars<TCoreUnit>().Flags)) {
                            return;
                        }
                        constexpr TBreakWordId MAX_BREAK_ID = 2048;
                        const TBreakWordId lastRightPos = Vars<TCoreUnit>().LastPosition;
                        const TBreakId lastBrkId = Vars().LastWordBreakId;
                        const TQueryWordId lastWrdId = Vars<TCoreUnit>().LastWordId;
                        TBreakId curBrkId = info.BreakId;
                        //workaround form double hit (word + translit)
                        if (BclmFilter != EBclmLiteHitFilter::None) {
                            bool isSameLastHit =
                                lastWrdId == info.WordId &&
                                curBrkId == lastBrkId &&
                                info.RightPosition == lastRightPos;
                            if (!isSameLastHit) {
                                Vars().UniqueHits++;
                            } else {
                                return;
                            }
                        }
                        if (Vars().LastWordBreakId != Max<TBreakId>() &&
                            curBrkId < MAX_BREAK_ID)
                        {
                            float proximityValue = CalcOldBclmDist(lastBrkId, lastRightPos, curBrkId, info.LeftPosition);
                            const float attenuation = 15.0f / (15.0f + (float)curBrkId);
                            proximityValue *= attenuation;
                            Vars().BclmAcc[info.WordId] += proximityValue;
                            Vars().BclmAcc[lastWrdId] += proximityValue;
                        }
                        Vars().LastWordBreakId = curBrkId;
                    }
                };
            };
        };

        using TAnyBclmLiteGroup = TBclmLiteGroup<THitFilterAny, EBclmLiteHitFilter::RightOnly>;
        using TAnyBclmLiteUnit = typename TAnyBclmLiteGroup::TBclmLiteUnit;
        using TOriginalBclmLiteGroup = TBclmLiteGroup<THitFilterOriginal, EBclmLiteHitFilter::RightOnly>;
        using TOriginalBclmLiteUnit = typename TOriginalBclmLiteGroup::TBclmLiteUnit;
        using TExactBclmLiteGroup = TBclmLiteGroup<THitFilterExact, EBclmLiteHitFilter::RightOnly>;
        using TExactBclmLiteUnit = typename TExactBclmLiteGroup::TBclmLiteUnit;

        // Computes
        //TxtPair from old text relevance
        template <typename HitFilterType>
        struct TTxtPairGroup {
            using THitFilter = HitFilterType;
            UNIT(TTxtPairUnit) {
                UNIT_STATE {
                    TAccumulatorsByFloatValue PairAcc;
                    THitFilter Filter;
                    TPoolPodHolder<TBreakWordId> WordLastPos;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, PairAcc);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(
                        TCoreUnit
                    );

                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                        Vars().PairAcc.Init(pool, info.NumWords);
                        Vars().WordLastPos.Init(pool, Vars<TCoreUnit>().QueryLength, EStorageMode::Full);
                    }
                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().PairAcc.Assign(0.0f);
                    }
                    Y_FORCE_INLINE void StartAnnotation(const TAnnotationInfo&) {
                        Vars().WordLastPos.Fill(Max<TBreakWordId>());
                    }
                    Y_FORCE_INLINE void AddHit(const THitInfo& info) {
                        Y_ASSERT(Vars<TCoreUnit>().Query);

                        if (!Vars().Filter(Vars<TCoreUnit>().Flags)) {
                            return;
                        }

                        Vars().WordLastPos[info.WordId] = info.RightPosition;
                        if (info.WordId > 0) {
                            const auto prevWordPos = Vars().WordLastPos[info.WordId - 1];
                            if (CalcOldWordDistance(prevWordPos, info.LeftPosition) == 1 ||
                                prevWordPos == info.RightPosition)
                            {
                                Vars().PairAcc[info.WordId] += 1.0;
                            }
                        }
                    }
                };
            };
        };

        using TAnyTxtPairGroup = TTxtPairGroup<THitFilterAny>;
        using TAnyTxtPairUnit = typename TAnyTxtPairGroup::TTxtPairUnit;
        using TOriginalTxtPairGroup = TTxtPairGroup<THitFilterOriginal>;
        using TOriginalTxtPairUnit = typename TOriginalTxtPairGroup::TTxtPairUnit;
        using TExactTxtPairGroup = TTxtPairGroup<THitFilterExact>;
        using TExactTxtPairUnit = typename TExactTxtPairGroup::TTxtPairUnit;

        // Computes MaxValues[wordId] = Max(annValue), for all annotations
        //  covered by query words, including wordId
        //
        // Then MinMaxValue = Min(MaxValues)
        //
        template <typename AccUnitType, typename CoverUnitType>
        struct TPerWordAnnotationMatchGroup {
            UNIT(TPerWordAnnotationMatchUnit) {
                UNIT_STATE {
                    TAccumulatorsByFloatValue MaxValues;
                    float MinMaxValue = NAN;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, MaxValues);
                        SAVE_JSON_VAR(value, MinMaxValue);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(
                        TCoreUnit,
                        AccUnitType,
                        CoverUnitType
                    );

                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                        Vars().MaxValues.Init(pool, info.NumWords);
                    }
                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().MaxValues.Assign(0.0f);
                        Vars().MinMaxValue = 1.0f;
                    }
                    Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                        const TQueryWordId queryLength = Vars<TCoreUnit>().QueryLength;
                        for (TQueryWordId wordId : xrange(queryLength)) {
                            Vars().MinMaxValue = Min(Vars().MinMaxValue, Vars().MaxValues[wordId]);
                        }
                    }
                    Y_FORCE_INLINE void FinishAnnotation(const TAnnotationInfo&) {
                        Y_ASSERT(Vars<TCoreUnit>().Query);
                        const TBreakWordId annLength = Vars<TCoreUnit>().AnnLength;
                        const TBreakWordId annWordCovered = Vars<CoverUnitType>().WordPosAcc.Count();

                        if (annLength == annWordCovered) {
                            const auto& words = Vars<AccUnitType>().Words;
                            const float annValue = Vars<TCoreUnit>().AnnValue;
                            for (size_t i : xrange(words.Count())) {
                                const TQueryWordId wordId = words[i].Id;
                                Vars().MaxValues[wordId] = Max(Vars().MaxValues[wordId], annValue);
                            }
                        }
                    }
                };
            };
        };

        using TAnyPerWordAnnotationMatchGroup = TPerWordAnnotationMatchGroup<TAnyAccumulationUnit, TAnyCoverageUnit>;
        using TAnyPerWordAnnotationMatchUnit = TAnyPerWordAnnotationMatchGroup::TPerWordAnnotationMatchUnit;

        // Computes
        //  coverage = num-ann-words-covered / num-ann-words
        //  cm-match = (coverage * sum-of-weights) ^ 2
        //  cm-predict = cm-match * ann-value
        //
        //  match(cm-match, ann-value) for all annotations covered by query
        //
        //  MaxPredictValues[wordId] = Max(cm-predict) for all annotations
        //      covered by query words including wordId
        //  MaxMatchValues[wordId] = Max(cm-match) for all annotations
        //      covered by query words including wordId
        //
        //  MinMaxPredictValue = Min(MaxPredictValues)
        //  MinMaxMatchValue = Min(MaxMatchValues)
        //
        template <
            typename AccUnitType,
            typename CoverUnitType,
            EHitAdjust ValueKey>
        struct TCMMatchGroup {
            UNIT_FAMILY(TCMMatchFamily);

            template <typename Accumulator>
            UNIT(TCMMatchStub) {
                REQUIRE_MACHINE(MatchAccumulator, Accumulator);

                UNIT_FAMILY_STATE(TCMMatchFamily) {
                    Accumulator MatchAcc;

                    TAccumulatorsByFloatValue MaxPredictValues;
                    float MinMaxPredictValue = NAN;
                    TAccumulatorsByFloatValue MaxMatchValues;
                    float MinMaxMatchValue = NAN;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, MatchAcc);
                        SAVE_JSON_VAR(value, MaxPredictValues);
                        SAVE_JSON_VAR(value, MinMaxPredictValue);
                        SAVE_JSON_VAR(value, MaxMatchValues);
                        SAVE_JSON_VAR(value, MinMaxMatchValue);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(
                        TCoreUnit,
                        AccUnitType,
                        CoverUnitType
                    );

                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                        Vars().MaxPredictValues.Init(pool, info.NumWords);
                        Vars().MaxMatchValues.Init(pool, info.NumWords);
                    }
                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().MatchAcc.Clear();
                        Vars().MaxPredictValues.Assign(0.0f);
                        Vars().MinMaxPredictValue = 1.0f;
                        Vars().MaxMatchValues.Assign(0.0f);
                        Vars().MinMaxMatchValue = 1.0f;
                    }
                    Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                        const TQueryWordId queryLength = Vars<TCoreUnit>().QueryLength;
                        for (TQueryWordId wordId : xrange(queryLength)) {
                            Vars().MinMaxPredictValue = Min(Vars().MinMaxPredictValue, Vars().MaxPredictValues[wordId]);
                            Vars().MinMaxMatchValue = Min(Vars().MinMaxMatchValue, Vars().MaxMatchValues[wordId]);
                        }
                    }
                    Y_FORCE_INLINE void FinishAnnotation(const TAnnotationInfo&) {
                        Y_ASSERT(Vars<TCoreUnit>().Query);
                        const TBreakWordId annLength = Vars<TCoreUnit>().AnnLength;
                        const TBreakWordId annWordCovered = Vars<CoverUnitType>().WordPosAcc.Count();

                        if (Y_LIKELY(annLength >= annWordCovered && annLength > 0)) {
                            const float coverage = static_cast<float>(annWordCovered) / static_cast<float>(annLength);
                            const float weightSum = Vars<AccUnitType>().SumWeightVals[TAdjustValues::TKeyGen<ValueKey>{}];
                            const float cmMatch = Sqr(coverage * weightSum);
                            const float annValue = Vars<TCoreUnit>().AnnValue;
                            const float cmPredict = cmMatch * annValue;

                            Vars().MatchAcc.Update(cmMatch, annValue);
                            const auto& words = Vars<AccUnitType>().Words;
                            for (size_t i : xrange(words.Count())) {
                                const TQueryWordId wordId = words[i].Id;
                                Vars().MaxPredictValues[wordId] = Max(Vars().MaxPredictValues[wordId], cmPredict);
                                Vars().MaxMatchValues[wordId] = Max(Vars().MaxMatchValues[wordId], cmMatch);
                            }
                        }
                    }
                };
            };
        };

        using TAnyCMMatchGroup = TCMMatchGroup<
            TAnyAccumulationUnit,
            TAnyCoverageUnit,
            EHitAdjust::Word>;

        using TAnyCMMatchFamily = typename TAnyCMMatchGroup::TCMMatchFamily;
        template <typename Acc> using TAnyCMMatchStub = typename TAnyCMMatchGroup::TCMMatchStub<Acc>;

        // Computes
        //  Values[wordId] = Sum(pos-form-adjust * sum-of-weights) for all annotations that have wordId
        //
        template <typename AccUnitType, EHitAdjust ValueKey>
        struct TBocmGroup {
            UNIT(TBocmUnit) {
                UNIT_STATE {
                    TAccumulatorsByFloatValue Values;
                    TAccumulatorsByFloatValue DoubleValues;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, Values);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(
                        TCoreUnit,
                        AccUnitType
                    );

                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                        Vars().Values.Init(pool, info.NumWords);
                        Vars().DoubleValues.Init(pool, info.NumWords);
                    }
                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().Values.Assign(0.0f);
                        Vars().DoubleValues.Assign(0.0f);
                    }
                    Y_FORCE_INLINE void FinishAnnotation(const TAnnotationInfo&) {
                        const float wordIdfSum = Vars<AccUnitType>().SumIdfVals[TAdjustValues::TKeyGen<ValueKey>{}];
                        const float idfSumValue = wordIdfSum * Vars<TCoreUnit>().AnnValue;

                        const float annotationPosForm = Vars<AccUnitType>().SumCoeffVals[TAdjustValueKey::PosForm] * Vars<TCoreUnit>().AnnValue;

                        const auto& words = Vars<AccUnitType>().Words;
                        for (size_t i : xrange(words.Count())) {
                            Vars().Values[words[i].Id] += words[i].PosFormAdjust * idfSumValue;
                            Vars().DoubleValues[words[i].Id] += words[i].PosFormAdjust * annotationPosForm;
                        }
                    }
                };
            };
        };

        using TAnyBocmGroup = TBocmGroup<TAnyAccumulationUnit, EHitAdjust::Word>;
        using TAnyBocmUnit = typename TAnyBocmGroup::TBocmUnit;

        // Computes
        //  WordValues[wordId] = Sum(proximity * ann-value)
        //  IdfValues[wordId] = Sum(proximity * ann-value * form-idf)
        //
        //  For all annotations that have >= 2 matching query words..
        //  proximity = left-proximity + right-proximity
        //  left-proximity = proximity to nearest hit on the left, or 0
        //  right-proximity = proximity to nearest hit on the right, or 0
        //
        // TODO. Maybe split into TLastWordUnit and several parametric
        //  Blcm units that listen to it.
        //
        // TODO. Dependency on coordination unit is not really justified.
        //
        // FIXME. Current implementation appears to be buggy. See below.
        //
        template <typename CoordUnitType>
        struct TBclmGroup {
            UNIT(TBclmUnit) {
                UNIT_STATE {
                    TAccumulatorsByFloatValue WordValues;
                    TAccumulatorsByFloatValue IdfValues;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, WordValues);
                        SAVE_JSON_VAR(value, IdfValues);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(
                        TCoreUnit,
                        CoordUnitType
                    );

                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                        Vars().WordValues.Init(pool, info.NumWords);
                        Vars().IdfValues.Init(pool, info.NumWords);
                    }
                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().WordValues.Assign(0.0f);
                        Vars().IdfValues.Assign(0.0f);
                    }
                    Y_FORCE_INLINE void AddHit(const THitInfo& info) {
                        Y_ASSERT(Vars<TCoreUnit>().Query);

                        if (Vars<CoordUnitType>().WordIds.Count() > 0) {
                            Y_ASSERT(Vars<TCoreUnit>().LastWordId < Max<TBreakWordId>());
                            if (info.WordId != Vars<TCoreUnit>().LastWordId) {
                                const float proximity = CalcProximity(Vars<TCoreUnit>().LastPosition, info.LeftPosition);
                                const float proximityValue = proximity * Vars<TCoreUnit>().AnnValue;

                                Vars().WordValues[info.WordId] += proximityValue;
                                Vars().WordValues[Vars<TCoreUnit>().LastWordId] += proximityValue;

                                // FIXME. This looks like a bug. LastFormId and FormId are mixed here.
                                Vars().IdfValues[info.WordId] += proximityValue * Vars<TCoreUnit>().LastFormIdf;
                                Vars().IdfValues[Vars<TCoreUnit>().LastWordId] += proximityValue * Vars<TCoreUnit>().GetFormIdf();
                            }
                        }
                    }
                };
            };
        };

        using TAnyBclmGroup = TBclmGroup<TAnyCoordinationUnit>;
        using TAnyBclmUnit = typename TAnyBclmGroup::TBclmUnit;

        template <typename HitFilterType, TQueryWordId NgramLen>
        struct TNgramGroup {
            using THitFilter = HitFilterType;

            static_assert(NgramLen > 0, "NgramLen should be greater 0");

            UNIT(TNgramUnit) {
                UNIT_STATE {
                    THitFilter Filter;
                    TQueryWordId NgramLenReal = Max<TQueryWordId>();
                    TAccumulatorsByFloatValue MaxAcc;
                    TAccumulatorsByFloatValue SumAcc;
                    TAccumulatorsByFloatValue CntAcc;
                    TPoolPodHolder<TBreakWordId> WordLastPos;

                    float MinPerNgramMaxValue = NAN;
                    float AvgPerNgramMaxValue = NAN;
                    float AvgPerNgramAvgValue = NAN;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, NgramLenReal);
                        SAVE_JSON_VAR(value, MaxAcc);
                        SAVE_JSON_VAR(value, SumAcc);
                        SAVE_JSON_VAR(value, CntAcc);
                        SAVE_JSON_VAR(value, WordLastPos);
                        SAVE_JSON_VAR(value, MinPerNgramMaxValue);
                        SAVE_JSON_VAR(value, AvgPerNgramMaxValue);
                        SAVE_JSON_VAR(value, AvgPerNgramAvgValue);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(
                        TCoreUnit
                    );

                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& /*info*/) {
                        Vars().WordLastPos.Init(pool, Vars<TCoreUnit>().QueryLength, EStorageMode::Full);

                        TQueryWordId ngramCnt = 1;
                        if (Vars<TCoreUnit>().QueryLength < NgramLen) {
                            Vars().NgramLenReal = Vars<TCoreUnit>().QueryLength;
                        } else {
                            Vars().NgramLenReal = NgramLen;
                            ngramCnt = Vars<TCoreUnit>().QueryLength - NgramLen + 1;
                        }
                        Vars().MaxAcc.Init(pool, ngramCnt);
                        Vars().SumAcc.Init(pool, ngramCnt);
                        Vars().CntAcc.Init(pool, ngramCnt);
                    }
                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().MaxAcc.Assign(0.0f);
                        Vars().SumAcc.Assign(0.0f);
                        Vars().CntAcc.Assign(0.0f);

                        Vars().MinPerNgramMaxValue = 1.0f;
                        Vars().AvgPerNgramMaxValue = 0.0f;
                        Vars().AvgPerNgramAvgValue = 0.0f;
                    }
                    Y_FORCE_INLINE void StartAnnotation(const TAnnotationInfo& /*info*/) {
                        Vars().WordLastPos.Fill(Max<TBreakWordId>());
                    }
                    Y_FORCE_INLINE void AddHit(const THitInfo& info) {
                        if (!Vars().Filter(Vars<TCoreUnit>().Flags)) {
                            return;
                        }

                        Vars().WordLastPos[info.WordId] = info.LeftPosition;

                        if (info.WordId + 1 >= Vars().NgramLenReal) {
                            bool isMatched = true;
                            for (TQueryWordId i = 1; i < Vars().NgramLenReal; ++i) {
                                if (Vars().WordLastPos[info.WordId - i] + i != info.LeftPosition) {
                                    isMatched = false;
                                    break;
                                }
                            }
                            if (isMatched) {
                                TQueryWordId index = info.WordId + 1 - Vars().NgramLenReal;
                                const float annValue = Vars<TCoreUnit>().AnnValue;
                                Vars().MaxAcc[index] = Max(Vars().MaxAcc[index], annValue);
                                Vars().SumAcc[index] += annValue;
                                Vars().CntAcc[index] += 1.0f;
                            }
                        }
                    }
                    Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                        Y_ASSERT(Vars().MaxAcc.Size() > 0);
                        Y_ASSERT(Vars().MaxAcc.Size() == Vars().SumAcc.Size());
                        Y_ASSERT(Vars().MaxAcc.Size() == Vars().CntAcc.Size());
                        TQueryWordId ngramCnt = Vars().MaxAcc.Size();
                        for (size_t i = 0; i < ngramCnt; ++i) {
                            Vars().MinPerNgramMaxValue = Min(Vars().MinPerNgramMaxValue, Vars().MaxAcc[i]);
                            Vars().AvgPerNgramMaxValue += Vars().MaxAcc[i];
                            if (Vars().CntAcc[i] > 0) {
                                Vars().AvgPerNgramAvgValue += Vars().SumAcc[i] / Vars().CntAcc[i];
                            }
                        }
                        float ngramCntInv = 1.0f / float(ngramCnt);
                        Vars().AvgPerNgramMaxValue *= ngramCntInv;
                        Vars().AvgPerNgramAvgValue *= ngramCntInv;
                    }
                };
            };
        };

        using TAnyTrigramGroup = TNgramGroup<THitFilterAny, 3>;
        using TAnyTrigramUnit = typename TAnyTrigramGroup::TNgramUnit;

        template <
            typename HitFilterType
        >
        struct TQueryPartMatchGroup {
            UNIT_FAMILY(TQueryPartMatchFamily);

            using TCoordinationUnit = typename TCoordinationGroup<HitFilterType>::TCoordinationUnit;
            using TCoverageUnit = typename TCoverageGroup<HitFilterType>::TCoverageUnit;

            template <
                typename Accumulator
            >
            UNIT(TQueryPartMatchStub) {
                REQUIRE_MACHINE(MatchAccumulator, Accumulator);

                UNIT_FAMILY_STATE(TQueryPartMatchFamily) {
                    Accumulator MatchAcc;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, MatchAcc);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(
                        TCoreUnit,
                        TCoordinationUnit,
                        TCoverageUnit
                    );

                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().MatchAcc.Clear();
                    }
                    Y_FORCE_INLINE void FinishAnnotation(const TAnnotationInfo&) {
                        const size_t wordCount = Vars<TCoordinationUnit>().WordIds.Count();

                        if (wordCount > 0 && wordCount < Vars<TCoreUnit>().QueryLength) {
                            const size_t posCount = Vars<TCoverageUnit>().WordPosAcc.Count();
                            const float coverageFrac = float(posCount) / float(Vars<TCoreUnit>().AnnLength);

                            Vars().MatchAcc.Update(coverageFrac, Vars<TCoreUnit>().AnnValue);
                        }
                    }
                };
            };
        };

        using TAnyQueryPartMatchGroup = TQueryPartMatchGroup<THitFilterAny>;
        using TAnyQueryPartMatchFamily = TAnyQueryPartMatchGroup::TQueryPartMatchFamily;
        template <typename Accumulator>
        using TAnyQueryPartMatchStub = typename TAnyQueryPartMatchGroup::TQueryPartMatchStub<Accumulator>;

    } // MACHINE_PARTS(AnnotationAccumulator)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
