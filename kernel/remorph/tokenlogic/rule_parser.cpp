#include "lexer.h"
#include "rule_parser.h"
#include "parser_defs.h"
#include "parser.h"

#include <kernel/remorph/common/verbose.h>
#include <kernel/remorph/literal/logic_expr.h>
#include <kernel/remorph/literal/parser.h>

#include <util/folder/dirut.h>

namespace NTokenLogic {

namespace NPrivate {

TFileContext::TFileContext(IInputStream& input, const TString& file, TParseResult& res, TParseContext& ctx)
    : Lexer(input, file)
    , Dir(GetDirName(file))
    , Res(res)
    , Ctx(ctx)
{
}

NLiteral::TExpressionId TFileContext::GetExpressionId(const TString& exp) {
    TExpressionIdMap::iterator i = Ctx.ExpressionIds.find(exp);
    if (i == Ctx.ExpressionIds.end()) {
        return Ctx.ExpressionIds.insert(std::make_pair(exp, static_cast<NLiteral::TExpressionId>(Ctx.ExpressionIds.size()))).first->second;
    }
    return i->second;
}

void TFileContext::SetExpressionId(NLiteral::TLTElementBase& e) {
    e.ExpId = GetExpressionId(e.Name);
    if (e.Type == NLiteral::TLTElementBase::Logic)
        SetExpressionIds(static_cast<NLiteral::TLTElementLogic&>(e).Instr);
}

void TFileContext::SetExpressionIds(NLiteral::TLInstrVector& v) {
    for (auto& instr : v) {
        switch (instr.Type) {
        case NLiteral::TLInstrBasePtr::Gzt:
        case NLiteral::TLInstrBasePtr::GztAttr:
        case NLiteral::TLInstrBasePtr::Gram:
        case NLiteral::TLInstrBasePtr::TextRE:
        case NLiteral::TLInstrBasePtr::NTextRE:
            if (instr.ExpId == Max<NLiteral::TExpressionId>())
                instr.ExpId = GetExpressionId(ToString(instr));
            break;
        default:
            break;
        }
    }
}

NLiteral::TLTElementPtr TFileContext::ParseLiteral(const TString& literalExp) {
    NLiteral::TLTElementPtr e = NLiteral::ParseLiteral(literalExp, Dir, THashMap<TString, NLiteral::TLInstrVector>());
    SetExpressionId(*e);
    return e;
}

NLiteral::TLTElementPtr TFileContext::ParseSingleWord(const TString& word) {
    NLiteral::TLTElementSinglePtr e = NLiteral::CreateWordLiteral(word);
    SetExpressionId(*e);
    return e.Get();
}

NLiteral::TLTElementPtr TFileContext::ParseSingleWordUnquoted(const TString& word) {
    REPORT(WARN, "WARNING: " << Lexer.GetContext() << ": Beware! Unquoted simple literals will be deprecated soon (token \"" << word << "\").");
    return ParseSingleWord(word);
}

bool TFileContext::CheckAvailable(const TString& file) {
    TString fullDir = Dir + GetDirectorySeparator() + file;
    return NFs::Exists(fullDir);
}

bool TFileContext::ParseFile(const TString& file) {
    TString fullFile = Dir + GetDirectorySeparator() + file;
    TIFStream stream(fullFile);
    TFileContext includeCtx(stream, fullFile, Res, Ctx);
    return !ParseRulesImpl(&includeCtx);
}


TParseResult ParseRules(const TString& fileName, IInputStream& stream) {
    TParseResult result;
    TParseContext parseCtx;
    TFileContext fileCtx(stream, fileName, result, parseCtx);
    if (ParseRulesImpl(&fileCtx))
        throw TSyntaxError() << parseCtx.Error;
    return result;
}

} // NPrivate

} // NTokenLogic

