#include "logic_expr.h"
#include "bool_stack.h"

#include <kernel/remorph/input/lemma_quality.h>

#include <util/ysaveload.h>

static_assert(LANG_MAX <= sizeof(ui8) * 255, "expect LANG_MAX <= sizeof(ui8) * 255");

template <>
TString ToString<NLiteral::TLInstrBasePtr>(const NLiteral::TLInstrBasePtr& i) {
    switch (i.Type) {
#define X(T) case NLiteral::TLInstrBasePtr::T: return i.Get##T().ToString();
        LOGIC_EXPR_INSTR_LIST
#undef X
    }
    return TString();
}

void TSerializer<NLiteral::TLInstrBasePtr>::Save(IOutputStream* out, const NLiteral::TLInstrBasePtr& p) {
    ::Save(out, (ui8)p.Type);
    ::Save(out, p.ExpId);
    switch (p.Type) {
#define X(A) case NLiteral::TLInstrBasePtr::A: p.Get##A().Save(out); break;
        LOGIC_EXPR_INSTR_LIST
#undef X
    }
}

void TSerializer<NLiteral::TLInstrBasePtr>::Load(IInputStream* in, NLiteral::TLInstrBasePtr& res) {
    ui8 type = 0;
    NLiteral::TExpressionId expId = Max<NLiteral::TExpressionId>();
    ::Load(in, type);
    ::Load(in, expId);
    switch (type) {
#define X(A) case NLiteral::TLInstrBasePtr::A: res = NLiteral::TLInstr##A::Load(in); break;
        LOGIC_EXPR_INSTR_LIST
#undef X
    }
    res.ExpId = expId;
}

namespace NLiteral {

void TLInstrLang::Save(IOutputStream* out) const {
    ::Save(out, Language);
}

TLInstrLang* TLInstrLang::Load(IInputStream* in) {
    TLangMask lang;
    ::Load(in, lang);
    return new TLInstrLang(lang);
}


void TLInstrQLang::Save(IOutputStream* out) const {
    ::Save(out, Language);
}

TLInstrQLang* TLInstrQLang::Load(IInputStream* in) {
    TLangMask lang;
    ::Load(in, lang);
    return new TLInstrQLang(lang);
}

void TLInstrGzt::Save(IOutputStream* out) const {
    ::Save(out, GztArts);
}

TLInstrGzt* TLInstrGzt::Load(IInputStream* in) {
    TVector<TUtf16String> gztArts;
    ::Load(in, gztArts);
    return new TLInstrGzt(gztArts);
}

void TLInstrGztAttr::Save(IOutputStream* out) const {
    ::Save(out, GztType);
    ::Save(out, GztAttr);
    ::Save(out, GztValue);
}

TLInstrGztAttr* TLInstrGztAttr::Load(IInputStream* in) {
    TUtf16String type;
    TString attr;
    TString val;
    ::Load(in, type);
    ::Load(in, attr);
    ::Load(in, val);
    return new TLInstrGztAttr(type, attr, val);
}

void TLInstrGram::Save(IOutputStream* out) const {
    ::Save(out, Grammems.ToString(","));
}

TLInstrGram* TLInstrGram::Load(IInputStream* in) {
    TString gram;
    ::Load(in, gram);
    return new TLInstrGram(gram);
}

void TLInstrText::Save(IOutputStream* out) const {
    ::Save(out, Text);
}

TLInstrText* TLInstrText::Load(IInputStream* in) {
    TUtf16String text;
    ::Load(in, text);
    return new TLInstrText(text);
}

void TLInstrTextRE::Save(IOutputStream* out) const {
    RegExpr->Save(out);
}

TLInstrTextRE* TLInstrTextRE::Load(IInputStream* in) {
    NRemorph::NCommon::TRegExpr::TPtr regExpr = NRemorph::NCommon::TRegExpr::Load(in);
    return new TLInstrTextRE(regExpr);
}

void TLInstrNText::Save(IOutputStream* out) const {
    ::Save(out, Text);
}

TLInstrNText* TLInstrNText::Load(IInputStream* in) {
    TUtf16String text;
    ::Load(in, text);
    return new TLInstrNText(text);
}

void TLInstrNTextRE::Save(IOutputStream* out) const {
    RegExpr->Save(out);
}

TLInstrNTextRE* TLInstrNTextRE::Load(IInputStream* in) {
    NRemorph::NCommon::TRegExpr::TPtr regExpr = NRemorph::NCommon::TRegExpr::Load(in);
    return new TLInstrNTextRE(regExpr);
}

void TLInstrProp::Save(IOutputStream* out) const {
    ::Save(out, Props);
}

TLInstrProp* TLInstrProp::Load(IInputStream* in) {
    TPropertyBitSet props;
    ::Load(in, props);
    return new TLInstrProp(props);
}

void TLInstrLemQ::Save(IOutputStream* out) const {
    ::Save(out, LemmaQualities);
}

TLInstrLemQ* TLInstrLemQ::Load(IInputStream* in) {
    TLemmaQualityBitSet lemmaQualities;
    ::Load(in, lemmaQualities);
    return new TLInstrLemQ(lemmaQualities);
}

void TLInstrLen::Save(IOutputStream* out) const {
    ::Save(out, static_cast<ui8>(CompMode));
    ::SaveSize(out, Len);
}

TLInstrLen* TLInstrLen::Load(IInputStream* in) {
    ui8 compMode;
    ::Load(in, compMode);
    size_t len = ::LoadSize(in);
    return new TLInstrLen(len, static_cast<ENumberCompMode>(compMode));
}

void TLInstrJump::Save(IOutputStream* out) const {
    ::Save(out, (ui8)Condition);
    ::Save(out, IntegerCast<ui32>(Offset));
}

TLInstrJump* TLInstrJump::Load(IInputStream* in) {
    ui8 cond = 0;
    ui32 off = -1;
    ::Load(in, cond);
    ::Load(in, off);
    return new TLInstrJump(0 != cond, off);
}

namespace NPrivate {

static bool Exec(const TLInstrLang& i, TInputSymbol& s, TDynBitMap&, TExpressionId) {
    const TLangMask& langMask = s.GetLangMask();
    return i.Language.HasAny(langMask) || (i.Language.Empty() && langMask.Empty());
}

static bool Exec(const TLInstrQLang& i, TInputSymbol& s, TDynBitMap&, TExpressionId) {
    const TLangMask& langMask = s.GetQLangMask();
    return i.Language.HasAny(langMask) || (i.Language.Empty() && langMask.Empty());
}

static bool Exec(const TLInstrGzt& i, TInputSymbol& s, TDynBitMap& ctx, TExpressionId expId) {
    bool res = false;
    if (s.HasMatchResult(expId, res)) {
        if (res) {
            // It is important to fill gazetteer context for cached result because weight calculation
            // takes into account only articles from the context.
            // Other match functions can still return empty context
            ctx.Set(0, s.GetGztArticles().size());
        }
        return res;
    }

    TDynBitMap gztCtx;
    for (const TUtf16String& a : i.GztArts) {
        res = s.HasGztArticle(a, gztCtx) || res;
    }

    if (!res || gztCtx.Count() == s.GetGztArticles().size())
        s.SetMatchResult(expId, res);

    if (res)
        ctx |= gztCtx;

    return res;
}

struct TArticleAttrCheck {
    const TLInstrGztAttr& Instr;
    TDynBitMap& Ctx;
    size_t Ndx;

    TArticleAttrCheck(const TLInstrGztAttr& i, TDynBitMap& ctx)
        : Instr(i)
        , Ctx(ctx)
        , Ndx(ctx.FirstNonZeroBit())
    {
    }

    inline bool operator() (const TArticlePtr& a) {
        Y_ASSERT(Ndx < Ctx.Size() && Ctx.Test(Ndx));
        if (a.GetTypeName() != Instr.GztType || !NGztSupport::HasFieldValue(*a, Instr.GztAttr, Instr.GztValue)) {
            Ctx.Reset(Ndx);
        }
        Ndx = Ctx.NextNonZeroBit(Ndx);
        return false;
    }
};

static bool Exec(const TLInstrGztAttr& i, TInputSymbol& s, TDynBitMap& ctx, TExpressionId expId) {
    bool res = false;
    if (s.HasMatchResult(expId, res))
        return res;

    TDynBitMap gztCtx;
    if (s.HasGztArticle(i.GztType, gztCtx) && (s.IsGztCtxEmpty(ctx) || !gztCtx.And(ctx).Empty())) {
        TArticleAttrCheck check(i, gztCtx);
        s.TraverseArticles(gztCtx, check);
        res = !gztCtx.Empty();
    }

    if (!res || gztCtx.Count() == s.GetGztArticles().size())
        s.SetMatchResult(expId, res);

    if (res)
        ctx |= gztCtx;

    return res;
}

static bool Exec(const TLInstrProp& i, TInputSymbol& s, TDynBitMap&, TExpressionId) {
    return s.GetProperties().HasAny(i.Props);
}

static bool Exec(const TLInstrText& i, TInputSymbol& s, TDynBitMap&, TExpressionId) {
    return i.Text == s.GetText();
}

static bool Exec(const TLInstrTextRE& i, TInputSymbol& s, TDynBitMap&, TExpressionId expId) {
    bool res = false;
    return s.HasMatchResult(expId, res) ? res : s.SetMatchResult(expId, i.RegExpr->Match(s.GetText()));
}

static bool Exec(const TLInstrNText& i, TInputSymbol& s, TDynBitMap&, TExpressionId) {
    return i.Text == s.GetNormalizedText();
}

static bool Exec(const TLInstrNTextRE& i, TInputSymbol& s, TDynBitMap&, TExpressionId expId) {
    bool res = false;
    return s.HasMatchResult(expId, res) ? res : s.SetMatchResult(expId, i.RegExpr->Match(s.GetNormalizedText()));
}

static bool Exec(const TLInstrGram& i, TInputSymbol& s, TDynBitMap& ctx, TExpressionId expId) {
    bool res = false;
    if (s.HasMatchResult(expId, res))
        return res;

    bool fullMatch = false;
    res = s.HasGrammems(i.Grammems, ctx, fullMatch);

    if (!res || fullMatch)
        s.SetMatchResult(expId, res);

    return res;
}

static bool Exec(const TLInstrLemQ& i, TInputSymbol& s, TDynBitMap& ctx, TExpressionId) {
    return s.HasLemmaQualities(i.LemmaQualities, ctx);
}

static bool Exec(const TLInstrLen& i, TInputSymbol& s, TDynBitMap&, TExpressionId) {
    switch (i.CompMode) {
    case NCM_EQUALS:
        return s.GetText().size() == i.Len;
    case NCM_LESS:
        return s.GetText().size() < i.Len;
    case NCM_GREATER:
        return s.GetText().size() > i.Len;
    }
    return false;
}

#if defined(__GNUC__) || defined(__clang__)
#define HAVE_COMPUTED_GOTO 1
#else
#define HAVE_COMPUTED_GOTO 0
#endif

bool Exec(const TLInstrVector& vv, TInputSymbol& s, TDynBitMap& ctx) {
    STACK_DECLARE(stack);
    TVector<TDynBitMap> ctxBackup;
#if HAVE_COMPUTED_GOTO
    void* targets[] = {
#define X(T) &&EXEC_##T,
        LOGIC_EXPR_INSTR_LIST
#undef X
    };
#endif
    for (size_t i = 0; i < vv.size(); ++i) {
#if HAVE_COMPUTED_GOTO
#define TARGET(T) EXEC_##T: case TLInstrBasePtr::T
        goto *targets[vv[i].Type];
#else
#define TARGET(T) case TLInstrBasePtr::T
#endif

        switch (vv[i].Type) {
        TARGET(Not):
            STACK_INVERT(stack);
            Y_ASSERT(!ctxBackup.empty());
            DoSwap(ctxBackup.back(), ctx);
            ctxBackup.pop_back();
            continue;

        TARGET(And):
            STACK_JOIN(stack, &&);
            continue;

        TARGET(Or):
            STACK_JOIN(stack, ||);
            continue;

        TARGET(Jump):
            if (vv[i].GetJump().Condition == STACK_TOP(stack)) {
                i += vv[i].GetJump().Offset;
                Y_ASSERT(i <= vv.size());
            }
            continue;

        TARGET(Backup):
            ctxBackup.push_back(ctx);
            continue;

#define X(T) \
        TARGET(T): {                                            \
            bool val = Exec(vv[i].Get##T(), s, ctx, vv[i].ExpId); \
            STACK_PUSH(stack, val);                             \
            continue;                                           \
        }
        LOGIC_EXPR_INSTR_LIST_AV
#undef X
        }
        Y_ASSERT(false);  // unknown opcode
    }
    STACK_VALIDATE(stack);
    return STACK_TOP(stack);
}

void CollectUsedGztItems(const TLInstrVector& v, THashSet<TUtf16String>& result) {
    for (auto& instr : v) {
        switch (instr.Type) {
        case TLInstrBasePtr::Gzt:
            result.insert(instr.GetGzt().GztArts.begin(), instr.GetGzt().GztArts.end());
            break;
        case TLInstrBasePtr::GztAttr:
            result.insert(instr.GetGztAttr().GztType);
            break;
        default:
            break;
        }
    }
}

} // NPrivate

} // NLiteral
