#pragma once

#include "token_expr.h"
#include "rule_parser.h"
#include "lexer.h"

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/literal/agreement.h>

#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <utility>
#include <library/cpp/containers/sorted_vector/sorted_vector.h>

namespace NTokenLogic {

namespace NPrivate {

typedef std::pair<NRemorph::TLiteral, NSorted::TSortedVector<TString>> TNamedLiteral;
typedef THashMap<TString, TNamedLiteral> TTokenMap;
typedef THashMap<TString, NLiteral::TExpressionId> TExpressionIdMap;

struct TExpPart : public TSimpleRefCount<TExpPart> {
    TTokenExpVector Part;
};
typedef TIntrusivePtr<TExpPart> TExpPartPtr;

struct TParseContext {
    TTokenMap TokenMap;
    size_t NextClassId;
    TExpressionIdMap ExpressionIds;
    TString Error;

    TVector<TExpPartPtr> ExpPartCache;
    TVector<NLiteral::TAgreementPtr> CurrentAgreements;
    NLiteral::TLTElementPtr CurrentLTElement;
    NRemorph::TLiteral CurrentLiteral;
    NSorted::TSortedVector<TString> CurrentLabels;
    bool HeavyFlag;

    TParseContext()
        : NextClassId(0)
        , HeavyFlag(false)
    {
    }

    TExpPart* AllocExpPart() {
        ExpPartCache.push_back(new TExpPart());
        return ExpPartCache.back().Get();
    }

    void ClearExpPartCache() {
        ExpPartCache.clear();
    }
};

struct TFileContext {
    TLexer Lexer;
    TString Dir;
    NTokenLogic::TTokenExpVector DefaultExp;
    TParseResult& Res;
    TParseContext& Ctx;

    TFileContext(IInputStream& input, const TString& file, TParseResult& res, TParseContext& ctx);

    NLiteral::TExpressionId GetExpressionId(const TString& exp);
    void SetExpressionId(NLiteral::TLTElementBase& e);
    void SetExpressionIds(NLiteral::TLInstrVector& v);

    NLiteral::TLTElementPtr ParseLiteral(const TString& literalExp);
    NLiteral::TLTElementPtr ParseSingleWord(const TString& word);
    NLiteral::TLTElementPtr ParseSingleWordUnquoted(const TString& word);

    bool CheckAvailable(const TString& file);
    bool ParseFile(const TString& file);
};

} // NPrivate

} // NReMorph

int ParseRulesImpl(NTokenLogic::NPrivate::TFileContext*);
