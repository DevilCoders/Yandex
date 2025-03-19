#pragma once

#include "literal.h"
#include "ptrholder.h"
#include "types.h"
#include "source_pos.h"

#include <util/generic/ptr.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/algorithm.h>

namespace NRemorph {

struct TNFAState;

class TNFATransition: public TSimpleRefCount<TNFATransition> {
    friend struct TNFA;
private:
    TNFATransition()
        : To(nullptr)
        , Tagged(false)
        , TagId(0)
        , Priority(0) {
    }
    TNFATransition(TLiteral l)
        : To(nullptr)
        , Lit(l)
        , Tagged(false)
        , TagId(0)
        , Priority(0) {
    }
    TNFATransition(TTagId submatchId)
        : To(nullptr)
        , Tagged(true)
        , TagId(submatchId)
        , Priority(0) {
    }
    /// To-state is empty after copy.
    TNFATransition(const TNFATransition& origin)
        : TSimpleRefCount<TNFATransition>()
        , To(nullptr)
        , Lit(origin.Lit)
        , Tagged(origin.Tagged)
        , TagId(origin.TagId)
        , Priority(origin.Priority) {
    }
public:
    TNFAState* To;
    TLiteral Lit;
    bool Tagged;
    TTagId TagId;
    size_t Priority;
public:
    bool IsSymbol() const {
        return !Lit.IsNone();
    }
    bool IsEpsilen() const {
        return Lit.IsNone();
    }
};

typedef TIntrusivePtr<TNFATransition> TNFATransitionPtr;
typedef TVector<TNFATransitionPtr> TVectorTransitions;

struct TNFAState: public TSimpleRefCount<TNFAState> {
    friend struct TNFA;
private:
    TNFAState()
        : Id(0) {
    }
    /// ID is unset and transitions are empty after copy.
    TNFAState(const TNFAState& origin)
        : TSimpleRefCount<TNFAState>()
        , Id(0)
        , Literal(origin.Literal) {
    }
public:
    TStateId Id;
    TVectorTransitions Transitions;
    TLiteral Literal;
};

typedef TIntrusivePtr<TNFAState> TNFAStatePtr;
typedef THashSet<TLiteral> TSetLiterals;
typedef THashMap<TTagId, TString> TSubmatchIdToName;
typedef THashMap<TTagId, TSourcePosPtr> TSubmatchIdToSourcePos;
typedef TVector<TNFAState*> TVectorNFAStates;
typedef TPtrHolder<TNFAState, TStateId> TStateHolder;
typedef THashMap<const TNFAState*, TNFAState*> TStateMap;

struct TNFA: public TSimpleRefCount<TNFA> {
    TStateHolder StateHolder;
    TNFAState* Start;
    TVectorNFAStates FinalStates;
    TTagId NumSubmatches;
    TSubmatchIdToName SubmatchIdToName;
    TSubmatchIdToSourcePos SubmatchIdToSourcePos;
    TVector<TTagId> SubmatchIdOffsets;
    TVector<TNFATransition*> TaggedTransitions;

    TNFA()
        : Start(nullptr)
        , NumSubmatches(0) {
    }
    const TNFAState* GetState(TStateId id) const {
        return StateHolder.Get(id);
    }
    TTagId GetNumTags() const {
        return NumSubmatches * 2;
    }
    size_t GetNumStates() const {
        return StateHolder.Storage.size();
    }
    bool IsFinal(const TNFAState* s) const {
        return IsIn(FinalStates, s);
    }
    TNFATransition* CreateTransition(TNFAState& from) {
        TNFATransitionPtr tr(new TNFATransition());
        from.Transitions.push_back(tr);
        return tr.Get();
    }
    TNFATransition* CreateTransition(TNFAState& from, TLiteral l) {
        TNFATransitionPtr tr(new TNFATransition(l));
        from.Transitions.push_back(tr.Get());
        from.Literal = l;
        return tr.Get();
    }
    TNFATransition* CreateTransition(TNFAState& from, TTagId submatchId) {
        TNFATransitionPtr tr(new TNFATransition(submatchId));
        TaggedTransitions.push_back(tr.Get());
        from.Transitions.push_back(tr.Get());
        return tr.Get();
    }
    TNFAState* CreateState() {
        return StateHolder.Store(new TNFAState());
    }
    TNFAState* CloneState(const TNFAState* origin, const TVectorTransitions& boundaries, TVectorTransitions& newBoundaries, TStateMap& traversedStates) {
        TNFAState* state = StateHolder.Store(new TNFAState(*origin));
        traversedStates.insert(::std::make_pair(origin, state));
        for (TVectorTransitions::const_iterator iTr = origin->Transitions.begin(); iTr != origin->Transitions.end(); ++iTr) {
            const TNFATransition* originTransition = iTr->Get();
            state->Transitions.push_back(TNFATransitionPtr(new TNFATransition(*originTransition)));
            TNFATransition* transition = state->Transitions.back().Get();
            bool stop = false;
            for (TVectorTransitions::const_iterator iBoundary = boundaries.begin(); iBoundary != boundaries.end(); ++iBoundary) {
                if (iBoundary->Get() == originTransition) {
                    newBoundaries.push_back(transition);
                    stop = true;
                    break;
                }
            }
            if (!stop && originTransition->To) {
                TStateMap::const_iterator found = traversedStates.find(originTransition->To);
                transition->To = (found != traversedStates.end())
                    ? found->second
                    : CloneState(originTransition->To, boundaries, newBoundaries, traversedStates);
            }
        }
        return state;
    }
    TNFAState* CloneState(const TNFAState* origin, const TVectorTransitions& boundaries, TVectorTransitions& newBoundaries) {
        Y_ASSERT(origin);
        TStateMap traversedStates;
        return CloneState(origin, boundaries, newBoundaries, traversedStates);
    }
};

typedef TIntrusivePtr<TNFA> TNFAPtr;
typedef TVector<TNFAPtr> TVectorNFAs;

template <class TLiteralTable>
inline void Print(IOutputStream& out, const TLiteralTable& lt, const TSetLiterals& ls) {
    out << "{";
    TSetLiterals::const_iterator i = ls.begin();
    for (; i != ls.end(); ++i) {
        if (i != ls.begin())
            out << ",";
        out << ToString(lt, *i);
    }
    out << "}";
}

inline void PrintStateId(IOutputStream& out, const TNFAState* s, size_t num) {
    out << "\"" << num << "_NFA_" << s->Id << "\"";
}

template <class TLiteralTable>
inline void PrintNFATransitionLabel(IOutputStream& out, const TLiteralTable& lt, const TNFATransition* t) {
    out << " [label=\"" << t->Priority;
    if (t->IsSymbol()) {
        out << "," << lt.ToString(t->Lit);
    } else if (t->Tagged) {
        out << ",(" << t->TagId << ")";
    }
    out << "\"]";
}

template <class TLiteralTable>
inline void PrintNFA(IOutputStream& out, const TLiteralTable& lt, const TNFA& nfa, size_t num) {
    out << "subgraph cluster" << num << "_NFA {" << Endl;
    out << "labeljust=l" << Endl;
    out << "label=\"" << num << "_NFA\";" << Endl;
    for (TStateId i = 0; i < nfa.GetNumStates(); ++i) {
        const TNFAState* s = nfa.GetState(i);
        out << "\t"; PrintStateId(out, s, num); out << " [label=\"" << s->Id;
        bool final = nfa.IsFinal(s);
        if (s == nfa.Start || final) {
            out << "("
                 << (s == nfa.Start ? "S" : "")
                 << (final ? "F" : "")
                 << ")";
        }
        out << "\"];" << Endl;
        for (size_t j = 0; j < s->Transitions.size(); ++j) {
            TNFATransition* t = s->Transitions[j].Get();
            out << "\t"; PrintStateId(out, s, num); out << " -> "; PrintStateId(out, t->To, num);
            PrintNFATransitionLabel(out, lt, t);
            out << ";" << Endl;
        }
    }
    out << "}" << Endl;
}

template <class TLiteralTable>
inline void PrintAsDot(IOutputStream& out, const TLiteralTable& lt, const NRemorph::TNFA& nfa) {
    out << "digraph G {" << Endl;
    out << "rankdir=LR;" << Endl;
    out << "node [shape=circle];" << Endl;
    PrintNFA(out, lt, nfa, 0);
    out << "}" << Endl;
}

template <class TLiteralTable>
inline void PrintAsDotSubgraph(IOutputStream& out, const TLiteralTable& lt, const NRemorph::TNFA& nfa, size_t num) {
    PrintNFA(out, lt, nfa, num);
}

} // NRemorph
