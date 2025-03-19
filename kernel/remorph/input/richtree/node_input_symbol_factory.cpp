#include "node_input_symbol_factory.h"

namespace NReMorph {

namespace NRichNode {

namespace {

struct TNopHandler {
    Y_FORCE_INLINE void operator() (size_t) const {
    }
};

struct TStoreOffsetHandler {
    TVector<size_t>& Offsets;

    TStoreOffsetHandler(TVector<size_t>& offsets)
        : Offsets(offsets)
    {
    }

    inline void operator() (size_t off) const {
        Offsets.push_back(off);
    }
};

template <bool processDelimiters>
class TNodeInputSymbolFactory {
private:
    const TLangMask QueryLang;
    size_t Pos;
    bool SpaceBefore;
    NSorted::TSortedVector<wchar16> Delimiters;

private:
    // This function processes delimiters between words and sets SpaceBefore flag.
    inline void ProcessDelimiters(TNodeInputSymbols& res, const TUtf16String& miscText) {
        SpaceBefore = false;

        if (!processDelimiters) {
            SpaceBefore = !miscText.empty();
            return;
        }

        if (miscText.empty()) {
            return;
        }

        for (size_t s = 0; s < miscText.size(); ++s) {
            const wchar16 delim = miscText.at(s);
            if (::IsWhitespace(delim)) {
                SpaceBefore = true;
            } else if (Delimiters.empty() || Delimiters.has(delim)) {
                TNodeInputSymbolPtr symbol = new TNodeInputSymbol(Pos++, delim);
                symbol->SetQLangMask(QueryLang);
                if (SpaceBefore) {
                    symbol->GetProperties().Set(NSymbol::PROP_SPACE_BEFORE);
                    SpaceBefore = false;
                }
                res.push_back(symbol);
            } else {
                SpaceBefore = true;
            }
        }
    }

public:
    TNodeInputSymbolFactory(const TLangMask& queryLang, const NSorted::TSortedVector<wchar16>& delimiters = NSorted::TSortedVector<wchar16> ())
        : QueryLang(queryLang)
        , Pos(0)
        , SpaceBefore(false)
        , Delimiters(delimiters)
    {
    }

    template <class TOffsetHandler>
    TNodeInputSymbols CreateSymbols(const TConstNodesVector& nodes, TOffsetHandler handler) {
        TNodeInputSymbols res;

        if (nodes.empty()) {
            return res;
        }

        Pos = 0;

        ProcessDelimiters(res, nodes[0]->GetTextBefore());
        for (size_t i = 0; i < nodes.size(); ++i) {
            const TRichRequestNode* const node = nodes[i];
            TNodeInputSymbolPtr symbol = new TNodeInputSymbol(Pos++, node);
            symbol->SetQLangMask(QueryLang);
            if (SpaceBefore) {
                symbol->GetProperties().Set(NSymbol::PROP_SPACE_BEFORE);
            }
            handler(res.size());
            res.push_back(symbol);
            ProcessDelimiters(res, node->GetTextAfter());
        }
        return res;
    }
};

} // unnamed namespace

TNodeInputSymbols CreateInputSymbols(const TConstNodesVector& nodes, const TLangMask& queryLang, bool ignoreDelimiters) {
    return ignoreDelimiters
        ? TNodeInputSymbolFactory<false>(queryLang).CreateSymbols(nodes, TNopHandler())
        : TNodeInputSymbolFactory<true>(queryLang).CreateSymbols(nodes, TNopHandler());
}

TNodeInputSymbols CreateInputSymbols(const TConstNodesVector& nodes, TVector<size_t>& nodesToSymbols,
                                     const TLangMask& queryLang, const NSorted::TSortedVector<wchar16>& delimiters) {
    nodesToSymbols.clear();
    return TNodeInputSymbolFactory<true>(queryLang, delimiters).CreateSymbols(nodes, TStoreOffsetHandler(nodesToSymbols));
}

TVector<size_t> CreateSymbolsToNodesMap(const TVector<size_t>& nodesToSymbols, size_t countOfSymbols) {
    TVector<size_t> symbolsToNodes;
    if (!nodesToSymbols.empty()) {
        // Prepare the reverse mapping from position of symbol to position of node
        // Punctuation before node have the same position as the node.
        // Punctuation after node have the position of the next node.
        symbolsToNodes.assign(nodesToSymbols[0] + 1, 0);
        size_t prev = nodesToSymbols[0];
        for (size_t i = 1; i < nodesToSymbols.size(); ++i) {
            symbolsToNodes.insert(symbolsToNodes.end(), nodesToSymbols[i] - prev, i);
            prev = nodesToSymbols[i];
        }
        symbolsToNodes.insert(symbolsToNodes.end(), countOfSymbols - prev, nodesToSymbols.size());
    }
    return symbolsToNodes;
}

bool CreateInputSymbols(TNodeInputSymbols& symbols, const NMatcher::TMatcherBase& matcher,
    const TConstNodesVector& nodes, const TGztResults& gztResults, const TLangMask& queryLang,
    TGztResultFilterFunc filter, bool ignoreDelimiters) {

    if (nodes.empty())
        return false;

    TVector<size_t> offsets;
    if (ignoreDelimiters)
        symbols = TNodeInputSymbolFactory<false>(queryLang).CreateSymbols(nodes, TNopHandler());
    else
        symbols = TNodeInputSymbolFactory<true>(queryLang).CreateSymbols(nodes, TStoreOffsetHandler(offsets));
    TGztResultIter iter(gztResults, nodes, filter, offsets);
    return matcher.ApplyGztResults(symbols, iter);
}

bool CreateInput(TNodeInput& input, const NMatcher::TMatcherBase& matcher,
    const TConstNodesVector& nodes, const TGztResults& gztResults, const TLangMask& queryLang,
    TGztResultFilterFunc filter, bool ignoreDelimiters) {

    if (nodes.empty())
        return false;

    TVector<size_t> offsets;
    TNodeInputSymbols symbols = ignoreDelimiters
        ? TNodeInputSymbolFactory<false>(queryLang).CreateSymbols(nodes, TNopHandler())
        : TNodeInputSymbolFactory<true>(queryLang).CreateSymbols(nodes, TStoreOffsetHandler(offsets));
    TGztResultIter iter(gztResults, nodes, filter, offsets);
    return matcher.CreateInput(input, symbols, iter);
}

} // NRichNode

} // NReMorph
