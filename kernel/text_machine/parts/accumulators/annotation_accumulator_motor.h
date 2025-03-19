#pragma once

#include "annotation_accumulator_parts.h"
#include "annotation_accumulator_parts_for_neuro_text_counters.h"

#include <kernel/text_machine/module/module_def.inc>
namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(AnnotationAccumulator) {
        template <typename M>
        class TMotor : public M {
        public:
            using TModule = M;

        public:
            void NewQuery(TMemoryPool& pool, const TQuery* query, const TWeights* weights) {
                Y_ASSERT(query);
                Y_ASSERT(weights);

                TQueryInfo info{*query, *weights, query->GetNumWords()};
                TModule::NewQuery(pool, info);
            }
            void NewDoc() {
                BreakDetector.Reset();

                TDocInfo info{};
                TModule::NewDoc(info);
            }
            void AddHit(const THit& hit) {
                Y_ASSERT(hit.Position.Annotation);
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
                PrepareHitInfo(hit.Word.WordId, hit.Word.FormId,
                    hit.Position.LeftWordPos, hit.Position.RightWordPos,
                    hit.Position.Annotation->BreakNumber, info);
                TModule::AddHit(info);
                TModule::FinishHit(info);
            }
            void FinishDoc() {
                if (BreakDetector.IsInBreak()) {
                    FinishAnnotation();
                }

                TDocInfo info{};
                TModule::FinishDoc(info);
            }

        public:
            Y_FORCE_INLINE const auto& GetAnyFullMatchState() const {
                return M::template Vars<TAnyFullMatchUnit>();
            }
            Y_FORCE_INLINE const auto& GetExactFullMatchState() const {
                return M::template Vars<TExactFullMatchUnit>();
            }
            Y_FORCE_INLINE const auto& GetOriginalFullMatchState() const {
                return M::template Vars<TOriginalFullMatchUnit>();
            }

            Y_FORCE_INLINE const auto& GetAnyBclmState() const {
                return M::template Vars<TAnyBclmUnit>();
            }
            Y_FORCE_INLINE const auto& GetAnyBocmState() const {
                return M::template Vars<TAnyBocmUnit>();
            }
            Y_FORCE_INLINE const auto& GetAnyTRBclmLiteState() const {
                return M::template Vars<TAnyBclmLiteUnit>();
            }
            Y_FORCE_INLINE const auto& GetAnyTRTxtPairState() const {
                return M::template Vars<TAnyTxtPairUnit>();
            }
            Y_FORCE_INLINE const auto& GetExactTRTxtPairState() const {
                return M::template Vars<TExactTxtPairUnit>();
            }
            Y_FORCE_INLINE const auto& GetOriginalTRTxtPairState() const {
                return M::template Vars<TOriginalTxtPairUnit>();
            }
            Y_FORCE_INLINE const auto& GetAnyAnnotationMatchState() const {
                return M::template Vars<TAnyAnnotationMatchFamily>();
            }
            Y_FORCE_INLINE const auto& GetOriginalAnnotationMatchState() const {
                return M::template Vars<TOriginalAnnotationMatchFamily>();
            }
            Y_FORCE_INLINE const auto& GetExactAnnotationMatchState() const {
                return M::template Vars<TExactAnnotationMatchFamily>();
            }
            Y_FORCE_INLINE const auto& GetAnyPerWordAnnotationMatchState() const {
                return M::template Vars<TAnyPerWordAnnotationMatchUnit>();
            }
            Y_FORCE_INLINE const auto& GetAnyCMMatchState() const {
                return M::template Vars<TAnyCMMatchFamily>();
            }
            Y_FORCE_INLINE const auto& GetAnyInfixMatchState() const {
                return M::template Vars<TAnyInfixMatchFamily>();
            }
            Y_FORCE_INLINE const auto& GetExactInfixMatchState() const {
                return M::template Vars<TExactInfixMatchFamily>();
            }
            Y_FORCE_INLINE const auto& GetAnyWcmState() const {
                return M::template Vars<TAnyWcmFamily>();
            }
            Y_FORCE_INLINE const auto& GetAnyTrigramState() const {
                return M::template Vars<TAnyTrigramUnit>();
            }
            Y_FORCE_INLINE const auto& GetAnyQueryPartMatchState() const {
                return M::template Vars<TAnyQueryPartMatchFamily>();
            }
            Y_FORCE_INLINE const auto& GetNeuroTextCounterLevel1State() const {
                return M::template Vars<TNeuroTextCounterLevel1Unit>();
            }

        private:
            Y_FORCE_INLINE void StartAnnotation(float value, TBreakWordId length) {
                Y_ASSERT(value >= 0);
                Y_ASSERT(value <= 1);

                LastAnnInfo = TAnnotationInfo{value, Min<TBreakWordId>(length, MaxAnnLength)};
                TModule::StartAnnotation(LastAnnInfo);
            }
            Y_FORCE_INLINE void FinishAnnotation() {
                TModule::FinishAnnotation(LastAnnInfo);
            }
            Y_FORCE_INLINE void PrepareHitInfo(TQueryWordId wordId, TWordFormId formId,
                TBreakWordId leftPosition, TBreakWordId rightPosition, TBreakId breakId, THitInfo& info) {
                Y_ASSERT(leftPosition <= rightPosition);

                info = THitInfo{wordId, formId, leftPosition, Min<TBreakWordId>(rightPosition, MaxAnnLength - 1), breakId};
            }

        private:
            TBreakDetector BreakDetector;
            TAnnotationInfo LastAnnInfo;
        };
    } // MACHINE_PARTS(AnnotationAccumulator)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
