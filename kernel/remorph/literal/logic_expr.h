#pragma once

#include <kernel/remorph/common/article_util.h>
#include <kernel/remorph/common/reg_expr.h>
#include <kernel/remorph/input/input_symbol.h>
#include <kernel/remorph/input/lemma_quality.h>

#include <kernel/lemmer/dictlib/grambitset.h>
#include <library/cpp/langmask/langmask.h>

#include <library/cpp/charset/doccodes.h>
#include <util/charset/wide.h>
#include <util/system/yassert.h>
#include <util/string/cast.h>
#include <util/string/vector.h>
#include <util/string/strip.h>
#include <util/stream/output.h>
#include <util/generic/algorithm.h>
#include <util/generic/bitmap.h>
#include <util/generic/cast.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NLiteral {

using namespace NSymbol;

#define LOGIC_EXPR_INSTR_LIST_AV                \
    X(Lang)                                     \
    X(QLang)                                    \
    X(Gzt)                                      \
    X(GztAttr)                                  \
    X(Gram)                                     \
    X(Text)                                     \
    X(TextRE)                                   \
    X(NText)                                    \
    X(NTextRE)                                  \
    X(Prop)                                     \
    X(LemQ)                                     \
    X(Len)

#define LOGIC_EXPR_INSTR_LIST                   \
    X(Not)                                      \
    X(And)                                      \
    X(Or)                                       \
    X(Jump)                                     \
    X(Backup)                                   \
    LOGIC_EXPR_INSTR_LIST_AV

#define CACHEABLE_LOGIC_EXPR_INSTR_LIST         \
    X(Gzt)                                      \
    X(GztAttr)                                  \
    X(Gram)                                     \
    X(TextRE)                                   \
    X(NTextRE)

struct TLInstrBase: TSimpleRefCount<TLInstrBase> {
    virtual ~TLInstrBase() {}
};

enum ENumberCompMode {
    NCM_EQUALS = 0,
    NCM_LESS,
    NCM_GREATER
};

struct TLInstrLang: TLInstrBase {
    TLangMask Language;
    TLInstrLang(const TLangMask& lang)
        : Language(lang)
    {
    }

    inline TString ToString() const {
        return TString("Lang=") + NLanguageMasks::ToString(Language);
    }

    void Save(IOutputStream* out) const;
    static TLInstrLang* Load(IInputStream* in);
};

struct TLInstrQLang: TLInstrBase {
    TLangMask Language;
    TLInstrQLang(const TLangMask& lang)
        : Language(lang)
    {
    }

    inline TString ToString() const {
        return TString("QLang=") + NLanguageMasks::ToString(Language);
    }

    void Save(IOutputStream* out) const;
    static TLInstrQLang* Load(IInputStream* in);
};

struct TLInstrGzt: TLInstrBase {
    TVector<TUtf16String> GztArts;

    TLInstrGzt(const TVector<TUtf16String>& arts)
        : GztArts(arts)
    {
        for (TUtf16String& s : GztArts) {
            StripString(s, s);
        }
    }

    inline TString ToString() const {
        return TString("Gzt=") + WideToUTF8(JoinStrings(GztArts, u","));
    }

    void Save(IOutputStream* out) const;
    static TLInstrGzt* Load(IInputStream* in);
};

struct TLInstrGztAttr: TLInstrBase {
    TUtf16String GztType;
    TString GztAttr;
    TString GztValue;
    TLInstrGztAttr(const TUtf16String& type, const TString& attr, const TString& val)
        : GztType(type)
        , GztAttr(attr)
        , GztValue(val)
    {
    }

    inline TString ToString() const {
        return TString("Gzt.").append(WideToUTF8(GztType)).append('.').append(GztAttr).append('=').append(GztValue);
    }

    void Save(IOutputStream* out) const;
    static TLInstrGztAttr* Load(IInputStream* in);
};

struct TLInstrGram: TLInstrBase {
    TGramBitSet Grammems;
    TLInstrGram(const TString& sGrammems)
        : Grammems(TGramBitSet::FromString(sGrammems))
    {
    }

    inline TString ToString() const {
        return TString("Gram=") + Grammems.ToString(",");
    }

    void Save(IOutputStream* out) const;
    static TLInstrGram* Load(IInputStream* in);
};

struct TLInstrText: TLInstrBase {
    TUtf16String Text;

    TLInstrText(const TUtf16String& text)
        : Text(text)
    {
    }

    inline TString ToString() const {
        return TString("Text=") + WideToUTF8(Text);
    }

    void Save(IOutputStream* out) const;
    static TLInstrText* Load(IInputStream* in);
};

struct TLInstrTextRE: TLInstrBase {
    NRemorph::NCommon::TRegExpr::TPtr RegExpr;

    TLInstrTextRE(const TString& expr, NRemorph::NCommon::TRegExprModifiers modifiers)
        : RegExpr(new NRemorph::NCommon::TRegExpr(expr, modifiers))
    {
    }

    TLInstrTextRE(const NRemorph::NCommon::TRegExpr::TPtr& regExpr)
        : RegExpr(regExpr)
    {
    }

    inline TString ToString() const {
        Y_ASSERT(RegExpr);
        return TString("TextRE=") + ::ToString(*RegExpr);
    }

    void Save(IOutputStream* out) const;
    static TLInstrTextRE* Load(IInputStream* in);
};

struct TLInstrNText: TLInstrBase {
    TUtf16String Text;

    TLInstrNText(const TUtf16String& text)
        : Text(text)
    {
    }

    inline TString ToString() const {
        return TString("NText=") + WideToUTF8(Text);
    }

    void Save(IOutputStream* out) const;
    static TLInstrNText* Load(IInputStream* in);
};

struct TLInstrNTextRE: TLInstrBase {
    NRemorph::NCommon::TRegExpr::TPtr RegExpr;

    TLInstrNTextRE(const TString& expr, NRemorph::NCommon::TRegExprModifiers modifiers)
        : RegExpr(new NRemorph::NCommon::TRegExpr(expr, modifiers))
    {
    }

    TLInstrNTextRE(const NRemorph::NCommon::TRegExpr::TPtr& regExpr)
        : RegExpr(regExpr)
    {
    }

    inline TString ToString() const {
        Y_ASSERT(RegExpr);
        return TString("NTextRE=") + ::ToString(*RegExpr);
    }

    void Save(IOutputStream* out) const;
    static TLInstrNTextRE* Load(IInputStream* in);
};

struct TLInstrProp: TLInstrBase {
    TPropertyBitSet Props;
    TLInstrProp(const TPropertyBitSet& props)
        : Props(props)
    {
    }

    inline TString ToString() const {
        return TString("Prop=") + NSymbol::ToString(Props);
    }

    void Save(IOutputStream* out) const;
    static TLInstrProp* Load(IInputStream* in);
};

struct TLInstrLemQ: TLInstrBase {
    NSymbol::TLemmaQualityBitSet LemmaQualities;

    TLInstrLemQ(const NSymbol::TLemmaQualityBitSet& lemmaQualities)
        : LemmaQualities(lemmaQualities)
    {
    }

    inline TString ToString() const {
        return TString("LemQ=") + NSymbol::ToString(LemmaQualities);
    }

    void Save(IOutputStream* out) const;
    static TLInstrLemQ* Load(IInputStream* in);
};

struct TLInstrLen: TLInstrBase {
    size_t Len;
    ENumberCompMode CompMode;

    TLInstrLen(const size_t len, ENumberCompMode compMode = NCM_EQUALS)
        : Len(len)
        , CompMode(compMode)
    {
    }

    inline TString ToString() const {
        TString repr("Len");
        switch (CompMode) {
            case NCM_EQUALS:
                repr += '=';
                break;
            case NCM_LESS:
                repr += '<';
                break;
            case NCM_GREATER:
                repr += '>';
                break;
        }
        return repr + ::ToString(Len);
    }

    void Save(IOutputStream* out) const;
    static TLInstrLen* Load(IInputStream* in);
};

// Used to skip second branch calculation in case of definite result of And/Or logical group
struct TLInstrJump: TLInstrBase {
    bool Condition;
    size_t Offset;

    TLInstrJump(bool val, size_t off)
        : Condition(val)
        , Offset(off)
    {
    }

    inline TString ToString() const {
        TString res("JumpOn");
        res.append(Condition ? "True" : "False").append('(').append(::ToString(Offset)).append(')');
        return res;
    }

    void Save(IOutputStream* out) const;
    static TLInstrJump* Load(IInputStream* in);
};

#define LOGIC_EXPR_INSTR_STATELESS(name) \
    struct TLInstr##name: TLInstrBase {             \
        TString ToString() const {                   \
            return #name;                           \
        }                                           \
        void Save(IOutputStream*) const {}          \
        static TLInstr##name* Load(IInputStream*) { \
            return new TLInstr##name;               \
        }                                           \
    }
LOGIC_EXPR_INSTR_STATELESS(Not);
LOGIC_EXPR_INSTR_STATELESS(And);
LOGIC_EXPR_INSTR_STATELESS(Or);
LOGIC_EXPR_INSTR_STATELESS(Backup);
#undef LOGIC_EXPR_INSTR_STATELESS

/////////////////////////////////////////////////////////////////////////

class TLInstrBasePtr: public TIntrusivePtr<TLInstrBase> {
public:
    enum TType {
#define X(T) T,
        LOGIC_EXPR_INSTR_LIST
#undef X
    };

    TType Type;
    TExpressionId ExpId;

    TLInstrBasePtr() : ExpId(Max<TExpressionId>()) {}
#define X(T) \
    TLInstrBasePtr(TLInstr##T* raw)                     \
        : TIntrusivePtr(static_cast<TLInstrBase*>(raw)) \
        , Type(T)                                       \
        , ExpId(Max<TExpressionId>())                   \
    {}                                                  \
    const TLInstr##T& Get##T() const {                  \
        return *static_cast<const TLInstr##T*>(Get());  \
    }
    LOGIC_EXPR_INSTR_LIST
#undef X
};

using TLInstrVector = TVector<TLInstrBasePtr>;

namespace NPrivate {
    bool Exec(const TLInstrVector& v, TInputSymbol& s, TDynBitMap& ctx);
    void CollectUsedGztItems(const TLInstrVector& logicExp, THashSet<TUtf16String>& result);
}

} // NLiteral

template <>
TString ToString<NLiteral::TLInstrBasePtr>(const NLiteral::TLInstrBasePtr& i);

template<>
class TSerializer<NLiteral::TLInstrBasePtr> {
public:
    static void Save(IOutputStream*, const NLiteral::TLInstrBasePtr&);
    static void Load(IInputStream*, NLiteral::TLInstrBasePtr&);
};
