#pragma once

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/literal/bool_stack.h>
#include <kernel/remorph/literal/literal_table.h>

#include <util/system/yassert.h>
#include <util/string/cast.h>
#include <util/generic/bitmap.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <library/cpp/containers/sorted_vector/sorted_vector.h>
#include <util/ysaveload.h>

namespace NTokenLogic {

#define TOKEN_EXPR_INSTR_LIST   \
    X(Const)                    \
    X(Cmp)                      \
    X(Logical)                  \
    X(Skip)

#define TOKEN_CMP_LIST          \
    X(Eq)                       \
    X(Neq)                      \
    X(Lte)                      \
    X(Gte)                      \
    X(Lt)                       \
    X(Gt)

#define TOKEN_LOGICAL_LIST      \
    X(Not)                      \
    X(And)                      \
    X(Xor)                      \
    X(Or)

class TTokenExpBase;
typedef TIntrusivePtr<TTokenExpBase> TTokenExpBasePtr;
typedef TVector<TTokenExpBasePtr> TTokenExpVector;

class TTokenExpBase: public TAtomicRefCount<TTokenExpBase> {
public:
    enum TType {
#define X(A) A,
        TOKEN_EXPR_INSTR_LIST
#undef X
    };
public:
    const TType Type;
protected:
    TTokenExpBase(TType t)
        : Type(t)
    {
    }
public:
    virtual ~TTokenExpBase() {
    }

    TString ToString(const NLiteral::TLiteralTable& lt) const;
    void Save(IOutputStream* out) const;
    static TTokenExpBasePtr Load(IInputStream* in);
};

template <TTokenExpBase::TType T>
struct TTokenExpBaseT: public TTokenExpBase {
protected:
    TTokenExpBaseT()
        : TTokenExpBase(T)
    {
    }
};

#define TOKEN_EXPR_INSTR(T) struct TTokenExp##T: public TTokenExpBaseT<TTokenExpBase::T>

TOKEN_EXPR_INSTR(Const) {
    const bool Value;

    TTokenExpConst(bool val)
        : Value(val)
    {
    }

    inline TString ToString(const NLiteral::TLiteralTable&) const {
        return Value ? "True" : "False";
    }

    void Save(IOutputStream* out) const {
        ::Save(out, (ui8)Value);
    }
    static TTokenExpBasePtr Load(IInputStream* in) {
        ui8 val;
        ::Load(in, val);
        return new TTokenExpConst(val != 0);
    }
};

TOKEN_EXPR_INSTR(Cmp) {
    enum TCmp {
#define X(A) A,
        TOKEN_CMP_LIST
#undef X
    };
    const NRemorph::TLiteral Lit;
    const NSorted::TSortedVector<TString> Labels;
    const TCmp Cmp;
    const ui32 Count;

    TTokenExpCmp(NRemorph::TLiteral l, const NSorted::TSortedVector<TString>& labels, TCmp cmp, ui32 count)
        : Lit(l)
        , Labels(labels)
        , Cmp(cmp)
        , Count(count)
    {
    }

    inline TString ToString(const NLiteral::TLiteralTable& lt) const {
        TString op;
        op.append('[').append(lt.ToString(Lit)).append(']');
        for (size_t i = 0; i < Labels.size(); ++i) {
            op.append(':').append(Labels[i]);
        }
        op.append(' ');
        switch (Cmp) {
#define X(A) case A: op.append(#A); break;
            TOKEN_CMP_LIST
#undef X
        }
        return op.append(' ').append(::ToString(Count));
    }

    void Save(IOutputStream* out) const {
        ::Save(out, Lit);
        ::Save(out, Labels);
        ::Save(out, (ui8)Cmp);
        ::Save(out, Count);
    }
    static TTokenExpBasePtr Load(IInputStream* in) {
        NRemorph::TLiteral l;
        NSorted::TSortedVector<TString> labels;
        ui8 cmp;
        ui32 cnt;
        ::Load(in, l);
        ::Load(in, labels);
        ::Load(in, cmp);
        ::Load(in, cnt);
        if (cmp > Gt) {
            throw yexception() << "loaded unknown compare op: " << cmp;
        }
        return new TTokenExpCmp(l, labels, (TCmp)cmp, cnt);
    }
};

TOKEN_EXPR_INSTR(Logical) {
    enum TLogicOp {
#define X(A) A,
        TOKEN_LOGICAL_LIST
#undef X
    };
    const TLogicOp Op;
    TTokenExpLogical(TLogicOp op)
        : Op(op)
    {
    }

    inline TString ToString(const NLiteral::TLiteralTable&) const {
        switch (Op) {
#define X(A) case A: return #A;
            TOKEN_LOGICAL_LIST
#undef X
        }
        return TString();
    }

    void Save(IOutputStream* out) const {
        ::Save(out, (ui8)Op);
    }
    static TTokenExpBasePtr Load(IInputStream* in) {
        ui8 op;
        ::Load(in, op);
        if (op > Or) {
            throw yexception() << "loaded unknown logical op: " << op;
        }
        return new TTokenExpLogical((TLogicOp)op);
    }
};

// Used to skip second branch calculation in case of definite result of And/Or logical group
TOKEN_EXPR_INSTR(Skip) {
    bool Condition;
    size_t Offset;

    TTokenExpSkip(bool val, size_t off)
        : Condition(val)
        , Offset(off)
    {
    }

    inline TString ToString(const NLiteral::TLiteralTable&) const {
        TString res("SkipOn");
        res.append(Condition ? "True" : "False").append('(').append(::ToString(Offset)).append(')');
        return res;
    }

    void Save(IOutputStream* out) const {
        ::Save(out, Condition);
        ::SaveSize(out, Offset);
    }
    static TTokenExpBasePtr Load(IInputStream* in) {
        bool cond;
        ::Load(in, cond);
        size_t offset = ::LoadSize(in);
        return new TTokenExpSkip(cond, offset);
    }
};

/////////////////////////////////////////////////////////////////////////

inline TString TTokenExpBase::ToString(const NLiteral::TLiteralTable& lt) const {
    switch (Type) {
#define X(A) case TTokenExpBase::A: return static_cast<const TTokenExp##A*>(this)->ToString(lt);
        TOKEN_EXPR_INSTR_LIST
#undef X
    }
    return TString();
}

inline void TTokenExpBase::Save(IOutputStream* out) const {
    ::Save(out, (ui8)Type);
    switch (Type) {
#define X(A) case TTokenExpBase::A: static_cast<const TTokenExp##A*>(this)->Save(out); break;
        TOKEN_EXPR_INSTR_LIST
#undef X
    }
}

inline TTokenExpBasePtr TTokenExpBase::Load(IInputStream* in) {
    ui8 type = 0;
    ::Load(in, type);
    switch (type) {
#define X(A) case TTokenExpBase::A: return TTokenExp##A::Load(in);
        TOKEN_EXPR_INSTR_LIST
#undef X
    }
    return TTokenExpBasePtr();
}

/////////////////////////////////////////////////////////////////////////

inline TString ToString(const NLiteral::TLiteralTable& lt, const TTokenExpVector& exp) {
    TString res;
    if (!exp.empty()) {
        res = exp.front()->ToString(lt);
        for (size_t i = 1; i < exp.size(); ++i) {
            res.append(' ').append(exp[i]->ToString(lt));
        }
    }
    return res;
}

inline void LogicalJoin(TTokenExpVector& left, const TTokenExpVector& right, TTokenExpLogical::TLogicOp op) {
    if (left.empty()) {
        left.assign(right.begin(), right.end());
    } else if (!right.empty()) {
        if (op == TTokenExpLogical::Xor) {
            left.reserve(left.size() + right.size() + 1);
        } else {
            left.reserve(left.size() + right.size() + 2);
            left.push_back(new TTokenExpSkip(TTokenExpLogical::Or == op, right.size() + 1));
        }
        left.insert(left.end(), right.begin(), right.end());
        left.push_back(new TTokenExpLogical(op));
    }
}

template <class TCounts>
inline bool EvalTokenExp(const TTokenExpVector& vv, TCounts& counts) {
    STACK_DECLARE(stack);
    for (size_t i = 0; i < vv.size(); ++i) {
        const TTokenExpBase& instr = *vv[i];
        switch (instr.Type) {
        case TTokenExpBase::Logical:
            switch (static_cast<const TTokenExpLogical&>(instr).Op) {
                case TTokenExpLogical::Not:
                    STACK_INVERT(stack);
                    break;
                case TTokenExpLogical::And:
                    STACK_JOIN(stack, &&);
                    break;
                case TTokenExpLogical::Or:
                    STACK_JOIN(stack, ||);
                    break;
                case TTokenExpLogical::Xor:
                    STACK_XOR(stack);
                    break;
            }
            break;
        case TTokenExpBase::Skip:
            if (static_cast<const TTokenExpSkip&>(instr).Condition == STACK_TOP(stack)) {
                i += static_cast<const TTokenExpSkip&>(instr).Offset;
                Y_ASSERT(i <= vv.size());
            }
            break;
        case TTokenExpBase::Const:
            STACK_PUSH(stack, static_cast<const TTokenExpConst&>(instr).Value);
            break;
        default:
            {
                const TTokenExpCmp& cmpInstr = static_cast<const TTokenExpCmp&>(instr);
                Y_ASSERT(cmpInstr.Lit.IsOrdinal() || cmpInstr.Lit.IsAny());
                const ui32 count = counts.GetCount(cmpInstr);
                bool val = false;
                switch (cmpInstr.Cmp) {
                case TTokenExpCmp::Eq: val = (count == cmpInstr.Count); break;
                case TTokenExpCmp::Neq: val = (count != cmpInstr.Count); break;
                case TTokenExpCmp::Lte: val = (count <= cmpInstr.Count); break;
                case TTokenExpCmp::Lt: val = (count < cmpInstr.Count); break;
                case TTokenExpCmp::Gte: val = (count >= cmpInstr.Count); break;
                case TTokenExpCmp::Gt: val = (count > cmpInstr.Count); break;
                default: Y_ASSERT(false);
                }
                STACK_PUSH(stack, val);
            }
        }
    }
    STACK_VALIDATE(stack);
    return STACK_TOP(stack);
}

} // NTokenLogic

template<>
class TSerializer<NTokenLogic::TTokenExpBasePtr> {
public:
    static inline void Save(IOutputStream* rh, const NTokenLogic::TTokenExpBasePtr& p) {
        p->Save(rh);
    }

    static inline void Load(IInputStream* rh, NTokenLogic::TTokenExpBasePtr& p) {
        p = NTokenLogic::TTokenExpBase::Load(rh);
    }
};
