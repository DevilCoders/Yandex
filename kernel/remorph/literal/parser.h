#pragma once

#include "ltelement.h"
#include "logic_expr.h"

#include <kernel/remorph/core/core.h>

#include <kernel/lemmer/core/lemmer.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

namespace NLiteral {

class TLiteralParseError: public yexception {
public:
    NRemorph::TSourcePos Ctx;
public:
    TLiteralParseError(const NRemorph::TSourcePos& ctx)
        : Ctx(ctx)
    {
    }
};

TLInstrVector ParseLogic(const TString& str, const THashMap<TString, TLInstrVector>& defs);
TLTElementPtr ParseLiteral(const TString& str, const TString& ctxDir, const THashMap<TString, TLInstrVector>& defs);

} // NLiteral
