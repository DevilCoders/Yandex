#include "compiler.h"

#include <util/generic/stack.h>

#define DBGO GetDebugOutCOMPILER()

namespace NRemorph {

namespace NPrivate {

struct TFragment: public TSimpleRefCount<TFragment> {
    TNFAState* Start;
    TVectorTransitions Out;
    TFragment(TNFAState* start)
        : Start(start)
    {
    }
};

typedef TIntrusivePtr<TFragment> TFragmentPtr;

struct TCompiler;

struct TOp: public TSimpleRefCount<TOp> {
    virtual ~TOp() {}
    virtual TString GetTypeString() = 0;
    virtual void Exec(TCompiler& c) = 0;
};

#define WRE_CORE_STRUCT_OP(TYPE) \
struct T ## TYPE: public TOp { \
TString GetTypeString() override { return #TYPE; }

typedef TIntrusivePtr<TOp> TOpPtr;

WRE_CORE_STRUCT_OP(Compile)
    TAstNode* Node;
    TCompile(TAstNode* node)
        : Node(node)
    {
    }
    void Exec(TCompiler& c) override;
};

WRE_CORE_STRUCT_OP(CompileLiteral)
    TAstNode* Node;
    TAstLiteral* Data;
    TCompileLiteral(TAstNode* node, TAstLiteral* data)
        : Node(node)
        , Data(data)
    {
    }
    void Exec(TCompiler& c) override;
};

WRE_CORE_STRUCT_OP(CompileCatenation)
    TAstNode* Node;
    TCompileCatenation(TAstNode* node)
        : Node(node)
    {
    }
    void Exec(TCompiler& c) override;
};

WRE_CORE_STRUCT_OP(CreateCatenation)
    void Exec(TCompiler& c) override;
};

WRE_CORE_STRUCT_OP(CompileUnion)
    TAstNode* Node;
    TCompileUnion(TAstNode* node)
        : Node(node)
    {
    }
    void Exec(TCompiler& c) override;
};

WRE_CORE_STRUCT_OP(CreateUnion)
    void Exec(TCompiler& c) override;
};

WRE_CORE_STRUCT_OP(CompileIteration)
    TAstNode* Node;
    TAstIteration* Data;
    TCompileIteration(TAstNode* node, TAstIteration* data)
        : Node(node)
        , Data(data)
    {
    }
    void Exec(TCompiler& c) override;
};

WRE_CORE_STRUCT_OP(CreateIteration)
    TAstIteration* Data;
    TCreateIteration(TAstIteration* data)
        : Data(data)
    {
    }
    void Exec(TCompiler& c) override;
};

WRE_CORE_STRUCT_OP(CompileSubmatch)
    TAstNode* Node;
    TAstSubmatch* Data;
    TCompileSubmatch(TAstNode* node, TAstSubmatch* data)
        : Node(node)
        , Data(data)
    {
    }
    void Exec(TCompiler& c) override;
};

WRE_CORE_STRUCT_OP(CreateSubmatch)
    TAstSubmatch* Data;
    TCreateSubmatch(TAstSubmatch* data)
        : Data(data)
    {
    }
    void Exec(TCompiler& c) override;
};

struct TCompiler {
    typedef TStack<TFragmentPtr, TVector<TFragmentPtr>> TFragmentStack;
    typedef TStack<TOpPtr, TVector<TOpPtr>> TOpStack;
    typedef TVector<std::pair<TNFAState*, TNFATransition*>> TAnyTransitions;

    TNFAPtr Result;
    TFragmentStack FStack;
    TOpStack OStack;

    TCompiler(const TNFAPtr& nfa, TAstNode* ast);

    TFragmentPtr CreateOptional(TFragment* source, bool greedy) {
        if (!greedy)
            return CreateOptionalNonGreedy(source);
        TNFAState* s = Result->CreateState();
        TNFAState* f = Result->CreateState();
        TNFATransition* s2 = Result->CreateTransition(*s);
        TNFATransition* s1 = Result->CreateTransition(*s);
        TNFATransition* f1 = Result->CreateTransition(*f);

        s2->To = source->Start;

        ConnectTo(source->Out, f);

        TFragmentPtr nf = new TFragment(s);
        nf->Out.push_back(s1);
        nf->Out.push_back(f1);
        return nf;
    }

    TFragmentPtr CreateOptionalNonGreedy(TFragment* source) {
        TNFAState* s = Result->CreateState();
        TNFAState* f = Result->CreateState();
        TNFATransition* s1 = Result->CreateTransition(*s);
        TNFATransition* s2 = Result->CreateTransition(*s);
        TNFATransition* f1 = Result->CreateTransition(*f);

        s2->To = source->Start;

        ConnectTo(source->Out, f);

        TFragmentPtr nf = new TFragment(s);
        nf->Out.push_back(s1);
        nf->Out.push_back(f1);
        return nf;
    }

    TFragmentPtr CreateZeroOrMoreNonGreedy(TFragment* source) {
        TNFAState* s = Result->CreateState();
        TNFAState* f = Result->CreateState();
        TNFATransition* s1 = Result->CreateTransition(*s);
        TNFATransition* s2 = Result->CreateTransition(*s);
        TNFATransition* f1 = Result->CreateTransition(*f);
        TNFATransition* f2 = Result->CreateTransition(*f);

        s2->To = source->Start;
        f1->To = source->Start;

        ConnectTo(source->Out, f);

        TFragmentPtr nf = new TFragment(s);

        nf->Out.push_back(s1);
        nf->Out.push_back(f2);

        return nf;
    }

    TFragmentPtr CreateZeroOrMore(TFragment* source, bool greedy) {
        if (!greedy)
            return CreateZeroOrMoreNonGreedy(source);

        TNFAState* s = Result->CreateState();
        TNFAState* f = Result->CreateState();
        TNFATransition* s1 = Result->CreateTransition(*s);
        TNFATransition* s2 = Result->CreateTransition(*s);
        TNFATransition* f1 = Result->CreateTransition(*f);
        TNFATransition* f2 = Result->CreateTransition(*f);

        s1->To = source->Start;
        f1->To = source->Start;

        ConnectTo(source->Out, f);

        TFragmentPtr nf = new TFragment(s);

        nf->Out.push_back(s2);
        nf->Out.push_back(f2);

        return nf;
    }

    TFragmentPtr CreateOneOrMore(TFragment* source, bool) {
        TNFAState* f = Result->CreateState();
        TNFATransition* f1 = Result->CreateTransition(*f);
        TNFATransition* f2 = Result->CreateTransition(*f);

        ConnectTo(source->Out, f);

        f1->To = source->Start;

        TFragmentPtr nf = new TFragment(source->Start);
        nf->Out.push_back(f2);
        return nf;
    }

    TFragmentPtr CloneFragment(TFragment* source) {
        Y_ASSERT(source->Start);
        TVectorTransitions newBoundaries;
        TNFAState* ns = Result->CloneState(source->Start, source->Out, newBoundaries);

        TFragmentPtr nf = new TFragment(ns);
        ::DoSwap(nf->Out, newBoundaries);

        Y_ASSERT(nf->Start);

        return nf;
    }

    TFragmentPtr CreateMinToMax(TFragment* source, int min, int max, bool greedy) {
        Y_UNUSED(greedy);

        Y_ASSERT(min >= 0);
        Y_ASSERT((max >= 1) || (max == -1));
        Y_ASSERT((min <= max) || (max == -1));

        // Create in state.
        TNFAState* s = Result->CreateState();
        TFragmentPtr nf = new TFragment(s);
        Result->CreateTransition(*s)->To = source->Start;
        // Link out.
        if (min == 0) {
            nf->Out.push_back(Result->CreateTransition(*s));
        }

        TFragmentPtr lastFragmentStorage;
        TFragment* lastFragment = source;

        // Create out state.
        TNFAState* f = Result->CreateState();
        ConnectTo(lastFragment->Out, f);
        // Link out.
        if (min <= 1) {
            nf->Out.push_back(Result->CreateTransition(*f));
        }

        // Clone fragments.
        int totalFragments = Max(1, (max != -1) ? max : min);
        // No-cloning cases are handled with other methods.
        Y_ASSERT(totalFragments >= 2);
        for (int n = 2; n <= totalFragments; ++n) {
            // Clone.
            TFragmentPtr newFragment = CloneFragment(lastFragment);
            // Link to new fragment from out state.
            Result->CreateTransition(*f)->To = newFragment->Start;
            // Create new out state.
            f = Result->CreateState();
            ConnectTo(newFragment->Out, f);
            // Link out and update pointers.
            if (n >= min) {
                nf->Out.push_back(Result->CreateTransition(*f));
            }
            s = newFragment->Start;
            lastFragmentStorage = newFragment;
            lastFragment = lastFragmentStorage.Get();
        }

        // Cycle.
        if (max == -1) {
            Result->CreateTransition(*f)->To = s;
        }

        return nf;
    }

    static void ConnectTo(TVectorTransitions& v, TNFAState* s) {
        for (size_t i = 0; i < v.size(); ++i) {
            v[i]->To = s;
        }
    }
};

void TCompile::Exec(TCompiler& c) {
    TOpPtr op;
    switch (TAstData::GetType(Node)) {
        case TAstData::Literal:
            op = new TCompileLiteral(Node, static_cast<TAstLiteral*>(Node->Data.Get()));
            break;
        case TAstData::Catenation:
            op = new TCompileCatenation(Node);
            break;
        case TAstData::Union:
            op = new TCompileUnion(Node);
            break;
        case TAstData::Iteration:
            op = new TCompileIteration(Node, static_cast<TAstIteration*>(Node->Data.Get()));
            break;
        case TAstData::Submatch:
            op = new TCompileSubmatch(Node, static_cast<TAstSubmatch*>(Node->Data.Get()));
            break;
    }
    c.OStack.push(op);
}

void TCompileLiteral::Exec(TCompiler& c) {
    TNFAState* state = c.Result->CreateState();
    TNFATransition* tr = c.Result->CreateTransition(*state, Data->Lit);
    TFragmentPtr f = new TFragment(state);
    f->Out.push_back(tr);
    c.FStack.push(f);
}

void TCompileCatenation::Exec(TCompiler& c) {
    c.OStack.push(new TCreateCatenation());
    c.OStack.push(new TCompile(Node->Left.Get()));
    c.OStack.push(new TCompile(Node->Right.Get()));
}

void TCreateCatenation::Exec(TCompiler& c) {
    TFragmentPtr left = c.FStack.top();
    c.FStack.pop();
    TFragmentPtr right = c.FStack.top();
    c.FStack.pop();
    TFragmentPtr f = new TFragment(left->Start);
    c.ConnectTo(left->Out, right->Start);
    f->Out.swap(right->Out);
    c.FStack.push(f);
}

void TCompileUnion::Exec(TCompiler& c) {
    c.OStack.push(new TCreateUnion());
    c.OStack.push(new TCompile(Node->Left.Get()));
    c.OStack.push(new TCompile(Node->Right.Get()));
}

void TCreateUnion::Exec(TCompiler& c) {
    TFragmentPtr left = c.FStack.top();
    c.FStack.pop();
    TFragmentPtr right = c.FStack.top();
    c.FStack.pop();
    TNFAState* s = c.Result->CreateState();
    TNFATransition* tr1 = c.Result->CreateTransition(*s);
    TNFATransition* tr2 = c.Result->CreateTransition(*s);
    tr1->To = left->Start;
    tr2->To = right->Start;

    TFragmentPtr f = new TFragment(s);
    f->Out.swap(left->Out);
    f->Out.insert(f->Out.end(), right->Out.begin(), right->Out.end());
    c.FStack.push(f);
}

void TCompileIteration::Exec(TCompiler& c) {
    c.OStack.push(new TCreateIteration(Data));
    c.OStack.push(new TCompile(Node->Left.Get()));
}

void TCreateIteration::Exec(TCompiler& c) {
    TFragmentPtr source = c.FStack.top();
    c.FStack.pop();
    const int min = Max(Data->Min, 0);
    const int max = Max(Data->Max, -1);
    TFragmentPtr f;
    bool requireCloning = false;
    if (min == 0) {
        if (max == 1) {
            f = c.CreateOptional(source.Get(), Data->Greedy);
        } else if (max == -1) {
            f = c.CreateZeroOrMore(source.Get(), Data->Greedy);
        } else {
            requireCloning = true;
        }
    } else if (min == 1) {
        if (max == 1) {
            f = source;
        } else if (max == -1) {
            f = c.CreateOneOrMore(source.Get(), Data->Greedy);
        } else {
            requireCloning = true;
        }
    } else {
        requireCloning = true;
    }
    if (requireCloning) {
        f = c.CreateMinToMax(source.Get(), min, max, Data->Greedy);
    }
    Y_ASSERT(!!f);
    c.FStack.push(f);
}

void TCompileSubmatch::Exec(TCompiler& c) {
    c.OStack.push(new TCreateSubmatch(Data));
    c.OStack.push(new TCompile(Node->Left.Get()));
}

void TCreateSubmatch::Exec(TCompiler& c) {
    TFragmentPtr source = c.FStack.top();
    c.FStack.pop();
    TNFAState* s1 = c.Result->CreateState();
    TNFAState* s2 = c.Result->CreateState();
    TNFATransition* tr1 = c.Result->CreateTransition(*s1, Data->Id * 2);
    TNFATransition* tr2 = c.Result->CreateTransition(*s2, Data->Id * 2 + 1);
    tr1->To = source->Start;

    c.ConnectTo(source->Out, s2);
    TFragmentPtr f = new TFragment(s1);
    f->Out.push_back(tr2);
    c.FStack.push(f);
}

struct TPriorityCalculatorState {
    TNFAState* State;
    size_t TransitionIndex;
    TPriorityCalculatorState(TNFAState* state, size_t index)
        : State(state)
        , TransitionIndex(index) {
    }
};

void CalcTransitionsPriority(TNFA& nfa) {
    TStack<TPriorityCalculatorState, TVector<TPriorityCalculatorState>> stack;
    THashSet<TStateId> visited;
    visited.insert(nfa.Start->Id);
    size_t nextPriority = 1;
    for (size_t i = nfa.Start->Transitions.size(); i > 0; --i) {
        stack.push(TPriorityCalculatorState(nfa.Start, i - 1));
        // NWRED(DBGO << "CalcTransitionsPriority: push:" << nfa.Start->Id << "-->" << nfa.Start->Transitions[i - 1]->To->Id << Endl);
    }
    while (!stack.empty()) {
        TPriorityCalculatorState st = stack.top();
        stack.pop();
        TNFATransition& tr = *st.State->Transitions[st.TransitionIndex];
        // NWRED(DBGO << "CalcTransitionsPriority: p=" << nextPriority << ", pop:" << st.State->Id << "-->" << tr->To->Id << Endl);
        tr.Priority = nextPriority++;
        if (!visited.contains(tr.To->Id)) {
            for (size_t i = tr.To->Transitions.size(); i > 0; --i) {
                stack.push(TPriorityCalculatorState(tr.To, i - 1));
                // NWRED(DBGO << "CalcTransitionsPriority: push:" << tr->To->Id << "-->" << tr->To->Transitions[i - 1]->To->Id << Endl);
            }
            visited.insert(tr.To->Id);
        }
    }
}

TCompiler::TCompiler(const TNFAPtr& nfa, TAstNode* ast)
    : Result(nfa) {
    TOpPtr op = new TCompile(ast);
    OStack.push(op);
    while (!OStack.empty()) {
        op = OStack.top();
        OStack.pop();
        NWRED(DBGO << "Exec: " << op->GetTypeString() << Endl);
        op->Exec(*this);
    }
    if (FStack.size() > 1) {
        NWRED(DBGO << "FStack.size()=" << FStack.size() << Endl);
        ythrow TCompilerError() << "internal error";
    }
    TFragmentPtr f = FStack.top();
    TNFAState* final = Result->CreateState();
    ConnectTo(f->Out, final);
    Result->Start = f->Start;
    Result->FinalStates.push_back(final);
    CalcTransitionsPriority(*Result);
    Result->SubmatchIdOffsets.push_back(0);
    Result->SubmatchIdOffsets.push_back(Result->NumSubmatches);
}

void Compile(const TNFAPtr& nfa, TAstNode* ast) {
    TCompiler(nfa, ast);
}

} // NPrivate

TNFAPtr Combine(const TVectorNFAs& nfas) {
    if (nfas.size() > (Max<TRuleId>() - 1u)) {
        ythrow TCompilerError() << "too many nfas to combine";
    }
    TNFAPtr result(new TNFA());
    result->Start = result->CreateState();
    for (size_t i = 0; i < nfas.size(); ++i) {
        TNFA& n = *nfas[i];
        TStateHolder::TStorage::iterator si = n.StateHolder.Storage.begin();
        for (; si != n.StateHolder.Storage.end(); ++si) {
            result->StateHolder.Store(si->second.Get());
        }
        TTagId submatchIdOffset = result->NumSubmatches;
        TTagId tagIdOffset = submatchIdOffset * 2;
        for (size_t j = 0; j < n.TaggedTransitions.size(); ++j) {
            TNFATransition* tr = n.TaggedTransitions[j];
            size_t newTagId = (size_t)tagIdOffset + tr->TagId;
            if (newTagId > Max<TTagId>()) {
                throw NPrivate::TTooManyTags();
            }
            tr->TagId = tagIdOffset + tr->TagId;
            result->TaggedTransitions.push_back(tr);
        }
        TSubmatchIdToName::iterator smi = n.SubmatchIdToName.begin();
        for (; smi != n.SubmatchIdToName.end(); ++smi) {
            result->SubmatchIdToName[smi->first + submatchIdOffset] = smi->second;
        }
        TSubmatchIdToSourcePos::iterator smitsi = n.SubmatchIdToSourcePos.begin();
        for (; smitsi != n.SubmatchIdToSourcePos.end(); ++smitsi) {
            result->SubmatchIdToSourcePos[smitsi->first + submatchIdOffset] = smitsi->second;
        }
        result->NumSubmatches += n.NumSubmatches;
        result->SubmatchIdOffsets.push_back(submatchIdOffset);
        result->FinalStates.push_back(n.FinalStates[0]);
        TNFATransition* tr = result->CreateTransition(*result->Start);
        tr->To = n.Start;
    }
    result->SubmatchIdOffsets.push_back(result->NumSubmatches);
    NPrivate::CalcTransitionsPriority(*result);
    return result;
}

} // NRemorph

#undef DBGO
