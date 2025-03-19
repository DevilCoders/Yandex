#pragma once

#include "input_symbol.h"
#include "gazetteer_input.h"

#include <kernel/remorph/core/core.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/generic/yexception.h>

namespace NSymbol {

class TWtrokaInputSymbol;
typedef TIntrusivePtr<TWtrokaInputSymbol> TWtrokaInputSymbolPtr;
typedef TVector<TWtrokaInputSymbolPtr> TWtrokaInputSymbols;
typedef NRemorph::TInput<TWtrokaInputSymbolPtr> TWtrokaInput;

class TWtrokaInputSymbol: public TInputSymbol {
public:
    TWtrokaInputSymbol(TWtrokaInputSymbols::const_iterator start, TWtrokaInputSymbols::const_iterator end,
        const TVector<TDynBitMap>& contexts, size_t mainOffset);

    TWtrokaInputSymbol(size_t pos, const TUtf16String& w);
};

} // NSymbol

DECLARE_GAZETTEER_SUPPORT(NSymbol::TWtrokaInputSymbols, TWtrokaSymbolsIterator)
