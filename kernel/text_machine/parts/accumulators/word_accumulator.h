#pragma once

#include <kernel/text_machine/parts/common/floats.h>
#include <kernel/text_machine/parts/common/storage.h>
#include <library/cpp/pop_count/popcount.h>

namespace NTextMachine {
namespace NCore {
    namespace NPrivate {
        template <typename BaseType>
        class TCalcOps
            : public BaseType
        {
        public:
            using TBase = BaseType;

            // Bm15 family
            //
            template <typename SeqTypeW>
            float CalcBm15(SeqTypeW&& weights, float k) const {
                Y_ASSERT(!IsNan(k));
                return NSeq4f::CalcBm15(TBase::AsSeq4f(), std::forward<SeqTypeW>(weights), k);
            }
            float CalcBm15W0(float k) const {
                Y_ASSERT(!IsNan(k));
                return NSeq4f::CalcBm15W0(TBase::AsSeq4f(), k);
            }
            template <typename SeqTypeW>
            float CalcBm15Plus(SeqTypeW&& weights, float k, float plus) const {
                Y_ASSERT(!IsNan(k));
                Y_ASSERT(!IsNan(plus));
                return NSeq4f::CalcBm15Plus(TBase::AsSeq4f(), std::forward<SeqTypeW>(weights), k, plus);
            }
            float CalcBm15PlusW0(float k, float plus) const {
                Y_ASSERT(!IsNan(k));
                Y_ASSERT(!IsNan(plus));
                return NSeq4f::CalcBm15PlusW0(TBase::AsSeq4f(), k, plus);
            }
            template <typename SeqTypeW>
            float CalcBm15Max(SeqTypeW&& weights, float k) const {
                Y_ASSERT(!IsNan(k));
                return NSeq4f::CalcBm15Max(TBase::AsSeq4f(), std::forward<SeqTypeW>(weights), k);
            }
            template <typename SeqTypeW>
            float CalcCoverageEstimation(SeqTypeW&& weights, float cutOffValue) const {
                Y_ASSERT(!IsNan(cutOffValue) && cutOffValue > 0);
                return NSeq4f::CalcCoverageEstimation(TBase::AsSeq4f(), std::forward<SeqTypeW>(weights), cutOffValue);
            }


            // Bm11 family (=Bm15 with specific k)
            //
            template <typename SeqTypeW>
            float CalcBm11(SeqTypeW&& weights, int textSize, int textNormalizer) const {
                Y_ASSERT(textSize >= 0);
                Y_ASSERT(textNormalizer > 0);
                return CalcBm15(std::forward<SeqTypeW>(weights), float(textSize) / float(textNormalizer));
            }
            float CalcBm11W0(int textSize, int textNormalizer) const {
                Y_ASSERT(textSize >= 0);
                Y_ASSERT(textNormalizer > 0);
                return CalcBm15W0(float(textSize) / float(textNormalizer));
            }
            template <typename SeqTypeW>
            float CalcBm11Plus(SeqTypeW&& weights, int textSize, int textNormalizer, float plus) const {
                Y_ASSERT(textSize >= 0);
                Y_ASSERT(textNormalizer > 0);
                return CalcBm15Plus(std::forward<SeqTypeW>(weights), float(textSize) / float(textNormalizer), plus);
            }
            float CalcBm11PlusW0(int textSize, int textNormalizer, float plus) const {
                Y_ASSERT(textSize >= 0);
                Y_ASSERT(textNormalizer > 0);
                return CalcBm15PlusW0(float(textSize) / float(textNormalizer), plus);
            }
        };

        struct TTextFormsAvg {
            size_t Length = 0;

            TTextFormsAvg(size_t length)
                : Length(length)
            {}

            float UpdateAnswer(float old, float update) {
                return old + update;
            }

            float PostProcess(float result) {
                if (Length > 0) {
                    return result / (float) Length;
                } else {
                    return result;
                }
            }
        };

        struct TTextFormsMax {
            float UpdateAnswer(float old, float update) {
                return Max(old, update);
            }

            float PostProcess(float result) {
                return result;
            }
        };

        struct TTextFormsWeightedAvg {
            //const TPoolPodHolder<float>& Idf;
            const TPodBuffer<float>& Idf;
            size_t Index = 0;

            TTextFormsWeightedAvg(const TPoolPodHolder<float>& idf)
                : Idf(idf)
            {}
            float UpdateAnswer(float old, float update) {
                return old + update * Idf[Index++];
            }

            float PostProcess(float result) {
                return result / (result + 1.0);
            }
        };

        template <typename BaseType>
        class TCalcOpsAux
            : public BaseType
        {
        private:
            const ui64 TextFormsMaxLength = 8;
            const ui32 MaxFormsCount = 64;

            template<typename Aggregator>
            float CalcTextFormsFactor(Aggregator& aggregator) const {
                const size_t last = Min(TextFormsMaxLength, this->Size());
                float answer = 0.0f;
                for (size_t blockId : xrange(last)) {
                    ui32 formCount = Min(PopCount(TBase::Values[blockId]), MaxFormsCount);
                    float fForms = (float)formCount / MaxFormsCount;
                    if (formCount) {
                        answer = aggregator.UpdateAnswer(answer, fForms);
                    }
                }
                answer = aggregator.PostProcess(answer);
                return answer;
            }
        public:
            using TBase = BaseType;

            float CalcTextAvgForms() const {
                TTextFormsAvg agg(TBase::Size());
                return CalcTextFormsFactor(agg);
            }
            float CalcTextMaxForms() const {
                TTextFormsMax agg;
                return CalcTextFormsFactor(agg);
            }
            float CalcTextWeightedAvgForms(const TPoolPodHolder<float>& idf) const {
                TTextFormsWeightedAvg agg(idf);
                return CalcTextFormsFactor(agg);
            }
        };
    } // NPrivate

   using TAccumulatorsByFloatValue = NPrivate::TCalcOps<TFloatCollection>;
   using TAccumulatorsByFloatRef = NPrivate::TCalcOps<TFloatRefCollection>;
   using TAccumulatorsByUintValue = NPrivate::TCalcOpsAux<TUint64Collection>;
   using TAccumulatorsByUintRef = NPrivate::TCalcOpsAux<TUint64RefCollection>;
} // NCore
} // NTextMachine
