#pragma once

#include "rule.h"

#include <kernel/remorph/literal/literal_table.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>
#include <util/stream/str.h>
#include <utility>

namespace NTokenLogic {

namespace NPrivate {

class TSyntaxError: public yexception {
};

struct TParseResult {
    NLiteral::TLiteralTablePtr LiteralTable;
    TVector<TRule> Rules;
    TVector<TString> UseGzt;

    TParseResult()
        : LiteralTable(new NLiteral::TLiteralTable())
    {
    }
};

TParseResult ParseRules(const TString& fileName, IInputStream& stream);

inline TParseResult ParseRules(const TString& rules) {
    TStringInput stream(rules);
    return ParseRules(TString("<inline>"), stream);
}

} // NPrivate

} // NReMorph
