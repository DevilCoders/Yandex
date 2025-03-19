#pragma once

#include "input_symbol.h"

#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NSymbol {

struct TInputSymbolRepr {
    static void Label(IOutputStream& output, const TInputSymbol& symbol);
    static void DotPayload(IOutputStream& output, const TInputSymbol& symbol, const TString& symbolId);
    static void DotAttrsPayload(IOutputStream& output, const TInputSymbol& symbol, const TString& symbolId);
};

struct TInputSymbolPtrRepr {
    template <class TSymbolPtr>
    static inline void Label(IOutputStream& output, const TSymbolPtr& symbolPtr) {
        if (!symbolPtr.Get()) {
            return;
        }
        TInputSymbolRepr::Label(output, *symbolPtr);
    }

    template <class TSymbolPtr>
    static void DotPayload(IOutputStream& output, const TSymbolPtr& symbolPtr, const TString& symbolId) {
        if (!symbolPtr.Get()) {
            return;
        }
        TInputSymbolRepr::DotPayload(output, *symbolPtr, symbolId);
    }

    template <class TSymbolPtr>
    static void DotAttrsPayload(IOutputStream& output, const TSymbolPtr& symbolPtr, const TString& symbolId) {
        if (!symbolPtr.Get()) {
            return;
        }
        TInputSymbolRepr::DotAttrsPayload(output, *symbolPtr, symbolId);
    }
};

}
