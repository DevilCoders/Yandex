#include "rule_lexer.h"
#include "rule_parser.h"

#include <kernel/remorph/common/verbose.h>
#include <kernel/remorph/core/core.h>
#include <kernel/remorph/literal/agreement.h>
#include <kernel/remorph/literal/logic_expr.h>
#include <kernel/remorph/literal/ltelement.h>
#include <kernel/remorph/literal/parser.h>

#include <util/charset/wide.h>
#include <util/datetime/cputimer.h>
#include <util/folder/dirut.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/string/vector.h>
#include <util/string/cast.h>
#include <util/system/defaults.h>
#include <utility>

namespace NReMorph {

namespace NPrivate {

using namespace NLiteral;

static const TString SPACE = TString(" ");

class TSyntaxError: public yexception {
};

#define UNEXPECTED_TOKEN()                                              \
    do { throw TSyntaxError() << "unexpected token " << ToString(Lexer.Token); } while (false)

#define CHECK_TOKEN(T) \
    do { if (T != Lexer.Token) UNEXPECTED_TOKEN(); } while (false)

struct TDefinition {
    NRemorph::TAstNodePtr Ast;
    NRemorph::TSetLiterals MarkedHeads;
};

typedef THashMap<TString, TDefinition> TDefMap;
typedef THashMap<TString, TLInstrVector> TLogicMap;
typedef THashMap<TString, TString> TStringMap;
typedef THashMap<TString, TExpressionId> TExpressionMap;

struct THeadCollector {
    NRemorph::TSetLiterals Heads;
    bool Explicit;

    THeadCollector()
        : Explicit(false)
    {
    }

    void SetExplicit() {
        if (!Explicit)
            Heads.clear();
        Explicit = true;
    }
};

struct TParserContext {
    TParseResult& Res;
    TDefMap DefMap;
    TLogicMap LogicMap;
    TStringMap StringMap;
    size_t NextClassId;
    TExpressionMap Expressions;

    TVector<TString> Dirs;
    size_t TotalLinesProcessed;
    TParserContext(TParseResult& rules, const TString& dir)
        : Res(rules)
        , NextClassId(0)
        , Dirs()
        , TotalLinesProcessed(0)
    {
        Res.LiteralTable = new TLiteralTable();
        Dirs.push_back(dir);
    }

    TExpressionId GetExpressionId(const TString& exp) {
        TExpressionMap::iterator i = Expressions.find(exp);
        if (i == Expressions.end()) {
            return Expressions.insert(std::make_pair(exp, static_cast<TExpressionId>(Expressions.size()))).first->second;
        }
        return i->second;
    }

    const TString& GetDir() const {
        Y_ASSERT(!Dirs.empty());
        return Dirs.back();
    }

    const TString& GetBaseDir() const {
        Y_ASSERT(!Dirs.empty());
        return Dirs.front();
    }
};

struct TCollectHeadsVisitor {

    const static size_t AmbiguousLiteral   = 0;
    const static size_t AmbiguousHead      = 1;
    const static size_t ExplicitHead       = 2;

    const NRemorph::TSetLiterals& MarkedHeads;
    NRemorph::TSetLiterals LocalHeads;
    std::bitset<3> Flags;
    // Count of found heads for each level
    TVector<size_t> Levels;

    TCollectHeadsVisitor(const NRemorph::TSetLiterals& markedHeads)
        : MarkedHeads(markedHeads)
    {
    }

    bool IsValid() const {
        return Flags.test(ExplicitHead) ? !Flags.test(AmbiguousHead) : !Flags.test(AmbiguousLiteral);
    }

    inline void HandleBeforeNonLiteral(bool ambiguous) {
        if (ambiguous) {
            Flags.set(AmbiguousLiteral);
        }
        Levels.push_back(0);
    }

    void HandleBeforeLiteral(NRemorph::TLiteral l) {
        if (l.IsOrdinal()) {
            if (MarkedHeads.count(l) != 0) {
                // We have explicitly marked head
                if (!Flags.test(ExplicitHead)) {
                    // Reset already collected implicit heads and start collecting only explicit ones
                    LocalHeads.clear();
                }
                LocalHeads.insert(l);
                Flags.set(ExplicitHead);
                if (!Levels.empty()) {
                    ++Levels.back();
                }
            } else if (!Flags.test(ExplicitHead) && !Flags.test(AmbiguousLiteral)) {
                // Collect implicit head for valid "single match" expression
                LocalHeads.insert(l);
            }
        } else {
            Flags.set(AmbiguousLiteral);
        }
    }

    bool VisitBeforeChildren(const NRemorph::TAstNode* node) {
        switch (NRemorph::TAstData::GetType(node)) {
        case NRemorph::TAstData::Catenation:
        case NRemorph::TAstData::Iteration:
            HandleBeforeNonLiteral(true);
            break;
        case NRemorph::TAstData::Union:
        case NRemorph::TAstData::Submatch:
            HandleBeforeNonLiteral(false);
            break;
        case NRemorph::TAstData::Literal:
            HandleBeforeLiteral(static_cast<NRemorph::TAstLiteral*>(node->Data.Get())->Lit);
            break;
        default:
            Y_ASSERT(false);
        }
        return !Flags.test(AmbiguousHead);
    }

    inline void HandleAfterIteration() {
        if (Levels.back() != 0) {
            Flags.set(AmbiguousHead);
        }
        Levels.pop_back();
    }

    inline void HandleAfterCatenation() {
        bool hasHead = Levels.back() != 0;
        if (Levels.back() > 1) {
            Flags.set(AmbiguousHead);
        }
        Levels.pop_back();
        if (!Levels.empty() && hasHead) {
            ++Levels.back();
        }
    }

    inline void HandleAfterUnion() {
        bool hasHead = Levels.back() != 0;
        if (Levels.back() == 1) {
            // Only one branch specifies the head but must both.
            Flags.set(AmbiguousHead);
        }
        Levels.pop_back();
        if (!Levels.empty() && hasHead) {
            ++Levels.back();
        }
    }

    inline void HandleAfterSubmatch() {
        size_t current = Levels.back();
        Levels.pop_back();
        if (!Levels.empty()) {
            Levels.back() += current;
        }
    }

    inline void HandleAfterLiteral() {
    }

    bool VisitAfterChildren(const NRemorph::TAstNode* node) {
        Y_ASSERT(NRemorph::TAstData::Literal == NRemorph::TAstData::GetType(node) || !Levels.empty());

        switch (NRemorph::TAstData::GetType(node)) {
#define X(A) case NRemorph::TAstData::A: HandleAfter##A(); break;
            WRE_AST_NODE_TYPES_LIST
#undef X
        }
        return !Flags.test(AmbiguousHead);
    }
};

struct TAttachAgreementVisitor {
    TParserContext& Context;
    THeadCollector& Collector;
    const NRemorph::TSetLiterals& MarkedHeads;
    const NRemorph::TSetLiterals& LocalHeads;
    const bool HeadLabel;
    const TVector<TAgreementPtr>& Agreements;

    TAttachAgreementVisitor(TParserContext& context, THeadCollector& collector,
        const NRemorph::TSetLiterals& markedHeads, const NRemorph::TSetLiterals& localHeads,
        bool headLabel, const TVector<TAgreementPtr>& agreements)
        : Context(context)
        , Collector(collector)
        , MarkedHeads(markedHeads)
        , LocalHeads(localHeads)
        , HeadLabel(headLabel)
        , Agreements(agreements)
    {
        Y_ASSERT(HeadLabel || !Agreements.empty());
    }

    bool VisitBeforeChildren(const NRemorph::TAstNode* node) const {
        if (NRemorph::TAstData::Literal == NRemorph::TAstData::GetType(node)) {
            NRemorph::TLiteral oldLit = static_cast<NRemorph::TAstLiteral*>(node->Data.Get())->Lit;
            if (LocalHeads.count(oldLit) != 0) {
                Y_ASSERT(oldLit.IsOrdinal());
                // Create new literal and attach agreements to it
                TLTElementPtr e = Context.Res.LiteralTable->Get(oldLit)->Clone("#" + ::ToString(++Context.NextClassId));
                NRemorph::TLiteral newLit = Context.Res.LiteralTable->Add(e);
                Context.Res.LiteralTable->CopyAgreements(oldLit, newLit);
                static_cast<NRemorph::TAstLiteral*>(node->Data.Get())->Lit = newLit;

                if (HeadLabel || (!Collector.Explicit && MarkedHeads.contains(oldLit))) {
                    Collector.Heads.insert(newLit);
                }
                if (!Agreements.empty()) {
                    Context.Res.LiteralTable->GetAgreementGroup(newLit).AddAgreements(Agreements);
                }
            }
        }
        return true;
    }

    bool VisitAfterChildren(const NRemorph::TAstNode*) const {
        return true;
    }
};

struct TParser {
    TParserContext& Context;
    TString FileName;
    IInputStream& Input;
    TRuleLexer Lexer;

    TParser(IInputStream& in, const TString& fileName, TParserContext& context, bool included = false)
        : Context(context)
        , FileName(fileName)
        , Input(in)
        , Lexer(Input, fileName)
    {
        try {
            if (!included) {
                REPORT(DETAIL, "Parsing... ");
                TProfileTimer t;
                while (ParseLine())
                    ;
                TDuration d = t.Get();
                REPORT(INFO, Context.Res.NFAs.size() << " rule(s), " << d.MilliSeconds() << "ms");
            } else {
                while (ParseLine())
                    ;
            }
        } catch (const TLexerError& e) {
            throw yexception() << GetErrorContext() << ": lexic error: " << e.what();
        } catch (const TSyntaxError& e) {
            throw yexception() << GetErrorContext() << ": syntax error: " << e.what();
        } catch (const NRemorph::TCompilerError& e) {
            throw yexception() << GetErrorContext() << ": re compile error: " << e.what();
        } catch (const NRemorph::TParserError& e) {
            throw yexception() << GetErrorContext() << ": re syntax error: " << e.what();
        } catch (const NLiteral::TLiteralParseError& e) {
            UpdateErrorContext(e);
            throw yexception() << GetErrorContext() << ": literal parse error: " << e.what();
        }
    }

    TString GetPath() const {
        return Context.GetBaseDir() + GetDirectorySeparator() + FileName;
    }

    TString GetErrorContext() const {
        TString retval;
        TStringOutput out(retval);
        out << Lexer.SourcePos;
        return retval;
    }

    void UpdateErrorContext(const NLiteral::TLiteralParseError& e) {
        if (e.Ctx.EndLine == 1) {
            Lexer.SourcePos.EndCol = Lexer.SourcePos.BegCol + e.Ctx.EndCol;
            Lexer.SourcePos.EndLine = Lexer.SourcePos.BegLine;
        } else {
            Lexer.SourcePos.EndLine = Lexer.SourcePos.BegLine + e.Ctx.EndLine - 1;
            Lexer.SourcePos.EndCol = e.Ctx.EndCol;
        }
        if (e.Ctx.BegLine == 1) {
            Lexer.SourcePos.BegCol += e.Ctx.BegCol;
        } else {
            Lexer.SourcePos.BegLine += e.Ctx.BegLine - 1;
            Lexer.SourcePos.BegCol = e.Ctx.BegCol;
        }
    }

    bool ParseLine() {
        Lexer.SetStartConditionINITIAL();
        Lexer.NextToken();
        switch (Lexer.Token) {
            case RLT_DEF:
                ParseDef();
                break;
            case RLT_LOGIC:
                ParseLogicDef();
                break;
            case RLT_STRING:
                ParseString();
                break;
            case RLT_RULE:
                ParseRule();
                break;
            case RLT_INCLUDE:
                ParseInclude();
                break;
            case RLT_USE_GZT:
                ParseUseGzt();
                break;
            case RLT_EOF:
                REPORT(INFO, "Processed " << Context.TotalLinesProcessed << " lines from '" << FileName << "'");
                return false;
            default:
                UNEXPECTED_TOKEN();
        }
        ++Context.TotalLinesProcessed;
        return true;
    }

    void ParseDef() {
        Lexer.SetStartConditionID();
        Lexer.NextToken();
        CHECK_TOKEN(RLT_ID);
        TString id = Lexer.TokenValue;
        CheckDefId(id);
        Lexer.NextToken();
        CHECK_TOKEN(RLT_EOID);
        TDefinition& def = Context.DefMap[id];
        THeadCollector collector;
        def.Ast = ParseRE(collector);
        DoSwap(def.MarkedHeads, collector.Heads);
    }

    void ParseLogicDef() {
        Lexer.SetStartConditionID();
        Lexer.NextToken();
        CHECK_TOKEN(RLT_ID);
        TString id = Lexer.TokenValue;
        CheckLogicId(id);
        Lexer.NextToken();
        CHECK_TOKEN(RLT_EOID);
        Lexer.SetStartConditionLITERAL();
        Lexer.NextToken();
        CHECK_TOKEN(RLT_EOL);
        TString logicStr = Lexer.TokenValue;
        TLInstrVector& res = Context.LogicMap[id];
        res = NLiteral::ParseLogic(logicStr, Context.LogicMap);
        SetExpressionIds(res);
    }

    NRemorph::TTokenPtr ParseSingleWord(THeadCollector& collector) {
        TLTElementSinglePtr e = NLiteral::CreateWordLiteral(Lexer.TokenValue);
        SetExpressionId(*e);
        return new NRemorph::TTokenLiteral(ParseLiteralSuffix(e, collector));
    }

    NRemorph::TTokenPtr ParseLiteral(THeadCollector& collector) {
        Lexer.SetStartConditionLITERAL();
        Lexer.NextToken();
        CHECK_TOKEN(RLT_RBRACKET);
        TLTElementPtr e = NLiteral::ParseLiteral(Lexer.TokenValue, Context.GetDir(), Context.LogicMap);
        SetExpressionId(*e);
        Lexer.SetStartConditionRE();
        return new NRemorph::TTokenLiteral(ParseLiteralSuffix(e, collector));
    }

    NRemorph::TTokenPtr ParseDefRef(THeadCollector& collector) {
        TString id = Lexer.TokenValue;
        const TDefinition& def = GetDefRefValue(id);
        NRemorph::TAstNodePtr node = def.Ast;
        TVector<TAgreementPtr> agreements;
        bool isHead = ParseSuffix(agreements);
        if (isHead || !agreements.empty()) {
            if (isHead) {
                collector.SetExplicit();
            }
            TCollectHeadsVisitor headsVisitor(def.MarkedHeads);
            NRemorph::TAstTree::Traverse(node.Get(), headsVisitor);
            if (!headsVisitor.IsValid()) {
                throw TSyntaxError() << (isHead ? "Head marker" : "Agreement") << " is specified for the \"" << id
                    << "\" definition, which matches ambiguous number of tokens";
            }

            node = NRemorph::TAstTree::Clone(node.Get());
            TAttachAgreementVisitor attachVisitor(Context, collector, def.MarkedHeads,
                headsVisitor.LocalHeads, isHead, agreements);
            NRemorph::TAstTree::Traverse(node.Get(), attachVisitor);
        } else if (!collector.Explicit) {
            collector.Heads.insert(def.MarkedHeads.begin(), def.MarkedHeads.end());
        }
        return new NRemorph::TTokenRE(node.Get());
    }

    void ParseString() {
        Lexer.SetStartConditionID();
        Lexer.NextToken();
        CHECK_TOKEN(RLT_ID);
        TString id = Lexer.TokenValue;
        CheckStringId(id);
        Lexer.NextToken();
        CHECK_TOKEN(RLT_EOID);
        Lexer.SetStartConditionSTRING();
        Lexer.NextToken();
        CHECK_TOKEN(RLT_STRING_VALUE);
        Context.StringMap[id] = Lexer.TokenValue;
        Lexer.NextToken();
        CHECK_TOKEN(RLT_EOL);
    }

    void ParseRule() {
        Lexer.SetStartConditionID();
        Lexer.NextToken();
        TString id;
        double weight = 1.0;
        switch (Lexer.Token) {
            case RLT_ID:
                id = Lexer.TokenValue;
                break;
            case RLT_REFERENCE:
                id = GetStringRefValue(Lexer.TokenValue);
                break;
            default:
                UNEXPECTED_TOKEN();
        }
        Lexer.NextToken();
        if (RLT_LPAREN == Lexer.Token) {
            Lexer.NextToken();
            CHECK_TOKEN(RLT_FLOAT);
            try {
                weight = FromString<double>(Lexer.TokenValue);
            } catch (const TFromStringException& e) {
                throw yexception() << GetErrorContext() << ": invalid rule weight: " << e.what();
            }
            Lexer.NextToken();
            CHECK_TOKEN(RLT_RPAREN);
            Lexer.NextToken();
        }
        CHECK_TOKEN(RLT_EOID);
        THeadCollector dummyCollector;
        NRemorph::TAstNodePtr ast = ParseRE(dummyCollector);
        // Cdbg << "rule " << id << "'s tree:" << Endl
        //      << NRemorph::Bind(*Context.Res.LiteralTable, *ast) << Endl;
        NRemorph::TNFAPtr nfa = NRemorph::CompileNFA(*Context.Res.LiteralTable, ast.Get());
        Context.Res.NFAs.push_back(nfa);
        Context.Res.Rules.push_back(std::make_pair(id, weight));
    }

    void ParseInclude() {
        Lexer.SetStartConditionPATH();
        Lexer.NextToken();
        CHECK_TOKEN(RLT_PATH);
        TString includedPath = Context.GetDir() + GetDirectorySeparator() + Lexer.TokenValue;
        TIFStream stream(includedPath);
        Context.Dirs.push_back(GetDirName(includedPath));
        TParser parser(stream, Lexer.TokenValue, Context, true);
        Lexer.NextToken();
        CHECK_TOKEN(RLT_EOL);
        Context.Dirs.pop_back();
    }

    void ParseUseGzt() {
        Lexer.SetStartConditionLIST();
        do {
            Lexer.NextToken();
            CHECK_TOKEN(RLT_WORD);
            Context.Res.UseGzt.push_back(UTF8ToWide(Lexer.TokenValue));
            Lexer.NextToken();
        } while (Lexer.Token == RLT_COMMA);
        CHECK_TOKEN(RLT_EOL);
    }

    NRemorph::TAstNodePtr ParseRE(THeadCollector& collector) {
        NRemorph::TVectorTokens tokens;
        Lexer.SetStartConditionRE();
        Lexer.NextToken();
        for (; Lexer.Token != RLT_EOL; Lexer.NextToken()) {
            switch (Lexer.Token) {
                case RLT_WORD:
                    REPORT(WARN, "WARNING: " << Lexer.SourcePos << ": Beware! Unquoted simple literals will be deprecated soon (token \"" << Lexer.TokenValue << "\").");
                    tokens.push_back(ParseSingleWord(collector));
                    break;
                case RLT_STRING_VALUE:
                    tokens.push_back(ParseSingleWord(collector));
                    break;
                case RLT_PIPE:
                    tokens.push_back(new NRemorph::TTokenPipe());
                    break;
                case RLT_ASTERISK:
                    tokens.push_back(new NRemorph::TTokenAsterisk());
                    break;
                case RLT_QUESTION:
                    tokens.push_back(new NRemorph::TTokenQuestion());
                    break;
                case RLT_DOT:
                    tokens.push_back(new NRemorph::TTokenLiteral(NRemorph::TLiteral(0, NRemorph::TLiteral::Any)));
                    break;
                case RLT_MARK:
                    tokens.push_back(new NRemorph::TTokenLiteral(ParseSpecialLiteralSuffix(NRemorph::TLiteral::Marker, collector)));
                    break;
                case RLT_PLUS:
                    tokens.push_back(new NRemorph::TTokenPlus());
                    break;
                case RLT_LPAREN:
                    tokens.push_back(new NRemorph::TTokenLParen(new NRemorph::TSourcePos(Lexer.SourcePos)));
                    break;
                case RLT_LPAREN_NC:
                    tokens.push_back(new NRemorph::TTokenLParen(new NRemorph::TSourcePos(Lexer.SourcePos), false));
                    break;
                case RLT_LPAREN_NAMED:
                    tokens.push_back(new NRemorph::TTokenLParen(new NRemorph::TSourcePos(Lexer.SourcePos), Lexer.TokenValue));
                    break;
                case RLT_RPAREN:
                    tokens.push_back(new NRemorph::TTokenRParen(new NRemorph::TSourcePos(Lexer.SourcePos)));
                    break;
                case RLT_REFERENCE:
                    tokens.push_back(ParseDefRef(collector));
                    break;
                case RLT_LBRACKET:
                    tokens.push_back(ParseLiteral(collector));
                    break;
                case RLT_CARET:
                    tokens.push_back(new NRemorph::TTokenLiteral(NRemorph::TLiteral(0, NRemorph::TLiteral::Bos)));
                    break;
                case RLT_DOLLAR:
                    tokens.push_back(new NRemorph::TTokenLiteral(NRemorph::TLiteral(0, NRemorph::TLiteral::Eos)));
                    break;
                default:
                    UNEXPECTED_TOKEN();
            }
        }
        // Cdbg << NRemorph::Bind(*Context.Res.LiteralTable, tokens) << Endl;
        return NRemorph::Parse(*Context.Res.LiteralTable, tokens);
    }

    // Returns true if we parsed #head suffix
    bool ParseSuffix(TVector<TAgreementPtr>& agreements) {
        Lexer.NextToken();
        bool head = false;
        while (RLT_SHARP == Lexer.Token) {
            TString agrName = Lexer.TokenValue;
            if ("head" == agrName) {
                head = true;
            } else {
                Lexer.NextToken();
                CHECK_TOKEN(RLT_LPAREN_NC);
                Lexer.NextToken();
                CHECK_TOKEN(RLT_WORD);
                TAgreementPtr agree = CreateAgreement(agrName);
                TString context = agrName + "#" + Lexer.TokenValue;
                agree->SetContext(context);
                agreements.push_back(agree);
                Lexer.NextToken();
                CHECK_TOKEN(RLT_RPAREN);
            }
            Lexer.NextToken();
        }

        Lexer.HoldCurrentToken();
        return head;
    }

    template <class TElementPtr>
    NRemorph::TLiteral ParseLiteralSuffix(TElementPtr& e, THeadCollector& collector) {
        TVector<TAgreementPtr> agreements;
        bool isHead = ParseSuffix(agreements);
        if (isHead || !agreements.empty()) {
            // Ensure unique literal if we have either head label or agreements
            e->Name.append('#').append(::ToString(++Context.NextClassId));
        }
        NRemorph::TLiteral lit = Context.Res.LiteralTable->Add(e.Get());
        if (isHead) {
            collector.SetExplicit();
            collector.Heads.insert(lit);
        }
        if (!agreements.empty()) {
            Context.Res.LiteralTable->GetAgreementGroup(lit).AddAgreements(agreements);
        }

        return lit;
    }

    NRemorph::TLiteral ParseSpecialLiteralSuffix(NRemorph::TLiteral::EType type, THeadCollector& collector) {
        TVector<TAgreementPtr> agreements;
        bool isHead = ParseSuffix(agreements);
        NRemorph::TLiteralId id = 0;
        if (isHead || !agreements.empty()) {
            // Ensure unique literal if we have either head label or agreements
            id = ++Context.NextClassId;
            if (id > NRemorph::TLiteral::MAX_ID)
                ythrow yexception() << "Too many literals";
        }
        NRemorph::TLiteral lit(id, type);
        if (isHead) {
            if (NRemorph::TLiteral::Marker == type) {
                throw yexception() << GetErrorContext() << ": #head cannot be specified for '/' literal";
            }
            collector.SetExplicit();
            collector.Heads.insert(lit);
        }
        if (!agreements.empty()) {
            Context.Res.LiteralTable->GetAgreementGroup(lit).AddAgreements(agreements);
        }

        return lit;
    }

    const TDefinition& GetDefRefValue(const TString& id) {
        TDefMap::iterator i = Context.DefMap.find(id);
        if (i == Context.DefMap.end())
            throw yexception() << GetErrorContext() << ": def id \"" << id << "\" is not defined";
        return i->second;
    }

    const TString GetStringRefValue(const TString& id) {
        TStringMap::iterator i = Context.StringMap.find(id);
        if (i == Context.StringMap.end())
            throw yexception() << GetErrorContext() << ": string id \"" << id << "\" is not defined";
        return i->second;
    }

    void CheckDefId(const TString& id) {
        if (Context.DefMap.contains(id))
            throw yexception() << GetErrorContext() << ": def id \"" << id << "\" is already defined";
    }

    void CheckLogicId(const TString& id) {
        if (Context.LogicMap.contains(id))
            throw yexception() << GetErrorContext() << ": logic id \"" << id << "\" is already defined";
    }

    void CheckStringId(const TString& id) {
        if (Context.StringMap.contains(id))
            throw yexception() << GetErrorContext() << ": string id \"" << id << "\" is already defined";
    }

    void SetExpressionId(TLTElementBase& e) {
        e.ExpId = Context.GetExpressionId(e.Name);
        if (e.Type == TLTElementBase::Logic)
            SetExpressionIds(static_cast<TLTElementLogic&>(e).Instr);
    }

    void SetExpressionIds(TLInstrVector& v) {
        for (auto& instr : v) {
            switch (instr.Type) {
#define X(A) case TLInstrBasePtr::A:
            CACHEABLE_LOGIC_EXPR_INSTR_LIST
#undef X
                if (instr.ExpId == Max<TExpressionId>())
                    instr.ExpId = Context.GetExpressionId(ToString(instr));
                break;
            default:
                break;
            }
        }
    }
};

void ParseFile(TParseResult& result, const TString& path, IInputStream& in) {
    TString dirName = GetDirName(path);
    if (dirName == path)
        dirName = ".";
    TParserContext context(result, dirName);
    TParser parser(in, GetFileNameComponent(path.data()), context);
}

void ParseStream(TParseResult& result, IInputStream& stream) {
    TParserContext context(result, ".");
    TParser parser(stream, "<inline>", context);
}

} // NPrivate

} // NReMorph

