#pragma once

#include <kernel/text_machine/parts/common/types.h>

#include <util/generic/utility.h>
#include <util/generic/ymath.h>
#include <util/generic/ylimits.h>
#include <util/system/yassert.h>
#include <util/system/defaults.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(MatchAccumulator) {
        UNIT(TBaseUnit) {
            UNIT_STATE {
                size_t Count = Max<size_t>();
                float SumMatch = NAN;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, Count);
                    SAVE_JSON_VAR(value, SumMatch);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void Clear() {
                    Count = 0;
                    SumMatch = 0.0;
                }
                Y_FORCE_INLINE void Update(float match, float value) {
                    Y_ASSERT(match >= 0.0f - FloatEpsilon);
                    Y_ASSERT(match <= 1.0f + FloatEpsilon);
                    Y_ASSERT(value >= 0.0f - FloatEpsilon);
                    Y_ASSERT(value <= 1.0f + FloatEpsilon);
                    Count += 1;
                    SumMatch += match;
                }

            public:
                Y_FORCE_INLINE float CalcCount() const {
                    return float(Count) / float(Count + 5);
                }
                Y_FORCE_INLINE float CalcSumMatch() const {
                    return SumMatch / (SumMatch + 1.0);
                }
                Y_FORCE_INLINE float CalcAvgMatch() const {
                    if (Count == 0) {
                        return 0;
                    }
                    const float avg = SumMatch / float(Count);
                    Y_ASSERT(avg <= 1.0 + FloatEpsilon);
                    return avg;
                }
            };
        };

        UNIT(TMaxUnit) {
            UNIT_STATE {
                float MaxMatch = NAN;
                float MaxMatchValue = NAN;
                float MaxValue = NAN;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, MaxMatch);
                    SAVE_JSON_VAR(value, MaxMatchValue);
                    SAVE_JSON_VAR(value, MaxValue);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void Clear() {
                    MaxMatch = 0.0f;
                    MaxMatchValue = 0.0f;
                    MaxValue = 0.0f;
                }

                Y_FORCE_INLINE void Update(float match, float value) {
                    if (match > MaxMatch) {
                        MaxMatch = match;
                        MaxMatchValue = value;
                    } else if (match == MaxMatch) {
                        MaxMatchValue = Max(MaxMatchValue, value);
                    }

                    MaxValue = Max(MaxValue, value);
                }

            public:
                Y_FORCE_INLINE float CalcMaxMatch() const {
                    return MaxMatch;
                }
                Y_FORCE_INLINE float CalcMaxMatchValue() const {
                    return MaxMatchValue;
                }
                Y_FORCE_INLINE float CalcMaxMatchPrediction() const {
                    return MaxMatch * MaxMatchValue;
                }
                Y_FORCE_INLINE float CalcMaxValue() const {
                    return MaxValue;
                }
            };
        };

        UNIT(TMatchAllUnit) {
            UNIT_STATE {
                float MatchAllSumValue = NAN;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, MatchAllSumValue);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void Clear() {
                    MatchAllSumValue = 0.0f;
                }
                Y_FORCE_INLINE void Update(float, float value) {
                    MatchAllSumValue += value;
                }
                Y_FORCE_INLINE float CalcSumValue() const {
                    return MatchAllSumValue / (0.5f + MatchAllSumValue);
                }
            };
        };

        UNIT(TMatch80Unit) {
            UNIT_STATE {
                size_t Match80Count = Max<size_t>();
                float Match80MaxValue = NAN;
                float Match80SumValue = NAN;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, Match80Count);
                    SAVE_JSON_VAR(value, Match80MaxValue);
                    SAVE_JSON_VAR(value, Match80SumValue);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void Clear() {
                    Match80Count = 0;
                    Match80MaxValue = 0.0f;
                    Match80SumValue = 0.0f;
                }
                Y_FORCE_INLINE void Update(float match, float value) {
                    if (match >= 0.8f) {
                        ++Match80Count;
                        Match80SumValue += value;
                        Match80MaxValue = Max(Match80MaxValue, value);
                    }
                }

            public:
                Y_FORCE_INLINE float CalcCount() const {
                    return float(Match80Count) / float(Match80Count + 5);
                }
                Y_FORCE_INLINE float CalcMaxValue() const {
                    return Match80MaxValue;
                }
                Y_FORCE_INLINE float CalcAvgValue() const {
                    if (Match80Count == 0) {
                        return 0;
                    }
                    const float avg = Match80SumValue / float(Match80Count);
                    Y_ASSERT(avg <= 1.0f + FloatEpsilon);
                    return avg;
                }
            };
        };

        UNIT(TMatch95Unit) {
            UNIT_STATE {
                size_t Match95Count = Max<size_t>();
                float Match95MaxValue = NAN;
                float Match95SumValue = NAN;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, Match95Count);
                    SAVE_JSON_VAR(value, Match95MaxValue);
                    SAVE_JSON_VAR(value, Match95SumValue);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void Clear() {
                    Match95Count = 0;
                    Match95MaxValue = 0.0;
                    Match95SumValue = 0.0;
                }
                Y_FORCE_INLINE void Update(float match, float value) {
                    if (match >= 0.95f) {
                        ++Match95Count;
                        Match95SumValue += value;
                        Match95MaxValue = Max(Match95MaxValue, value);
                    }
                }

            public:
                Y_FORCE_INLINE float CalcCount() const {
                    return float(Match95Count) / float(Match95Count + 5);
                }
                Y_FORCE_INLINE float CalcMaxValue() const {
                    return Match95MaxValue;
                }
                Y_FORCE_INLINE float CalcAvgValue() const {
                    if (Match95Count == 0) {
                        return 0;
                    }
                    const float avg = Match95SumValue / float(Match95Count);
                    Y_ASSERT(avg <= 1.0 + FloatEpsilon);
                    return avg;
                }
            };
        };

        UNIT(TPredictionUnit) {
            UNIT_STATE {
                float MaxPrediction = NAN;
                float SumPrediction = NAN;
                float SumMatchPrediction = NAN;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, MaxPrediction);
                    SAVE_JSON_VAR(value, SumPrediction);
                    SAVE_JSON_VAR(value, SumMatchPrediction);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void Clear() {
                    MaxPrediction = 0.0f;
                    SumPrediction = 0.0f;
                    SumMatchPrediction = 0.0f;
                }
                Y_FORCE_INLINE void Update(float match, float value) {
                    const float prediction = match * value;
                    MaxPrediction = Max(MaxPrediction, prediction);
                    SumPrediction += prediction;
                    const float matchPrediction = match * prediction;
                    SumMatchPrediction += matchPrediction;
                }

            public:
                Y_FORCE_INLINE float CalcMaxPrediction() const {
                    return MaxPrediction;
                }
                Y_FORCE_INLINE float CalcWeightedPrediction() const {
                    if (Vars<TBaseUnit>().SumMatch <= 0) {
                        return 0;
                    }
                    const float avg = SumMatchPrediction / Vars<TBaseUnit>().SumMatch;
                    Y_ASSERT(avg <= 1.0 + FloatEpsilon);
                    return avg;
                }
                Y_FORCE_INLINE float CalcWeightedValue() const {
                    if (Vars<TBaseUnit>().SumMatch <= 0) {
                        return 0;
                    }
                    const float avg = SumPrediction / Vars<TBaseUnit>().SumMatch;
                    Y_ASSERT(avg <= 1.0 + FloatEpsilon);
                    return avg;
                }
            };
        };

        template <size_t MaxTopElements>
        struct TTopAccGroup {
            UNIT_FAMILY(TTopFamily)

            static_assert(MaxTopElements > 0, "MaxTopElements should be greater 0");

            UNIT(TTopUnit) {
                struct TMatchValuePair : public NModule::TJsonSerializable {
                    float Match = 0.0f;
                    float Value = 0.0f;
                    void SaveToJson(NJson::TJsonValue& jsonValue) const {
                        SAVE_JSON_VAR(jsonValue, Match);
                        SAVE_JSON_VAR(jsonValue, Value);
                    }
                };
                UNIT_FAMILY_STATE(TTopFamily) {
                    //Annotations that are most similar to query
                    //TopElements[0].Match >= TopElements[1].Match >= ... >= TopElements[MaxTopElements - 1].Match
                    std::array<TMatchValuePair, MaxTopElements> TopElements;
                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, TopElements);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    Y_FORCE_INLINE void Clear() {
                        Vars().TopElements.fill(TMatchValuePair());
                    }
                    Y_FORCE_INLINE void Update(float match, float value) {
                        if (match <= Vars().TopElements.back().Match) {
                            return;
                        }
                        size_t posToInsert = MaxTopElements - 1;
                        while (posToInsert > 0 && match > Vars().TopElements[posToInsert - 1].Match) {
                            Vars().TopElements[posToInsert] = Vars().TopElements[posToInsert - 1];
                            --posToInsert;
                        }
                        Vars().TopElements[posToInsert].Match = match;
                        Vars().TopElements[posToInsert].Value = value;
                    }

                    Y_FORCE_INLINE float CalcAvgPrediction() const {
                        float sum = 0.0f;
                        for (size_t i : xrange(MaxTopElements)) {
                            sum += Vars().TopElements[i].Match * Vars().TopElements[i].Value;
                        }
                        return sum / static_cast<float>(MaxTopElements);
                    }
                    Y_FORCE_INLINE float CalcAvgMatch() const {
                        float sum = 0.0f;
                        for (size_t i : xrange(MaxTopElements)) {
                            sum += Vars().TopElements[i].Match;
                        }
                        return sum / static_cast<float>(MaxTopElements);
                    }
                    Y_FORCE_INLINE float CalcAvgValue() const {
                        float sum = 0.0f;
                        for (size_t i : xrange(MaxTopElements)) {
                            sum += Vars().TopElements[i].Value;
                        }
                        return sum / static_cast<float>(MaxTopElements);
                    }
                };
            };
        };

    #define UNIT_TOP(UnitName, MaxTopElements) \
        using T##UnitName##Family = TTopAccGroup<MaxTopElements>::TTopFamily; \
        using T##UnitName##Unit = TTopAccGroup<MaxTopElements>::TTopUnit;

            UNIT_TOP(Top5, 5)

    #undef UNIT_TOP

        MACHINE_MOTOR {
        public:
            using M::Clear;
            using M::Update;

        public:
            Y_FORCE_INLINE const typename M::template TGetProc<TBaseUnit>& GetBaseProc() const {
                return M::template Proc<TBaseUnit>();
            }
            Y_FORCE_INLINE const typename M::template TGetProc<TMaxUnit>& GetMaxProc() const {
                return M::template Proc<TMaxUnit>();
            }
            Y_FORCE_INLINE const typename M::template TGetProc<TMatchAllUnit>& GetMatchAllProc() const {
                return M::template Proc<TMatchAllUnit>();
            }
            Y_FORCE_INLINE const typename M::template TGetProc<TMatch80Unit>& GetMatch80Proc() const {
                return M::template Proc<TMatch80Unit>();
            }
            Y_FORCE_INLINE const typename M::template TGetProc<TMatch95Unit>& GetMatch95Proc() const {
                return M::template Proc<TMatch95Unit>();
            }
            Y_FORCE_INLINE const typename M::template TGetProc<TPredictionUnit>& GetPredictionProc() const {
                return M::template Proc<TPredictionUnit>();
            }
            Y_FORCE_INLINE const typename M::template TGetProc<TTop5Unit>& GetTop5Proc() const {
                return M::template Proc<TTop5Unit>();
            }

            Y_FORCE_INLINE float CalcCount() const {
                return GetBaseProc().CalcCount();
            }
            Y_FORCE_INLINE float CalcSumMatch() const {
                return GetBaseProc().CalcSumMatch();
            }
            Y_FORCE_INLINE float CalcAvgMatch() const {
                return GetBaseProc().CalcAvgMatch();
            }
            Y_FORCE_INLINE float CalcMaxMatch() const {
                return GetMaxProc().CalcMaxMatch();
            }
            Y_FORCE_INLINE float CalcMaxMatchValue() const {
                return GetMaxProc().CalcMaxMatchValue();
            }
            Y_FORCE_INLINE float CalcMaxAnyValue() const {//TODO: rename CalcMaxValue to CalcMax80Value
                return GetMaxProc().CalcMaxValue();
            }
            Y_FORCE_INLINE float CalcMaxMatchPrediction() const {
                return GetMaxProc().CalcMaxMatchPrediction();
            }
            Y_FORCE_INLINE float CalcMatchAllSumValue() const {
                return GetMatchAllProc().CalcSumValue();
            }
            Y_FORCE_INLINE float CalcMatch80Count() const {
                return GetMatch80Proc().CalcCount();
            }
            Y_FORCE_INLINE float CalcMatch80MaxValue() const {
                return GetMatch80Proc().CalcMaxValue();
            }
            Y_FORCE_INLINE float CalcMatch80AvgValue() const {
                return GetMatch80Proc().CalcAvgValue();
            }
            Y_FORCE_INLINE float CalcMatch95Count() const {
                return GetMatch95Proc().CalcCount();
            }
            Y_FORCE_INLINE float CalcMatch95MaxValue() const {
                return GetMatch95Proc().CalcMaxValue();
            }
            Y_FORCE_INLINE float CalcMatch95AvgValue() const {
                return GetMatch95Proc().CalcAvgValue();
            }
            Y_FORCE_INLINE float CalcMaxPrediction() const {
                return GetPredictionProc().CalcMaxPrediction();
            }
            Y_FORCE_INLINE float CalcWeightedPrediction() const {
                return GetPredictionProc().CalcWeightedPrediction();
            }
            Y_FORCE_INLINE float CalcWeightedValue() const {
                return GetPredictionProc().CalcWeightedValue();
            }
            Y_FORCE_INLINE float CalcTop5AvgPrediction() const {
                return GetTop5Proc().CalcAvgPrediction();
            }
            Y_FORCE_INLINE float CalcTop5AvgMatch() const {
                return GetTop5Proc().CalcAvgMatch();
            }
            Y_FORCE_INLINE float CalcTop5AvgValue() const {
                return GetTop5Proc().CalcAvgValue();
            }
        };
    } // MACHINE_PARTS(MatchAccumulator)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
