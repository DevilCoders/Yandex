#pragma once

#include "nfa.h"
#include "dfa.h"
#include "debug.h"
#include "util.h"
#include "types.h"

#include <util/generic/stack.h>
#include <util/generic/algorithm.h>
#include <util/generic/ylimits.h>
#include <util/stream/printf.h>

#define DBGOS GetDebugOutCONVERTER_STORE()
#define DBGOC GetDebugOutCONVERTER_CLOSURE()
#define DBGOA GetDebugOutCONVERTER_ADD()
#define DBGOF GetDebugOutCONVERTER_FIND()
#define DBGOV GetDebugOutCONVERTER_CONVERT()
#define DBGON GetDebugOutCONVERTER_NI()

namespace NRemorph {
namespace NPrivate {

class TInternalError: public yexception {
public:
    TInternalError() {
        *this << "Internal error";
    }
    TInternalError(const char* msg) {
        *this << "Internal error: " << msg;
    }
};

class TNFAStateSet;
typedef TIntrusivePtr<TNFAStateSet> TNFAStateSetPtr;

struct TNFASSTransition {
    TLiteral Lit;
    const TNFAStateSet* To;
    TTagIdToCommands Commands;
    TNFASSTransition(TLiteral l, const TNFAStateSet* t, const TTagIdToCommands& commands)
        : Lit(l)
        , To(t)
        , Commands(commands) {
    }
};

typedef TVector<TNFASSTransition> TVectorNFASSTransitions;

struct TStateData {
    size_t Priority;
    TTagIdToIndex TagIdToIndex;

    TStateData()
        : Priority(0)
    {
    }
    TStateData(size_t priority, const TTagIdToIndex& titi)
        : Priority(priority)
        , TagIdToIndex(titi)
    {
    }
    bool operator==(const TStateData& r) const {
        return EqualKeys(TagIdToIndex, r.TagIdToIndex);
    }
    bool operator!=(const TStateData& r) const {
        return !operator==(r);
    }
};

class TNFAStateSet: public TSimpleRefCount<TNFAStateSet> {
public:
    typedef NSorted::TSimpleMap<TStateId, TStateData> TStatesData;
    typedef NSorted::TSimpleMap<TTagId, std::pair<TTagIndex, TTagIndex>> TUsedIndexes;
    typedef ui32 THash;

    THash Hash;
    TUsedIndexes UsedIndexes;
    const TNFA* Nfa;
    TStatesData StatesData;
    TStateId Id;
    TVectorFinalCommands Finals;
    TVectorNFASSTransitions Transitions;
    TSetLiterals Literals;
    size_t PathLength; // Minimal number of literals to achieve this state from the start state

public:
    TNFAStateSet(const TNFA& nfa)
        : Hash(0)
        , Nfa(&nfa)
        , Id(0)
        , PathLength(0)
    {
    }

    bool operator==(const TNFAStateSet& r) const {
        return GetHash() == r.GetHash() && EqualKeys(UsedIndexes, r.UsedIndexes)
            && StatesData.size() == r.StatesData.size() && Equal(StatesData.begin(), StatesData.end(), r.StatesData.begin());
    }

    void Clear() {
        Hash = 0;
        UsedIndexes.clear();
        StatesData.clear();
        Id = 0;
        Finals.clear();
        Transitions.clear();
        Literals.clear();
    }
    bool Insert(TStateId id) {
        return Insert(id, TTagIdToIndex());
    }
    bool Insert(TStateId id, const TTagIdToIndex& tii) {
        return Insert(id, 0, tii);
    }
    bool Insert(TStateId id, size_t priority) {
        return Insert(id, priority, TTagIdToIndex());
    }
    bool Insert(TStateId id, size_t priority, const TTagIdToIndex& tii) {
        TStatesData::iterator iState = StatesData.Find(id);
        if (StatesData.end() == iState) {
            const TStateData& state = StatesData.Insert(TStatesData::value_type(id, TStateData(priority, tii)))->second;
            Hash = CombineHashes((THash)id, Hash);
            TLiteral l = Nfa->GetState(id)->Literal;
            if (!l.IsNone()) {
                Literals.insert(l);
            }
            for (TTagIdToIndex::const_iterator iTag = state.TagIdToIndex.begin(); iTag != state.TagIdToIndex.end(); ++iTag) {
                UpdateUsedIndexes(iTag->first, iTag->second);
            }
            return true;
        } else {
            TStateData& state = iState->second;
            if (priority < state.Priority) {
                state.Priority = priority;
                state.TagIdToIndex = tii;
                UpdateUsedIndexes();
                return true;
            }
        }
        return false;
    }
    void UpdateUsedIndexes(TTagId tagId, TTagIndex index) {
        TUsedIndexes::iterator iVal = UsedIndexes.Find(tagId);
        if (UsedIndexes.end() != iVal) {
            iVal->second.first = Min(iVal->second.first, index);
            iVal->second.second = Max(iVal->second.second, index);
        } else {
            UsedIndexes[tagId] = std::make_pair(index, index);
        }
    }
    void UpdateUsedIndexes() {
        UsedIndexes.clear();
        for (TStatesData::const_iterator iState = StatesData.begin(); iState != StatesData.end(); ++iState) {
            const TTagIdToIndex& tags = iState->second.TagIdToIndex;
            for (TTagIdToIndex::const_iterator iTag = tags.begin(); iTag != tags.end(); ++iTag) {
                UpdateUsedIndexes(iTag->first, iTag->second);
            }
        }
    }
    TTagIndex GetNextIndex(TTagId tagId) const {
        TUsedIndexes::const_iterator iVal = UsedIndexes.Find(tagId);
        return UsedIndexes.end() != iVal ? iVal->second.second + 1 : 0;
    }
    const TUsedIndexes& GetUsedIndexes() const {
        return UsedIndexes;
    }
    size_t GetHash() const {
        return Hash;
    }

    TNFAStateSet& CalcEClosure(TTagIdToIndex& newIndexes) {
        TStack<TStateId, TVector<TStateId>> stack;

        for (TStatesData::const_iterator iState = StatesData.begin(); iState != StatesData.end(); ++iState) {
            stack.push(iState->first);
        }

        while (!stack.empty()) {
            NWRED(DBGOC << *this << Endl << Endl);
            TStateId sId = stack.top();
            stack.pop();
            const TNFAState* s = Nfa->GetState(sId);
            for (size_t i = 0; i < s->Transitions.size(); ++i) {
                const TNFATransition& tr = *s->Transitions[i];
                if (tr.IsEpsilen()) {
                    const TTagIdToIndex& tti = StatesData[sId].TagIdToIndex;
                    if (tr.Tagged) {
                        TTagIdToIndex tmp(tti);
                        TTagIndex nextIndex = GetNextIndex(tr.TagId);
                        tmp[tr.TagId] = nextIndex;
                        if (Insert(tr.To->Id, tr.Priority, tmp))
                            newIndexes[tr.TagId] = nextIndex;
                    } else {
                        Insert(tr.To->Id, tr.Priority, tti);
                    }
                    stack.push(tr.To->Id);
                }
            }
        }
        return *this;
    }

    void CalcShiftsAndAdjust(TTagIdToIndex& shifts) {
        for (TUsedIndexes::iterator iVal = UsedIndexes.begin(); iVal != UsedIndexes.end(); ++iVal) {
            std::pair<TTagIndex, TTagIndex>& curValue = iVal->second;
            if (curValue.first) {
                shifts[iVal->first] = curValue.first;
                curValue = std::make_pair(0, curValue.second - curValue.first);
            }
        }

        if (!shifts.empty()) {
            for (TStatesData::iterator iState = StatesData.begin(); iState != StatesData.end(); ++iState) {
                TTagIdToIndex& ndx = iState->second.TagIdToIndex;
                for (TTagIdToIndex::const_iterator iShift = shifts.begin(); iShift != shifts.end(); ++iShift) {
                    TTagIdToIndex::iterator iStateIndex = ndx.Find(iShift->first);
                    if (ndx.end() != iStateIndex)
                        iStateIndex->second -= iShift->second;
                }
            }
        }
    }

    TNFAStateSetPtr CalcReach(const NRemorph::TLiteral& l) const {
        TNFAStateSetPtr result(new TNFAStateSet(*Nfa));
        result->PathLength = l.IsOrdinal() || l.IsAny() ? PathLength + 1 : PathLength;
        for (TStatesData::const_iterator iState = StatesData.begin(); iState != StatesData.end(); ++iState) {
            const TNFAState* s = Nfa->GetState(iState->first);
            for (size_t t = 0; t < s->Transitions.size(); ++t) {
                const TNFATransition& tr = *s->Transitions[t];
                if (tr.IsSymbol() && (tr.Lit == l || (tr.Lit.IsAny() && !l.IsZeroShift()))) {
                    result->Insert(tr.To->Id, tr.Priority, iState->second.TagIdToIndex);
                }
            }
        }
        return result;
    }

    void Print(IOutputStream& out) const {
        out << "[";
        Printf(out, "%" PRIX32, Hash);
        out << "]" << Endl;
        out << "{";
        for (TStatesData::const_iterator iState = StatesData.begin(); iState != StatesData.end(); ++iState) {
            if (iState != StatesData.begin())
                out << ",";
            out << "(" << iState->first << "," << iState->second.Priority << "," << iState->second.TagIdToIndex << ")"  << Endl;
        }
        out << "} " << GetUsedIndexes();
    }
};

} // NPrivate
} // NRemorph

template<>
inline void Out<NRemorph::NPrivate::TNFAStateSet>(IOutputStream& out, const NRemorph::NPrivate::TNFAStateSet& ss) {
    ss.Print(out);
}

template<>
inline void Out<NRemorph::NPrivate::TNFAStateSet::TUsedIndexes>(IOutputStream& out, const NRemorph::NPrivate::TNFAStateSet::TUsedIndexes& tii) {
    out << "{";
    bool first = true;
    for (NRemorph::NPrivate::TNFAStateSet::TUsedIndexes::const_iterator iTag = tii.begin(); iTag != tii.end(); ++iTag) {
        if (first) {
            first = false;
        } else {
            out << ",";
        }
        out << iTag->first << "->" << "[" << iTag->second.first << "," << iTag->second.second << "]";
    }
    out << "}";
}

template<>
struct THash<NRemorph::NPrivate::TNFAStateSet*> {
    inline size_t operator()(const NRemorph::NPrivate::TNFAStateSet* s) const {
        return s->GetHash();
    }
};

template<>
struct TEqualTo<NRemorph::NPrivate::TNFAStateSet*> {
    inline bool operator()(NRemorph::NPrivate::TNFAStateSet* l, NRemorph::NPrivate::TNFAStateSet* r) const {
        return *l == *r;
    }
};

namespace NRemorph {
namespace NPrivate {

typedef THashSet<TNFAStateSet*> TSetNFASS;
typedef TVector<TNFAStateSetPtr> TVectorNFASS;

class TDFABuilder {
private:
    TDFAPtr Dfa;
    ui32 StateOffset;
    ui32 TransitionOffset;
    ui32 FinalOffset;
    ui32 CommandOffset;

private:
    TDFABuilder(ui32 states, ui32 trans, ui32 finals, ui32 cmds)
        : Dfa(new TDFA())
        , StateOffset(0)
        , TransitionOffset(0)
        , FinalOffset(0)
        , CommandOffset(0)
    {
        Dfa->States.Reset(states);
        Dfa->Transitions.Reset(trans);
        Dfa->FinalCommands.Reset(finals);
        Dfa->Commands.Reset(cmds);
    }

    ui32 AddCommands(const TTagIdToCommands& cmds) {
        ui32 res = CommandOffset;
        if (!cmds.empty()) {
            Y_ASSERT(CommandOffset + cmds.size() <= Dfa->Commands.Size());
            ::Copy(cmds.begin(), cmds.end(), Dfa->Commands.Get() + CommandOffset);
            CommandOffset += cmds.size();
        }
        return res;
    }

    ui32 AddTransitions(const TVectorNFASSTransitions& trs) {
        ui32 res = TransitionOffset;
        Y_ASSERT(TransitionOffset + trs.size() <= Dfa->Transitions.Size());
        for (TVectorNFASSTransitions::const_iterator iTr = trs.begin(); iTr != trs.end(); ++iTr, ++TransitionOffset) {
            TDFATransition& tr = Dfa->Transitions[TransitionOffset];
            tr.Lit = iTr->Lit;
            tr.To = iTr->To->Id;
            tr.CommandOffset = AddCommands(iTr->Commands);
            tr.CommandCount = iTr->Commands.size();
        }
        return res;
    }

    ui32 AddFinals(const TVectorFinalCommands& fins) {
        ui32 res = FinalOffset;
        Y_ASSERT(FinalOffset + fins.size() <= Dfa->FinalCommands.Size());
        for (TVectorFinalCommands::const_iterator iFin = fins.begin(); iFin != fins.end(); ++iFin, ++FinalOffset) {
            TFinalCommands& fin = Dfa->FinalCommands[FinalOffset];
            fin.Rule = iFin->first;
            fin.CommandOffset = AddCommands(iFin->second);
            fin.CommandCount = iFin->second.size();
        }
        return res;
    }

    void AddStates(const TVectorNFASS& states) {
        Y_ASSERT(StateOffset + states.size() <= Dfa->States.Size());
        for (TVectorNFASS::const_iterator i = states.begin(); i != states.end(); ++i, ++StateOffset) {
            TDFAState& st = Dfa->States[StateOffset];
            st.TransitionOffset = AddTransitions(i->Get()->Transitions);
            st.TransitionCount = i->Get()->Transitions.size();
            st.FinalOffset = AddFinals(i->Get()->Finals);
            st.FinalCount = i->Get()->Finals.size();
        }
    }

    TDFAPtr GetResult() const {
        Y_ASSERT(StateOffset == Dfa->States.Size());
        Y_ASSERT(CommandOffset == Dfa->Commands.Size());
        Y_ASSERT(TransitionOffset == Dfa->Transitions.Size());
        Y_ASSERT(FinalOffset == Dfa->FinalCommands.Size());
        return Dfa;
    }

public:
    static TDFAPtr Build(const TTagIdToCommands& initCmds, const TVectorNFASS& states,
        size_t totalTransitions, size_t totalFinals, size_t totalCommands) {
        if (states.size() > Max<ui32>()
            || totalTransitions > Max<ui32>()
            || totalFinals > Max<ui32>()
            || totalCommands > Max<ui32>())
            throw TInternalError("too many objects");

        TDFABuilder builder((ui32)states.size(), (ui32)totalTransitions, (ui32)totalFinals, (ui32)totalCommands);
        builder.AddCommands(initCmds);
        builder.AddStates(states);

        TDFAPtr res = builder.GetResult();
        res->InitCommandCount = initCmds.size();
        return res;
    }
};

template <class TLiteralTable>
struct LessByLiteralPriority {
    const TLiteralTable& LT;
    LessByLiteralPriority(const TLiteralTable& lt)
        : LT(lt)
    {
    }
    bool operator()(const TDFATransition& l, const TDFATransition& r) const {
        return LT.GetPriority(l.Lit) < LT.GetPriority(r.Lit);
    }
};

class Converter {
private:
    const TNFA& Nfa;
    TVectorNFASS Marked;
    TVectorNFASS Unmarked;
    TSetNFASS States;
    TTagIdToIndex TagIdToNumValues;
    size_t MinimalPathLength; // Minimal number of literals to achieve some final state in DFA from the start

private:
    TTagIndex GetNumValues(TTagId id) const {
        TTagIdToIndex::const_iterator i = TagIdToNumValues.Find(id);
        return TagIdToNumValues.end() != i ? i->second : 0;
    }

    void SetNumValues(TTagId id, TTagIndex numValues) {
        TagIdToNumValues[id] = numValues;
    }

    TNFAStateSet* Find(TTagIdToIndex& shifts, TNFAStateSet& s) {
        NWRED(DBGOF << "looking for: "  << Endl << s << Endl);
        TSetNFASS::iterator it = States.find(&s);
        if (it == States.end()) {
            NWRED(DBGOF << "not found" << Endl);
            return nullptr;
        }
        TNFAStateSet* found = *it;
        NWRED(DBGOF << "found: " << "set " << found->Id << ":" << Endl << *found << Endl);
        const TNFAStateSet::TUsedIndexes& smui = s.GetUsedIndexes();
        const TNFAStateSet::TUsedIndexes& fmui = found->GetUsedIndexes();

        for (TNFAStateSet::TUsedIndexes::const_iterator iS = smui.begin(); iS != smui.end(); ++iS) {
            TNFAStateSet::TUsedIndexes::const_iterator iF = fmui.Find(iS->first);
            if (fmui.end() == iF)
                throw TInternalError();

            if (iS->second.second >= iF->second.second) {
                TTagIndex shift = iS->second.second - iF->second.second;
                if (shift) {
                    shifts[iS->first] = shift;
                }
            } else {
                TString sm = "<unknown>";
                TSubmatchIdToSourcePos::const_iterator i = Nfa.SubmatchIdToSourcePos.find(iS->first / 2);
                if (i != Nfa.SubmatchIdToSourcePos.end())
                    sm = i->second->ToString();
                throw TInternalError("The error occurred when processing ") << sm << " sub-match."
                    << " Possibly, some rules have a sequence, which ambiguously can appear inside/outside of this sub-match";
            }
        }

        NWRED(DBGOF << "shifts: " << shifts << Endl);
        found->PathLength = Min(found->PathLength, s.PathLength);
        return found;
    }

    TNFAStateSet* Add(const TNFAStateSetPtr& sp) {
        Unmarked.push_back(sp);
        for (size_t fi = 0; fi < Nfa.FinalStates.size(); ++fi) {
            if (sp->StatesData.end() != sp->StatesData.Find(Nfa.FinalStates[fi]->Id)) {
                sp->Finals.emplace_back();
                sp->Finals.back().first = fi;
            }
        }
        TNFAStateSet* retval = *States.insert(sp.Get()).first;
        NWRED(DBGOA << "add: " << *retval << Endl);
        Y_ASSERT(sp.Get() == retval);
        return retval;
    }

    void AddStorePositionCommands(TTagIdToCommands& commands, const TTagIdToIndex& newIndexes, const TTagIdToIndex& shifts) {
        for (TTagIdToIndex::const_iterator iTag = newIndexes.begin(); iTag != newIndexes.end(); ++iTag) {
            TTagIndex index = iTag->second;
            TTagIdToIndex::const_iterator iShift = shifts.Find(iTag->first);
            if (shifts.end() != iShift) {
                if (index <= iShift->second)
                    throw TInternalError();
                index -= iShift->second;
            }
            commands[iTag->first].AddCommand(TCommands::StorePosition, index);
            SetNumValues(iTag->first, Max(GetNumValues(iTag->first), index + 1));
            NWRED(DBGOS << iTag->first << "->" << GetNumValues(iTag->first) << Endl);
        }
    }

    void AddShiftCommands(TTagIdToCommands& commands, const TTagIdToIndex& shifts) {
        for (TTagIdToIndex::const_iterator iShift = shifts.begin(); iShift != shifts.end(); ++iShift) {
            commands[iShift->first].AddCommand(TCommands::Shift, iShift->second);
        }
    }

    void AddStoreTagValueCommands(TNFAStateSet& s) {
        if (!s.Finals.empty()) {
            for (TVectorFinalCommands::iterator iFinal = s.Finals.begin(); iFinal != s.Finals.end(); ++iFinal) {
                const TRuleId ruleId = iFinal->first;
                TTagIdToCommands& commands = iFinal->second;
                const TTagIdToIndex& titi = s.StatesData[Nfa.FinalStates[ruleId]->Id].TagIdToIndex;
                for (TTagIdToIndex::const_iterator iTag = titi.begin(); iTag != titi.end(); ++iTag) {
                    commands[iTag->first].AddCommand(TCommands::StoreTag, iTag->second);
                }
            }
        }
    }

    inline void UpdateMinimalPathLength(const TNFAStateSet& s) {
        if (!s.Finals.empty()) {
            MinimalPathLength = Min(MinimalPathLength, s.PathLength);
        }
    }

    template <class TLiteralTable>
    void ProcessLiteral(TNFAStateSet& s, const TLiteralTable& lt, TLiteral l) {
        NWRE_UNUSED(lt);
        NWRED(DBGOV << "literal: " << ToString(lt, l) << Endl);
        TNFAStateSetPtr next = s.CalcReach(l);
        NWRED(DBGOV << "reach:" << Endl);
        NWRED(DBGOV << *next << Endl);
        TTagIdToIndex newIndexes;
        next->CalcEClosure(newIndexes);
        NWRED(DBGOV << "closure:" << Endl);
        NWRED(DBGOV << *next << Endl);
        if (next->StatesData.empty())
            return;

        NWRED(DBGOV << "new indexes: " << newIndexes << Endl);

        TTagIdToIndex shifts;
        TTagIdToCommands commands;
        TNFAStateSet* existing = Find(shifts, *next);
        if (!existing) {
            next->CalcShiftsAndAdjust(shifts);
            NWRED(DBGOV << "shifts: " << shifts << Endl);
            existing = Add(next);
        }
        UpdateMinimalPathLength(*existing);
        AddShiftCommands(commands, shifts);
        AddStorePositionCommands(commands, newIndexes, shifts);
        AddStoreTagValueCommands(*existing);
        s.Transitions.push_back(TNFASSTransition(l, existing, commands));
    }

    template <class TLiteralTable>
    void Convert(const TLiteralTable& lt) {
        TNFAStateSetPtr startSet(new TNFAStateSet(Nfa));
        startSet->Insert(Nfa.Start->Id);
        TTagIdToIndex newIndexes;
        TTagIdToIndex shifts;
        startSet->CalcEClosure(newIndexes);
        Add(startSet);
        TTagIdToCommands initCmds;
        AddStorePositionCommands(initCmds, newIndexes, shifts);

        ui32 totalTransitions = 0;
        ui32 totalFinals = 0;
        ui32 totalCommands = initCmds.size();

        while (!Unmarked.empty()) {
            TNFAStateSetPtr t = Unmarked.back();
            Unmarked.pop_back();
            t->Id = Marked.size();
            Marked.push_back(t);
            NWRED(DBGOV << "set " << t->Id << ":" << Endl);
            NWRED(DBGOV << *t << Endl);
            TSetLiterals::const_iterator l = t->Literals.begin();
            const TLiteral* any = nullptr;
            for (; l != t->Literals.end(); ++l) {
                if (l->IsAny()) {
                    any = &*l;
                } else {
                    ProcessLiteral(*t, lt, *l);
                }
            }
            if (any) {
                ProcessLiteral(*t, lt, *any);
            }
            totalTransitions += t->Transitions.size();
            totalFinals += t->Finals.size();
            for (TVectorNFASSTransitions::const_iterator iTr = t->Transitions.begin(); iTr != t->Transitions.end(); ++iTr) {
                totalCommands += iTr->Commands.size();
            }
            for (TVectorFinalCommands::const_iterator iFin = t->Finals.begin(); iFin != t->Finals.end(); ++iFin) {
                totalCommands += iFin->second.size();
            }
        }

        Result = TDFABuilder::Build(initCmds, Marked, totalTransitions, totalFinals, totalCommands);
        Result->SubmatchIdToName = Nfa.SubmatchIdToName;
        Result->SubmatchIdOffsets.Reset(Nfa.SubmatchIdOffsets.size());
        ::Copy(Nfa.SubmatchIdOffsets.begin(), Nfa.SubmatchIdOffsets.end(), Result->SubmatchIdOffsets.Get());

        Result->SetTagOffsets(Nfa.GetNumTags(), TagIdToNumValues);

        Result->MinimalPathLength = MinimalPathLength;
        // If all rules start from the '^' anchor then set the flag
        if (!startSet->Literals.empty()) {
            bool hasNonBos = false;
            for (TSetLiterals::const_iterator iLit = startSet->Literals.begin(); iLit != startSet->Literals.end(); ++iLit) {
                if (!iLit->IsBOS()) {
                    hasNonBos = true;
                    break;
                }
            }
            if (!hasNonBos)
                Result->Flags |= DFAFLG_STARTS_WITH_ANCHOR;
        }
    }

public:
    TDFAPtr Result;

public:
    template <class TLiteralTable>
    Converter(const TLiteralTable& lt, const NRemorph::TNFA& nfa)
        : Nfa(nfa)
        , MinimalPathLength(-1)
    {
        Convert(lt);
    }
};

} // NPrivate
} // NRemorph

#undef DBGOS
#undef DBGOC
#undef DBGOA
#undef DBGOF
#undef DBGOV
#undef DBGON
