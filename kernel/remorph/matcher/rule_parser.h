#pragma once

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/literal/literal_table.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>
#include <util/stream/str.h>
#include <utility>

namespace NReMorph {

namespace NPrivate {

struct TParseResult {
    NLiteral::TLiteralTablePtr LiteralTable;
    NRemorph::TVectorNFAs NFAs;
    // Vector of pairs <rule name, rule weight>
    TVector<std::pair<TString, double>> Rules;
    TVector<TUtf16String> UseGzt;
};

void ParseFile(TParseResult& result, const TString& path, IInputStream& in);

void ParseStream(TParseResult& result, IInputStream& stream);

} // NPrivate

} // NReMorph
