#pragma once

#include "logic_expr.h"

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/input/input_symbol.h>
#include <kernel/remorph/input/lemmas.h>

#include <util/charset/wide.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>

#define NRM_LT_ELEMENTS \
    X(Single)           \
    X(Set)              \
    X(LemmaSet)         \
    X(Logic)

namespace NLiteral {

using namespace NSymbol;

class TLTElementBase;
typedef TIntrusivePtr<TLTElementBase> TLTElementPtr;

class TLTElementBase: public TSimpleRefCount<TLTElementBase> {
public:
    enum TType {
#define X(A) A,
        NRM_LT_ELEMENTS
#undef X
    };
public:
    const TType Type;
    TString Name;
    TExpressionId ExpId;
protected:
    TLTElementBase(TType type, const TString& name)
        : Type(type)
        , Name(name)
        , ExpId(Max<TExpressionId>())
    {
    }

public:
    virtual ~TLTElementBase() {
    }

    inline const TString& ToString() const {
        return Name;
    }

    void Save(IOutputStream* out) const;
    TLTElementPtr Clone(const TString& suffix) const;

    static TLTElementPtr Load(IInputStream* in);
};

template <TLTElementBase::TType Type_>
class TLTElementBaseT: public TLTElementBase {
protected:
    TLTElementBaseT(const TString& name)
        : TLTElementBase(Type_, name)
    {
    }
};

#define NRM_LT_ELEMENT(A)                                               \
    struct TLTElement##A: public TLTElementBaseT<TLTElementBase::A> {   \
        typedef TLTElementBaseT<TLTElementBase::A> Base;                \
        explicit TLTElement##A(const TString& name)                      \
            : Base(name)                                                \
        {                                                               \
        }


NRM_LT_ELEMENT(Single)
    TUtf16String Text;

    inline void Load(IInputStream* in) {
        ::Load(in, Text);
    }

    inline void Save(IOutputStream* out) const {
        ::Save(out, Text);
    }

    inline TLTElementPtr Clone(const TString& suffix) const {
        TAutoPtr<TLTElementSingle> ret(new TLTElementSingle(Name + suffix));
        ret->ExpId = ExpId;
        ret->Text = Text;
        return ret.Release();
    }
};

typedef TIntrusivePtr<TLTElementSingle> TLTElementSinglePtr;

typedef THashSet<TUtf16String, THash<TUtf16String>, TEqualTo<TWtringBuf>> TSetWtroka;

NRM_LT_ELEMENT(Set)
    TSetWtroka TextSet;

    inline void Load(IInputStream* in) {
        ::Load(in, TextSet);
    }

    inline void Save(IOutputStream* out) const {
        ::Save(out, TextSet);
    }

    inline TLTElementPtr Clone(const TString& suffix) const {
        TAutoPtr<TLTElementSet> ret(new TLTElementSet(Name + suffix));
        ret->ExpId = ExpId;
        ret->TextSet = TextSet;
        return ret.Release();
    }
};

typedef TIntrusivePtr<TLTElementSet> TLTElementSetPtr;

NRM_LT_ELEMENT(LemmaSet)
    TSetWtroka LemmaSet;

    inline void Load(IInputStream* in) {
        ::Load(in, LemmaSet);
    }

    inline void Save(IOutputStream* out) const {
        ::Save(out, LemmaSet);
    }

    inline TLTElementPtr Clone(const TString& suffix) const {
        TAutoPtr<TLTElementLemmaSet> ret(new TLTElementLemmaSet(Name + suffix));
        ret->ExpId = ExpId;
        ret->LemmaSet = LemmaSet;
        return ret.Release();
    }
};

typedef TIntrusivePtr<TLTElementLemmaSet> TLTElementLemmaSetPtr;

NRM_LT_ELEMENT(Logic)
    TLInstrVector Instr;

    inline void Load(IInputStream* in) {
        ::Load(in, Instr);
    }

    inline void Save(IOutputStream* out) const {
        ::Save(out, Instr);
    }

    inline TLTElementPtr Clone(const TString& suffix) const {
        TAutoPtr<TLTElementLogic> ret(new TLTElementLogic(Name + suffix));
        ret->ExpId = ExpId;
        ret->Instr = Instr;
        return ret.Release();
    }
};

typedef TIntrusivePtr<TLTElementLogic> TLTElementLogicPtr;

inline TLTElementPtr TLTElementBase::Clone(const TString& suffix) const {
    switch (Type) {
#define X(A) case TLTElementBase::A: return static_cast<const TLTElement##A*>(this)->Clone(suffix);
        NRM_LT_ELEMENTS
#undef X
    }
    return TLTElementPtr();
}

inline TLTElementSinglePtr CreateWordLiteral(const TString& word) {
    TLTElementSinglePtr e(new TLTElementSingle("word#" + word));
    e->Text = UTF8ToWide(word);
    return e.Get();
}

inline TLTElementSetPtr CreateClassLiteral() {
    return new TLTElementSet("class");
}

TLTElementSetPtr CreateClassLiteral(const TString& path);

inline TLTElementLemmaSetPtr CreateLemmaLiteral() {
    return new TLTElementLemmaSet("class*");
}

TLTElementLemmaSetPtr CreateLemmaLiteral(const TString& path);

inline TLTElementLogicPtr CreateLogicLiteral() {
    return new TLTElementLogic("class?");
}

namespace NPrivate {

inline bool IsEqualSingle(const TLTElementSingle& e, TInputSymbol& s, TDynBitMap&) {
    bool res = false;
    return s.HasMatchResult(e.ExpId, res)
        ? res
        : s.SetMatchResult(e.ExpId, s.GetNormalizedText() == e.Text);
}

inline bool IsEqualSet(const TLTElementSet& e, TInputSymbol& s, TDynBitMap&) {
    bool res = false;
    return s.HasMatchResult(e.ExpId, res)
        ? res
        : s.SetMatchResult(e.ExpId, e.TextSet.contains(s.GetNormalizedText()));
}

struct TLemmaCheck {
    const TSetWtroka& LemmaSet;

    TLemmaCheck(const TSetWtroka& ls)
        : LemmaSet(ls)
    {
    }

    inline bool operator() (const ILemmas& lemmas, size_t i) const {
        return LemmaSet.contains(lemmas.GetLemmaText(i));
    }
};

inline bool IsEqualLemmaSet(const TLTElementLemmaSet& e, TInputSymbol& s, TDynBitMap& ctx) {
    bool res = false;
    if (s.HasMatchResult(e.ExpId, res))
        return res;

    bool fullMatch = false;
    res = s.CheckLemmas(TLemmaCheck(e.LemmaSet), ctx, fullMatch);

    if (!res || fullMatch)
        s.SetMatchResult(e.ExpId, res);

    return res;
}

inline bool IsEqualLogic(const TLTElementLogic& e, TInputSymbol& s, TDynBitMap& ctx) {
    return Exec(e.Instr, s, ctx);
}

Y_FORCE_INLINE bool IsEqual(const TLTElementBase& e, TInputSymbol& s, TDynBitMap& ctx) {
    switch (e.Type) {
#define X(A) case TLTElementBase::A: return IsEqual##A(static_cast<const TLTElement##A&>(e), s, ctx);
        NRM_LT_ELEMENTS
#undef X
    }

    return false;
}

} // NPrivate

} // NLiteral


template<>
class TSerializer<NLiteral::TLTElementPtr> {
public:
    static inline void Save(IOutputStream* rh, const NLiteral::TLTElementPtr& p) {
        p->Save(rh);
    }

    static inline void Load(IInputStream* rh, NLiteral::TLTElementPtr& p) {
        p = NLiteral::TLTElementBase::Load(rh);
    }
};
