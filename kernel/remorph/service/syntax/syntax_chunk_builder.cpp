#include "syntax_chunk_builder.h"

#include <util/generic/ylimits.h>

namespace NReMorph {

void TSyntaxChunkBuilder::AddSubChunks(const NSymbol::TInputSymbols& symbols, TSyntaxChunks& chunks) const {
    NSymbol::TInputSymbols sub;
    for (size_t i = 0; i < symbols.size(); ++i) {
        const NSymbol::TInputSymbol& s = *symbols[i];
        if (!s.GetChildren().empty()) {
            for (NRemorph::TNamedSubmatches::const_iterator iName = s.GetNamedSubRanges().begin(); iName != s.GetNamedSubRanges().end(); ++iName) {
                if (iName->first != "head") {
                    size_t start = s.GetChildren()[iName->second.first]->GetSourcePos().first;
                    size_t end = s.GetChildren()[iName->second.second - 1]->GetSourcePos().second;
                    if (Flags.Test(FlagSingleWordPhrases) || end - start > 1) {
                        TSyntaxChunk::AddChunk(chunks, TSyntaxChunk(std::make_pair(start, end), 0.0, iName->first));
                    }
                }
            }
            if (Flags.Test(FlagSetHeads) && s.GetNamedSubRanges().has("head")) {
                std::pair<NRemorph::TNamedSubmatches::const_iterator, NRemorph::TNamedSubmatches::const_iterator> range = s.GetNamedSubRanges().equal_range("head");
                for (NRemorph::TNamedSubmatches::const_iterator iName = range.first; iName != range.second; ++iName) {
                    size_t start = s.GetChildren()[iName->second.first]->GetSourcePos().first;
                    size_t end = s.GetChildren()[iName->second.second - 1]->GetSourcePos().second;
                    TSyntaxChunk::SetHead(chunks, std::make_pair(start, end), s.GetSourcePos());
                }
            }
            AddSubChunks(s.GetChildren(), chunks);
        }
    }
}

} // NReMorph
