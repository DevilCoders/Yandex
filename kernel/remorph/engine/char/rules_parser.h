#pragma once

#include "literal_table.h"

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/engine/engine.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <utility>

namespace NReMorph {
namespace NPrivate {

typedef THashMap<TString, NRemorph::TAstNodePtr> TDefinitions;

class TRulesParser {
public:
    struct TResult {
        TLiteralTablePtr LiteralTable;
        TVector<std::pair<TString, double>> Rules;
        NRemorph::TVectorNFAs NFAs;

        explicit TResult()
            : LiteralTable(new TLiteralTable())
            , Rules()
            , NFAs()
        {
        }
    };

private:
    TPreParser PreParser;

    TResult Result;
    TDefinitions Definitions;

public:
    explicit TRulesParser(const TFileParseOptions& fileParseOptions);
    explicit TRulesParser(const TStreamParseOptions& streamParseOptions);

    TResult Parse();
    void HandleStatement(const TParseStatement& statement);

private:
    void ParseRule(const TParseTokens& tokens);
    void ParseDef(const TParseTokens& tokens);
};

} // NPrivate
} // NReMorph
