#pragma once

#include "literal.h"
#include "ptrholder.h"
#include "commands.h"
#include "types.h"
#include "source_pos.h"
#include "sized_array.h"

#include <util/generic/vector.h>
#include <util/system/defaults.h>
#include <util/ysaveload.h>

namespace NRemorph {

// The order of fields is important to exclude undesirable implicit padding
// Larger fields are declared first
struct TDFATransition {
    TStateId To;
    ui32 CommandOffset;
    ui16 CommandCount;
    TLiteral Lit;
};

// Ensure we have no undesirable padding
static_assert(sizeof(TDFATransition) == 12, "expect sizeof(TDFATransition) == 12");

typedef TVector<std::pair<TRuleId, TTagIdToCommands>> TVectorFinalCommands;

// The order of fields is important to exclude undesirable implicit padding
// Larger fields are declared first
struct TFinalCommands {
    ui32 CommandOffset;
    ui16 CommandCount;
    TRuleId Rule;
};

// Ensure we have no undesirable padding
static_assert(sizeof(TFinalCommands) == 8, "expect sizeof(TFinalCommands) == 8");

// The order of fields is important to exclude undesirable implicit padding
// Larger fields are declared first
struct TDFAState {
    ui32 TransitionOffset;
    ui32 FinalOffset;
    ui16 TransitionCount;
    ui16 FinalCount;

    Y_FORCE_INLINE bool IsFinal() const {
        return FinalCount > 0;
    }
};

// Ensure we have no undesirable padding
static_assert(sizeof(TDFAState) == 12, "expect sizeof(TDFAState) == 12");

} // NRemorph

Y_DECLARE_PODTYPE(NRemorph::TDFATransition);
Y_DECLARE_PODTYPE(NRemorph::TFinalCommands);
Y_DECLARE_PODTYPE(NRemorph::TDFAState);

namespace NRemorph {

typedef THashMap<TTagId, TString> TSubmatchIdToName;
typedef THashMap<TTagId, TSourcePosPtr> TSubmatchIdToSourcePos;

namespace NPrivate {
    class TDFABuilder;
}

enum EDFAFlags {
    DFAFLG_STARTS_WITH_ANCHOR = 1,  // All rules in DFA starts from '^' anchor
};

class TDFA: public TSimpleRefCount<TDFA> {
private:
    friend class NPrivate::TDFABuilder;

    TSizedArrayHolder<TDFAState> States;
    TSizedArrayHolder<TDFATransition> Transitions;
    TSizedArrayHolder<TFinalCommands> FinalCommands;
    TSizedArrayHolder<TCommands> Commands;
    TSizedArrayHolder<ui32> TagOffsets;
    ui16 InitCommandCount;

public:
    TSizedArrayHolder<TTagId> SubmatchIdOffsets;
    TSubmatchIdToName SubmatchIdToName;
    ui32 Flags; // Precalculated DFA flags
    size_t MinimalPathLength; // Minimal sequence, which can be matched by the DFA

public:
    TDFA()
        : InitCommandCount(0)
        , Flags(0)
        , MinimalPathLength(0)
    {
    }

    void SetTagOffsets(TTagId numTags, const TTagIdToIndex& tagIdToNumValues) {
        TagOffsets.Reset(numTags + 1);
        size_t offset = 0;
        TTagId t = 0;
        for (; t < TagOffsets.Size() - 1; ++t) {
            TagOffsets[t] = offset;
            TTagIdToIndex::const_iterator iVal = tagIdToNumValues.Find(t);
            offset += (iVal != tagIdToNumValues.end() ? iVal->second : 0);
        }
        TagOffsets[t] = offset;
    }

    Y_FORCE_INLINE const ui32* GetTagOffsets() const {
        return TagOffsets.Get();
    }

    Y_FORCE_INLINE size_t GetMaxTags() const {
        return TagOffsets.Size() - 1;
    }

    Y_FORCE_INLINE size_t GetStateCount() const {
        return States.Size();
    }

    Y_FORCE_INLINE const TDFAState& GetState(TStateId state) const {
        return States[state];
    }

    Y_FORCE_INLINE size_t GetTransitionCount() const {
        return Transitions.Size();
    }

    Y_FORCE_INLINE const TDFATransition* GetTransBeg(TStateId state) const {
        Y_ASSERT(States[state].TransitionOffset <= Transitions.Size());
        return Transitions.Get() + States[state].TransitionOffset;
    }

    Y_FORCE_INLINE const TDFATransition* GetTransEnd(TStateId state) const {
        const TDFAState& st = States[state];
        Y_ASSERT(st.TransitionOffset + st.TransitionCount <= Transitions.Size());
        return Transitions.Get() + st.TransitionOffset + st.TransitionCount;
    }

    Y_FORCE_INLINE const TFinalCommands* GetFinalsBeg(TStateId state) const {
        Y_ASSERT(States[state].FinalOffset <= FinalCommands.Size());
        return FinalCommands.Get() + States[state].FinalOffset;
    }

    Y_FORCE_INLINE const TFinalCommands* GetFinalsEnd(TStateId state) const {
        const TDFAState& st = States[state];
        Y_ASSERT(st.FinalOffset + st.FinalCount <= FinalCommands.Size());
        return FinalCommands.Get() + st.FinalOffset + st.FinalCount;
    }

    Y_FORCE_INLINE size_t GetCommandCount() const {
        return Commands.Size();
    }

    Y_FORCE_INLINE const TCommands* GetInitCmdBeg() const {
        return Commands.Get();
    }

    Y_FORCE_INLINE const TCommands* GetInitCmdEnd() const {
        Y_ASSERT(InitCommandCount <= Commands.Size());
        return Commands.Get() + InitCommandCount;
    }

    Y_FORCE_INLINE const TCommands* GetCmdBeg(const TDFATransition& tr) const {
        Y_ASSERT(tr.CommandOffset <= Commands.Size());
        return Commands.Get() + tr.CommandOffset;
    }

    Y_FORCE_INLINE const TCommands* GetCmdEnd(const TDFATransition& tr) const {
        Y_ASSERT(tr.CommandOffset + tr.CommandCount <= Commands.Size());
        return Commands.Get() + tr.CommandOffset + tr.CommandCount;
    }

    Y_FORCE_INLINE const TCommands* GetCmdBeg(const TFinalCommands& fin) const {
        Y_ASSERT(fin.CommandOffset <= Commands.Size());
        return Commands.Get() + fin.CommandOffset;
    }

    Y_FORCE_INLINE const TCommands* GetCmdEnd(const TFinalCommands& fin) const {
        Y_ASSERT(fin.CommandOffset + fin.CommandCount <= Commands.Size());
        return Commands.Get() + fin.CommandOffset + fin.CommandCount;
    }

    void Save(IOutputStream* rh) const {
        States.Save(rh);
        Transitions.Save(rh);
        FinalCommands.Save(rh);
        Commands.Save(rh);
        TagOffsets.Save(rh);
        SubmatchIdOffsets.Save(rh);
        ::Save(rh, InitCommandCount);
        ::Save(rh, SubmatchIdToName);
        ::Save(rh, Flags);
        ::Save(rh, (ui32)MinimalPathLength);
    }

    void Load(IInputStream* rh) {
        States.Load(rh);
        Transitions.Load(rh);
        FinalCommands.Load(rh);
        Commands.Load(rh);
        TagOffsets.Load(rh);
        SubmatchIdOffsets.Load(rh);
        ::Load(rh, InitCommandCount);
        ::Load(rh, SubmatchIdToName);
        ::Load(rh, Flags);

        ui32 value;
        ::Load(rh, value);
        MinimalPathLength = value;
    }
};

typedef TIntrusivePtr<TDFA> TDFAPtr;

inline void PrintStateId(IOutputStream& out, size_t id, size_t num) {
    out << "\"" << num << "_DFA_" << id << "\"";
}

inline void PrintCommands(IOutputStream& out, const TCommands* begin, const TCommands* end) {
    out << "{";
    for (const TCommands* i = begin; i != end; ++i) {
        if (i != begin)
            out << ",";
        out << *i;
    }
    out << "}";
}

template <class TLiteralTable>
inline void PrintDFATransitionLabel(IOutputStream& out, const TLiteralTable& lt, const TDFA& dfa, const TDFATransition& t) {
    out << " [label=\"" << ToString(lt, t.Lit) << "/\\n";
    PrintCommands(out, dfa.GetCmdBeg(t), dfa.GetCmdEnd(t));
    out << "\"]";
}

template <class TLiteralTable>
inline void PrintDFA(IOutputStream& out, const TLiteralTable& lt, const TDFA& dfa, size_t num) {
    out << "subgraph cluster" << num << "_DFA {" << Endl;
    out << "labeljust=l" << Endl;
    out << "label=\"" << num << "_DFA\";" << Endl;
    for (size_t i = 0; i < dfa.GetStateCount(); ++i) {
        const TDFAState& s = dfa.GetState(i);
        out << "\t"; PrintStateId(out, i, num); out << " [label=\"" << i;
        if (i == 0 || s.IsFinal()) {
            out << "(";
            if (i == 0) {
                out << "S";
                PrintCommands(out, dfa.GetInitCmdBeg(), dfa.GetInitCmdEnd());
                out << " ";
            }
            if (s.IsFinal()) {
                const TFinalCommands* begin = dfa.GetFinalsBeg(i);
                const TFinalCommands* end = dfa.GetFinalsEnd(i);
                for (const TFinalCommands* f = begin; f != end; ++f) {
                    out << "F";
                    PrintCommands(out, dfa.GetCmdBeg(*f), dfa.GetCmdEnd(*f));
                    out << " ";
                }
            }
            out << ")";
        }
        out << "\"];" << Endl;
        const TDFATransition* begin = dfa.GetTransBeg(i);
        const TDFATransition* end = dfa.GetTransEnd(i);
        for (const TDFATransition* t = begin; t != end; ++t) {
            out << "\t"; PrintStateId(out, i, num); out << " -> "; PrintStateId(out, t->To, num);
            PrintDFATransitionLabel(out, lt, dfa, *t);
            out << ";" << Endl;
        }
    }
    out << "}" << Endl;
}

template <class TLiteralTable>
inline void PrintAsDot(IOutputStream& out, const TLiteralTable& lt, const NRemorph::TDFA& dfa) {
    out << "digraph G {" << Endl;
    out << "rankdir=LR;" << Endl;
    out << "node [shape=circle];" << Endl;
    PrintDFA(out, lt, dfa, 0);
    out << "}" << Endl;
}

template <class TLiteralTable>
inline void PrintAsDotSubgraph(IOutputStream& out, const TLiteralTable& lt, const NRemorph::TDFA& dfa, size_t num) {
    PrintDFA(out, lt, dfa, num);
}

inline void PrintStats(IOutputStream& out, const TDFA& dfa) {
    out << "states: " << dfa.GetStateCount() << ", sizeof=" << sizeof(TDFAState) << Endl;
    out << "transitions: " << dfa.GetTransitionCount() << ", sizeof=" << sizeof(TDFATransition) << Endl;
    out << "commands: " << dfa.GetCommandCount() << ", sizeof=" << sizeof(TCommands) << Endl;
}

} // NRemorph
