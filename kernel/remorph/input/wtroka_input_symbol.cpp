#include "wtroka_input_symbol.h"

namespace NSymbol {

TWtrokaInputSymbol::TWtrokaInputSymbol(size_t pos, const TUtf16String& w)
    : TInputSymbol(pos)
{
    Text = w;
    NormalizedText = w;
    // Needed to insert spaces between symbols
    Properties.Set(PROP_SPACE_BEFORE);
}

TWtrokaInputSymbol::TWtrokaInputSymbol(TWtrokaInputSymbols::const_iterator start, TWtrokaInputSymbols::const_iterator end,
    const TVector<TDynBitMap>& contexts, size_t mainOffset)
    : TInputSymbol(start, end, contexts, mainOffset)
{
}

} // NSymbol
