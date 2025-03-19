#include "dt_factory.h"
#include "ring_buffer.h"

#include <util/charset/unidata.h>
#include <util/generic/vector.h>
#include <util/system/defaults.h>
#include <utility>

namespace NDT {

using namespace NIndexerCore;

static const wchar16 DOLLAR = '$';

TDTInputSymbolPtr TDTInpuSymbolFactory::NewSymbol() {
    return new TDTInputSymbol();
}

// If count of non-space punctuation symbols exceeds the MaxPunctCount limit,
// then only first MaxPunctCount and last MaxPunctCount puncts will be used
bool TDTInpuSymbolFactory::ProcessPunctuation(TDTInputSymbols& res, const TDirectTextEntry2& entry, const TLangMask& langs) {
    bool spaceBefore = false;
    TRingBuffer<std::pair<wchar16, bool>> tail(MaxPunctCount);
    size_t punctCount = 0;
    for (size_t spaceIndex = 0; spaceIndex < entry.SpaceCount; ++spaceIndex) {
        const TDirectTextSpace& space = entry.Spaces[spaceIndex];
        for (ui32 s = 0; s < space.Length; ++s) {
            if (!IsSpace(space.Space[s])) {
                if (Y_LIKELY(punctCount < MaxPunctCount)) {
                    TDTInputSymbolPtr symbol = NewSymbol();
                    symbol->AssignPunct(res.size(), space.Space[s], entry.Posting);
                    symbol->SetQLangMask(langs);
                    if (spaceBefore) {
                        symbol->GetProperties().Set(NSymbol::PROP_SPACE_BEFORE);
                    }
                    res.push_back(symbol);
                } else {
                    tail.Put(std::make_pair(space.Space[s], spaceBefore));
                }
                spaceBefore = false;
                ++punctCount;
            } else {
                spaceBefore = true;
            }
        }
    }
    if (Y_UNLIKELY(!tail.Empty())) {
        for (size_t i = 0; i < tail.Size(); ++i) {
            TDTInputSymbolPtr symbol = NewSymbol();
            symbol->AssignPunct(res.size(), tail.At(i).first, entry.Posting);
            symbol->SetQLangMask(langs);
            if (tail.At(i).second) {
                symbol->GetProperties().Set(NSymbol::PROP_SPACE_BEFORE);
            }
            res.push_back(symbol);
        }
    }
    return spaceBefore;
}

void TDTInpuSymbolFactory::CreateSymbols(TDTInputSymbols& res, const TDirectTextEntry2* entries,
    size_t count, const TLangMask& langs) {
    bool spaceBefore = false;

    for (size_t i = 0; i < count; ++i) {
        const TDirectTextEntry2& entry = entries[i];
        if (entry.Token) {
            // Handle special case when $ appears before number. Create a separate token for $.
            const wchar16* text = entry.Token.data();
            if (DOLLAR == *text && text[1] != 0 && ::IsDigit(text[1])) {
                TDTInputSymbolPtr symbol = NewSymbol();
                symbol->AssignPunct(res.size(), *text, entry.Posting);
                symbol->SetQLangMask(langs);
                if (spaceBefore) {
                    symbol->GetProperties().Set(NSymbol::PROP_SPACE_BEFORE);
                    spaceBefore = false;
                }
                res.push_back(symbol);
                ++text;
            }
            TDTInputSymbolPtr symbol = NewSymbol();
            symbol->AssignWord(res.size(), entry, langs, text);
            symbol->SetQLangMask(langs);
            if (spaceBefore) {
                symbol->GetProperties().Set(NSymbol::PROP_SPACE_BEFORE);
            }
            res.push_back(symbol);
            spaceBefore = false;
        }

        spaceBefore = ProcessPunctuation(res, entry, langs);
    }
}

TDTInputSymbolPtr TDTInpuSymbolCacheFactory::NewSymbol() {
    // Don't cache symbols if we exceed the limit
    if (CacheOffset >= MaxCacheSize)
        return TDTInpuSymbolFactory::NewSymbol();

    // Skip entries which are still used
    while (CacheOffset < Cache.size() && Cache[CacheOffset]->RefCount() > 1) {
        ++CacheOffset;
    }

    while (CacheOffset >= Cache.size()) {
        Cache.push_back(TDTInpuSymbolFactory::NewSymbol());
    }
    return Cache[CacheOffset++];
}

void TDTInpuSymbolCacheFactory::CreateSymbols(TDTInputSymbols& res,
    const TDirectTextEntry2* entries, size_t count, const TLangMask& langs) {

    CacheOffset = 0;
    TDTInpuSymbolFactory::CreateSymbols(res, entries, count, langs);
}

} // NDT



