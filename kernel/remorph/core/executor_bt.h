#pragma once

#include "executor.h"
#include "nfa.h"
#include "literal.h"
#include "types.h"

#include <util/generic/stack.h>

#define DBGO GetDebugOutEXECUTE_NFA()

namespace NRemorph {

namespace NPrivate {

template <class TSymbolIterator>
struct TExecutorState: public TSimpleRefCount<TExecutorState<TSymbolIterator>> {
    TSymbolIterator CurInput;
    const TNFAState* State;
    size_t Transition;
    TExecutorState(TSymbolIterator curInput, const TNFAState* state): CurInput(curInput), State(state), Transition(0) {}
};

template <class TSymbolIterator>
struct TExecutorStack: public TStack<TExecutorState<TSymbolIterator>> {
    typedef TStack<TExecutorState<TSymbolIterator>> TBase;
    template <class TLiteralTable>
    void DbgPrint(const TLiteralTable& lt) {
        NWRE_UNUSED(lt);
        for (size_t i = 0; i < TBase::c.size(); ++i) {
            NWRED(DBGO << TBase::c[i].State->Id);
            if (i < TBase::c.size() - 1) {
                const TNFATransition& t = *TBase::c[i].State->Transitions[TBase::c[i].Transition - 1];
                NWRED(DBGO << " -");
                if (t.IsSymbol()) {
                    NWRED(DBGO << "[" << ToString(lt, t.Lit) << "]");
                } else if (t.Tagged) {
                    NWRED(DBGO << "(" << t.TagId << ")");
                }
                NWRED(DBGO << "-> ");
            }
        }
        NWRED(DBGO << Endl);
    }
    void FillSubmatches(TMatchInfo& r, const TNFA& nfa, TStateId matchedId) const {
        TTagId submatchIdOffset = nfa.SubmatchIdOffsets[matchedId];
        r.Submatches.resize(nfa.SubmatchIdOffsets[matchedId + 1] - submatchIdOffset);
        TVectorSubmatches& v = r.Submatches;
        for (size_t i = 1; i < TBase::c.size(); ++i) {
            const TExecutorState<TSymbolIterator>& state = TBase::c[i - 1];
            const TNFATransition& t = *state.State->Transitions[state.Transition - 1];
            if (t.Tagged) {
                TTagId submatchId = t.TagId / 2 - submatchIdOffset;
                if (t.TagId % 2 == 0) {
                    v[submatchId].first = state.CurInput.GetCurPos();
                } else {
                    v[submatchId].second = state.CurInput.GetCurPos();
                }
            }
        }
        for (size_t i = 0; i < v.size(); ++i) {
            if (v[i].first == v[i].second) {
                v[i].first = -1;
                v[i].second = -1;
            } else {
                TSubmatchIdToName::const_iterator it = nfa.SubmatchIdToName.find(i + submatchIdOffset);
                if (it != nfa.SubmatchIdToName.end())
                    r.NamedSubmatches.insert(std::make_pair(it->second, v[i]));
            }
        }
    }
};

} // NPrivate

template <class TLiteralTable, class TSymbolIterator>
inline TMatchInfoPtr Match(const TLiteralTable& lt, const TSymbolIterator& begin, const TNFA& nfa) {
    TMatchInfoPtr result;
    NPrivate::TExecutorStack<TSymbolIterator> stack;
    stack.push(NPrivate::TExecutorState<TSymbolIterator>(begin, nfa.Start));
    while (!stack.empty()) {
        NPrivate::TExecutorState<TSymbolIterator>& state = stack.top();
        if (state.CurInput.AtEnd()) {
            for (size_t i = 0; i < nfa.FinalStates.size(); ++i) {
                if (state.State == nfa.FinalStates[i]) {
                    result = TMatchInfoPtr(new TMatchInfo());
                    result->MatchedId = i;
                    stack.FillSubmatches(*result, nfa, i);
                    stack.DbgPrint(lt);
                    return result;
                }
            }
            // find next epsilon transition
            for (; state.Transition < state.State->Transitions.size(); ++state.Transition) {
                const TNFATransition& t = *state.State->Transitions[state.Transition];
                if (t.IsEpsilen()) {
                    ++state.Transition;
                    stack.push(NPrivate::TExecutorState<TSymbolIterator>(state.CurInput, t.To));
                    goto cont;
                }
            }
            // no epsilon transitions from here left - backtrack
            stack.DbgPrint(lt);
            stack.pop();
        } else {
            for (; state.Transition < state.State->Transitions.size(); ++state.Transition) {
                const TNFATransition& t = *state.State->Transitions[state.Transition];
                if (t.IsSymbol()) {
                    if (Compare(lt, t.Lit, state.CurInput)) {
                        const size_t cnt = NPrivate::GetNextCount(t.Lit, state.CurInput);
                        ++state.Transition;
                        for (size_t nit = 0; nit < cnt; ++nit) {
                            stack.push(NPrivate::TExecutorState<TSymbolIterator>(NPrivate::GetNextIter(t.Lit, state.CurInput, nit), t.To));
                        }
                        goto cont;
                    }
                } else {
                    ++state.Transition;
                    stack.push(NPrivate::TExecutorState<TSymbolIterator>(state.CurInput, t.To));
                    goto cont;
                }
            }
            stack.DbgPrint(lt);
            stack.pop();
        }
      cont:
        ;
    }
    return result;
}

} // NRemorph

#undef DBGO
