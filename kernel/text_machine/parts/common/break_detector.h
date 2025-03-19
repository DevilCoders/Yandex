#pragma once

#include <kernel/text_machine/interface/hit.h>

namespace NTextMachine {
namespace NCore {
    struct TFullBreakId {
        EBaseIndexType IndexType;
        TBreakId BreakNum;

        TFullBreakId() = default;
        TFullBreakId(
            EBaseIndexType indexType,
            TBreakId breakNum)
            : IndexType(indexType)
            , BreakNum(breakNum)
        {}
        explicit TFullBreakId(const TAnnotation& ann) {
            IndexType = ann.StreamRef().IndexType;
            BreakNum = ann.BreakNumber;
        }

        bool operator == (const TFullBreakId& x) const {
            return x.IndexType == IndexType
                && x.BreakNum == BreakNum;
        }

        static TFullBreakId Invalid() {
            return {TBaseIndex::BaseIndexMax, TAbsentValue::BreakId};
        }
    };

    class TBreakDetector {
        enum class EState {
            NoBreak,
            FirstBreak,
            NextBreak,
            InBreak
        };

        EState State = EState::NoBreak;
        TFullBreakId PrevId = TFullBreakId::Invalid();

    public:
        void Reset() {
            new (this) TBreakDetector;
        }

        void Add(const TFullBreakId& id) {
            switch (State) {
                case EState::NoBreak: {
                    State = EState::FirstBreak;
                    PrevId = id;
                    break;
                }
                case EState::FirstBreak:
                case EState::InBreak:
                case EState::NextBreak: {
                    if (id == PrevId) {
                        State = EState::InBreak;
                        break;
                    }
                    State = EState::NextBreak;
                    PrevId = id;
                    break;
                }
            }
        }

        void Add(const TAnnotation& ann) {
            Add(TFullBreakId{ann});
        }
        void Add(const TPosition& pos) {
            Add(pos.AnnotationRef());
        }

        bool IsInBreak() const {
            return EState::NoBreak != State;
        }
        bool IsFinishBreak() const {
            return EState::NextBreak == State;
        }
        bool IsStartBreak() const {
            return EState::NextBreak == State || EState::FirstBreak == State;
        }
    };
} // NCore
} // NTextMachine
