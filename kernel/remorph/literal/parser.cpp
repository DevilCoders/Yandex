#include "parser.h"
#include "lexer.h"

#include <kernel/remorph/common/reg_expr.h>
#include <kernel/remorph/input/lemma_quality.h>
#include <kernel/remorph/input/properties.h>

#include <library/cpp/langmask/langmask.h>

#include <util/generic/ptr.h>
#include <util/generic/stack.h>
#include <util/generic/yexception.h>
#include <library/cpp/charset/recyr.hh>
#include <util/string/vector.h>
#include <util/charset/wide.h>
#include <util/string/split.h>

namespace NLiteral {

enum ECompMode {
    CM_EQUALS = 0,
    CM_NOT_EQUALS,
    CM_LESS,
    CM_GREATER
};

static const TString SPACE = TString(" ");
static const TString DOT = TString(".");

#define UNEXPECTED_TOKEN(lexer)                                              \
    do { throw TLiteralParseError(lexer.SourcePos) << "unexpected token " << ToString(lexer.Token); } while (false)

struct TLogicParserContext;
TLInstrBasePtr CreateAttrValInstr(const TString& attr, const NRemorph::TSourcePos& attrCtx, const TString& value,
                                  const NRemorph::TSourcePos& valCtx, const TString* reModsStr = nullptr,
                                  ENumberCompMode numberCompMode = NCM_EQUALS);

struct TLogicParserFuncBase: public TSimpleRefCount<TLogicParserFuncBase> {
    virtual ~TLogicParserFuncBase() {
    }
    virtual void Exec(TLogicParserContext& c) const = 0;
    virtual const char* GetName() const = 0;
};

typedef TIntrusivePtr<TLogicParserFuncBase> TLogicParserFuncBasePtr;

struct TLogicParserContext {
    typedef TStack<TLogicParserFuncBasePtr, TVector<TLogicParserFuncBasePtr>> TStackType;
    TLiteralLexer& Lexer;
    const THashMap<TString, TLInstrVector>& Defs;
    TLInstrVector& Result;
    TStackType Stack;
    TLogicParserContext(TLiteralLexer& lexer, const THashMap<TString, TLInstrVector>& defs, TLInstrVector& result)
        : Lexer(lexer)
        , Defs(defs)
        , Result(result)
    {
        Lexer.NextToken();
    }
};

#define LOGIC_PARSER_FUNC_DECL(A)                           \
    struct TParse##A: public TLogicParserFuncBase {         \
        virtual void Exec(TLogicParserContext& c) const;    \
        virtual const char* GetName() const {               \
            return #A;                                      \
        }                                                   \
    }

#define LOGIC_PARSER_FUNC_SKIP_DECL(A)                      \
    struct TParse##A: public TLogicParserFuncBase {         \
        size_t Offset;                                      \
        TParse##A(const TLogicParserContext& c) {           \
            Offset = c.Result.size();                       \
        }                                                   \
        virtual void Exec(TLogicParserContext& c) const;    \
        virtual const char* GetName() const {               \
            return #A;                                      \
        }                                                   \
    }

#define LOGIC_PARSER_FUNC_DEF(A)                        \
    void TParse##A::Exec(TLogicParserContext& c) const

#define LOGIC_PARSER_PUSH(A)                    \
    c.Stack.push(new TParse##A())

#define LOGIC_PARSER_PUSH_CTX(A)                \
    c.Stack.push(new TParse##A(c))

LOGIC_PARSER_FUNC_DECL(OrList);
LOGIC_PARSER_FUNC_DECL(AndList);
LOGIC_PARSER_FUNC_DECL(ORest);
LOGIC_PARSER_FUNC_DECL(ARest);
LOGIC_PARSER_FUNC_DECL(Term);
LOGIC_PARSER_FUNC_DECL(BExp);
LOGIC_PARSER_FUNC_DECL(RParen);
LOGIC_PARSER_FUNC_DECL(PushOr);
LOGIC_PARSER_FUNC_DECL(PushAnd);
LOGIC_PARSER_FUNC_DECL(PushNot);
LOGIC_PARSER_FUNC_SKIP_DECL(OSkip);
LOGIC_PARSER_FUNC_SKIP_DECL(ASkip);

LOGIC_PARSER_FUNC_DEF(OrList) {
    LOGIC_PARSER_PUSH(ORest);
    LOGIC_PARSER_PUSH(AndList);
}

LOGIC_PARSER_FUNC_DEF(ORest) {
    if (c.Lexer.Token != RLT_OR)
        return;
    c.Lexer.NextToken();
    LOGIC_PARSER_PUSH(ORest);
    LOGIC_PARSER_PUSH_CTX(OSkip);
    LOGIC_PARSER_PUSH(PushOr);
    LOGIC_PARSER_PUSH(AndList);
}

LOGIC_PARSER_FUNC_DEF(AndList) {
    LOGIC_PARSER_PUSH(ARest);
    LOGIC_PARSER_PUSH(Term);
}

LOGIC_PARSER_FUNC_DEF(ARest) {
    if (c.Lexer.Token != RLT_AND)
        return;
    c.Lexer.NextToken();
    LOGIC_PARSER_PUSH(ARest);
    LOGIC_PARSER_PUSH_CTX(ASkip);
    LOGIC_PARSER_PUSH(PushAnd);
    LOGIC_PARSER_PUSH(Term);
}

LOGIC_PARSER_FUNC_DEF(Term) {
    if (c.Lexer.Token == RLT_NOT) {
        c.Lexer.NextToken();
        c.Result.push_back(TLInstrBasePtr(new TLInstrBackup()));
        LOGIC_PARSER_PUSH(PushNot);
    }
    LOGIC_PARSER_PUSH(BExp);
}

LOGIC_PARSER_FUNC_DEF(BExp) {
    switch (c.Lexer.Token) {
        case RLT_ID: {
            const TString attr = c.Lexer.TokenValue;
            const NRemorph::TSourcePos attrCtx = c.Lexer.SourcePos;
            ECompMode compMode = CM_EQUALS;
            c.Lexer.NextToken();
            switch (c.Lexer.Token) {
                case RLT_EQUAL:
                    break;
                case RLT_NOT_EQUAL:
                    compMode = CM_NOT_EQUALS;
                    break;
                case RLT_LESS:
                    compMode = CM_LESS;
                    break;
                case RLT_GREATER:
                    compMode = CM_GREATER;
                    break;
                default:
                    UNEXPECTED_TOKEN(c.Lexer);
            }
            c.Lexer.NextToken();
            if ((c.Lexer.Token != RLT_WORD) && (c.Lexer.Token != RLT_RE)) {
                UNEXPECTED_TOKEN(c.Lexer);
            }
            TString* reMods = (c.Lexer.Token == RLT_RE) ? &c.Lexer.Suffix : nullptr;
            if (compMode == CM_NOT_EQUALS) {
                c.Result.push_back(TLInstrBasePtr(new TLInstrBackup()));
            }
            ENumberCompMode numberCompMode = NCM_EQUALS;
            switch (compMode) {
                case CM_LESS:
                    numberCompMode = NCM_LESS;
                    break;
                case CM_GREATER:
                    numberCompMode = NCM_GREATER;
                    break;
                default:
                    break;
            }
            c.Result.push_back(CreateAttrValInstr(attr, attrCtx, c.Lexer.TokenValue, c.Lexer.SourcePos, reMods,
                                                  numberCompMode));
            if (compMode == CM_NOT_EQUALS) {
                c.Result.push_back(TLInstrBasePtr(new TLInstrNot));
            }
            c.Lexer.NextToken();
            break;
        }
        case RLT_REFERENCE: {
            TString id = c.Lexer.TokenValue;
            THashMap<TString, TLInstrVector>::const_iterator i = c.Defs.find(id);
            if (i == c.Defs.end())
                throw TLiteralParseError(c.Lexer.SourcePos) << "Logic id \"" << id << "\" is not defined";
            c.Result.insert(c.Result.end(), i->second.begin(), i->second.end());
            c.Lexer.NextToken();
            break;
        }
        case RLT_LPAREN:
            c.Lexer.NextToken();
            LOGIC_PARSER_PUSH(RParen);
            LOGIC_PARSER_PUSH(OrList);
            break;
        default:
            UNEXPECTED_TOKEN(c.Lexer);
    }
}

LOGIC_PARSER_FUNC_DEF(PushOr) {
    c.Result.push_back(TLInstrBasePtr(new TLInstrOr));
}

LOGIC_PARSER_FUNC_DEF(PushAnd) {
    c.Result.push_back(TLInstrBasePtr(new TLInstrAnd));
}

LOGIC_PARSER_FUNC_DEF(PushNot) {
    c.Result.push_back(TLInstrBasePtr(new TLInstrNot));
}

LOGIC_PARSER_FUNC_DEF(RParen) {
    if (c.Lexer.Token != RLT_RPAREN)
        UNEXPECTED_TOKEN(c.Lexer);
    c.Lexer.NextToken();
}

LOGIC_PARSER_FUNC_DEF(OSkip) {
    c.Result.insert(c.Result.begin() + Offset, TLInstrBasePtr(new TLInstrJump(true, c.Result.size() - Offset)));
}

LOGIC_PARSER_FUNC_DEF(ASkip) {
    c.Result.insert(c.Result.begin() + Offset, TLInstrBasePtr(new TLInstrJump(false, c.Result.size() - Offset)));
}

TLInstrBasePtr CreateAttrValInstr(const TString& attr, const NRemorph::TSourcePos& attrCtx, const TString& value,
                                  const NRemorph::TSourcePos& valCtx, const TString* reModsStr,
                                  ENumberCompMode numberCompMode) {
    bool regExpr = false;
    NRemorph::NCommon::TRegExprModifiers regExprModifiers;

    if (reModsStr) {
        if ((attr != "text") && (attr != "ntext")) {
            throw TLiteralParseError(attrCtx) << "regular expressions syntax are not supported for " << attr << " attribute";
        }

        regExpr = true;
        regExprModifiers = NRemorph::NCommon::ParseRegExprModifiers(*reModsStr);
    }

    if (numberCompMode != NCM_EQUALS) {
        if (attr != "len") {
            throw TLiteralParseError(attrCtx) << "less and greater comparisons are not supported for " << attr << " attribute";
        }
    }

    regExpr = regExpr || (attr == "reg") || (attr == "ireg");
    if (attr == "ireg") {
        regExprModifiers.Set(NRemorph::NCommon::REM_IGNORE_CASE);
    }

    if (attr.StartsWith("gzt.")) {
        TVector<TString> res;
        StringSplitter(attr).SplitByString(DOT.data()).SkipEmpty().Collect(&res);
        if (res.size() < 3)
            throw TLiteralParseError(attrCtx) << "invalid gzt attribute syntax: \"" << attr << "\". Expected gzt.<type>.<attribute>";
        return new TLInstrGztAttr(UTF8ToWide(res[1]), JoinStrings(res.begin() + 2, res.end(), DOT), value);
    }
    if (attr == "gzt") {
        TVector<TUtf16String> res;
        TUtf16String delim = u",";
        StringSplitter(UTF8ToWide(value)).SplitByString(delim.data()).SkipEmpty().Collect(&res);
        return new TLInstrGzt(res);
    }
    if (attr == "lang") {
        try {
            TLangMask lang = NLanguageMasks::CreateFromList(value);
            return new TLInstrLang(lang);
        } catch (const yexception& e) {
            throw TLiteralParseError(valCtx) << e.what();
        }
    }
    if (attr == "qlang") {
        try {
            TLangMask lang = NLanguageMasks::CreateFromList(value);
            return new TLInstrQLang(lang);
        } catch (const yexception& e) {
            throw TLiteralParseError(valCtx) << e.what();
        }
    }
    if (attr == "gram") {
        TString value1251 = ::Recode(CODES_UTF8, CODES_WIN, value);
        try {
            return new TLInstrGram(value1251);
        } catch (const yexception& e) {
            throw TLiteralParseError(valCtx) << e.what();
        }
    }
    if ((attr == "text") || (attr == "reg") || (attr == "ireg")) {
        try {
            if (regExpr) {
                return new TLInstrTextRE(value, regExprModifiers);
            } else {
                return new TLInstrText(UTF8ToWide(value));
            }
        } catch (const yexception& e) {
            throw TLiteralParseError(valCtx) << e.what();
        }
    }
    if (attr == "ntext") {
        if (regExpr) {
            return new TLInstrNTextRE(value, regExprModifiers);
        } else {
            return new TLInstrNText(UTF8ToWide(value));
        }
    }
    if (attr == "prop") {
        try {
            return new TLInstrProp(NSymbol::ParseProperties(value));
        } catch (const yexception& e) {
            throw TLiteralParseError(valCtx) << e.what();
        }
    }
    if (attr == "lemq") {
        try {
            return new TLInstrLemQ(NSymbol::ParseLemmaQualities(value));
        } catch (const yexception& e) {
            throw TLiteralParseError(valCtx) << e.what();
        }
    }
    if (attr == "len") {
        try {
            return new TLInstrLen(::FromString<size_t>(value), numberCompMode);
        } catch (const TFromStringException& error) {
            throw TLiteralParseError(valCtx) << "len attribute value is not a number: " << value;
        } catch (const yexception& error) {
            throw TLiteralParseError(valCtx) << error.what();
        }
    }
    throw TLiteralParseError(attrCtx) << "unknown attribute: " << attr;
    return TLInstrBasePtr();
}

TLInstrVector ParseLogic(TLiteralLexer& lexer, const THashMap<TString, TLInstrVector>& defs) {
    TLInstrVector result;
    lexer.SetStartConditionLOGIC();
    TLogicParserContext c(lexer, defs, result);
    LOGIC_PARSER_PUSH(OrList);
    while (!c.Stack.empty()) {
        TLogicParserFuncBasePtr i = c.Stack.top();
        c.Stack.pop();
        // Cerr << i->GetName() << Endl;
        i->Exec(c);
    }
    return result;
}

TLInstrVector ParseLogic(const TString& str, const THashMap<TString, TLInstrVector>& defs) {
    TLiteralLexer lexer(str);
    return ParseLogic(lexer, defs);
}

void ParseSet(TLiteralLexer& lexer, TString& name, TSetWtroka& elements) {
    for (; lexer.Token == RLT_WORD; lexer.NextToken()) {
        name.append('#').append(lexer.TokenValue);
        elements.insert(UTF8ToWide(lexer.TokenValue));
    }
}

TLTElementPtr ParseClassLiteral(TLiteralLexer& lexer) {
    TLTElementSetPtr e = CreateClassLiteral();
    ParseSet(lexer, e->Name, e->TextSet);
    return e.Get();
}

TLTElementPtr ParseLemmaLiteral(TLiteralLexer& lexer) {
    TLTElementLemmaSetPtr e = CreateLemmaLiteral();
    ParseSet(lexer, e->Name, e->LemmaSet);
    return e.Get();
}

TLTElementPtr ParseLogicLiteral(TLiteralLexer& lexer, const THashMap<TString, TLInstrVector>& defs) {
    TLTElementLogicPtr e = CreateLogicLiteral();

    e->Instr = ParseLogic(lexer, defs);
    if (lexer.Token != RLT_EOF)
        UNEXPECTED_TOKEN(lexer);

    e->Name.append(JoinStrings(e->Instr.begin(), e->Instr.end(), SPACE));
    return e.Get();
}

TLTElementPtr ParseLiteral(const TString& str, const TString& ctxDir, const THashMap<TString, TLInstrVector>& defs) {
    TLiteralLexer lexer(str);

    TLTElementPtr res;
    lexer.NextToken();
    switch (lexer.Token) {
    case RLT_TYPE_LEMMA:
        lexer.NextToken();
        res = ParseLemmaLiteral(lexer);
        break;
    case RLT_WORD:
        res = ParseClassLiteral(lexer);
        break;
    case RLT_TYPE_LOGIC:
        res = ParseLogicLiteral(lexer, defs);
        break;
    case RLT_TYPE_FILE:
        try {
            res = CreateClassLiteral(ctxDir + GetDirectorySeparator() + lexer.TokenValue).Get();
        } catch (const yexception& e) {
            throw TLiteralParseError(lexer.SourcePos) << ": " << e.what();
        }
        lexer.NextToken();
        break;
    case RLT_TYPE_LEMMA_FILE:
        try {
            res = CreateLemmaLiteral(ctxDir + GetDirectorySeparator() + lexer.TokenValue).Get();
        } catch (const yexception& e) {
            throw TLiteralParseError(lexer.SourcePos) << ": " << e.what();
        }
        lexer.NextToken();
        break;
    default:
        UNEXPECTED_TOKEN(lexer);
    }
    if (lexer.Token != RLT_EOF)
        UNEXPECTED_TOKEN(lexer);
    return res;
}

} // NLiteral
