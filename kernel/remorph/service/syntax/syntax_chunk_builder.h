#pragma once

#include "syntax_chunk.h"

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/input/input_symbol.h>
#include <kernel/remorph/matcher/match_result.h>

#include <library/cpp/solve_ambig/solve_ambiguity.h>
#include <library/cpp/solve_ambig/rank.h>
#include <library/cpp/enumbitset/enumbitset.h>

#include <util/generic/algorithm.h>
#include <util/generic/utility.h>

namespace NReMorph {

class TSyntaxChunkBuilder {
private:
    enum EFlags {
        FlagSingleWordPhrases,
        FlagSetHeads,

        FlagMax
    };
    typedef TEnumBitSet<EFlags, FlagSingleWordPhrases, FlagMax> TFlags;

private:
    TFlags Flags;
    NSolveAmbig::TRankMethod RankMethod;

private:
    // Order excluded chunks by length (from small to large) and then by right border (from back to front)
    static inline bool CompareChunkLength(const TSyntaxChunk& l, const TSyntaxChunk& r) {
        return l.Pos.second - l.Pos.first == r.Pos.second - r.Pos.first
            ? l.Pos.second > r.Pos.second
            : l.Pos.second - l.Pos.first < r.Pos.second - r.Pos.first;
    }

    void AddSubChunks(const NSymbol::TInputSymbols& symbols, TSyntaxChunks& chunks) const;

public:
    TSyntaxChunkBuilder()
        : Flags()
        , RankMethod(NSolveAmbig::DefaultRankMethod())
    {
    }

    TSyntaxChunkBuilder& SingleWordPhrases(bool flag = true) {
        Flags.Set(FlagSingleWordPhrases, flag);
        return *this;
    }

    TSyntaxChunkBuilder& SetHeads(bool flag = true) {
        Flags.Set(FlagSetHeads, flag);
        return *this;
    }

    template <class TInputSource>
    TSyntaxChunks Build(const NReMorph::TMatchResult& matchRes, const TInputSource& inputSource) const {
        TSyntaxChunks chunks;

        // Find named sub-matches
        for (NRemorph::TNamedSubmatches::const_iterator iName = matchRes.Result->NamedSubmatches.begin();
            iName != matchRes.Result->NamedSubmatches.end(); ++iName) {
            if (iName->first != "head") {
                NRemorph::TSubmatch pos = matchRes.SubmatchToOriginal(inputSource, iName->second);
                if (Flags.Test(FlagSingleWordPhrases) || pos.Size() > 1) {
                    const double weight = matchRes.GetWeight() * double(matchRes.WholeExpr.Size()) / double(pos.Size());
                    TSyntaxChunk::AddChunk(chunks, TSyntaxChunk(pos, weight, iName->first));
                }
            }
        }
        if (Flags.Test(FlagSetHeads) && matchRes.Result->NamedSubmatches.has("head")) {
            std::pair<NRemorph::TNamedSubmatches::const_iterator, NRemorph::TNamedSubmatches::const_iterator> range = matchRes.Result->NamedSubmatches.equal_range("head");
            for (NRemorph::TNamedSubmatches::const_iterator iName = range.first; iName != range.second; ++iName) {
                TSyntaxChunk::SetHead(chunks, matchRes.SubmatchToOriginal(inputSource, iName->second));
            }
        }
        // Traverse sub-chunks
        NSymbol::TInputSymbols children;
        matchRes.ExtractMatched(inputSource, children);
        AddSubChunks(children, chunks);
        return chunks;
    }

    template <class TInputSource>
    inline bool Build(const NReMorph::TMatchResults& results, const TInputSource& inputSource,
        TSyntaxChunks& chunks, TSyntaxChunks& excluded) const {

        chunks.clear();
        excluded.clear();

        NSymbol::TInputSymbols children;
        // Process remorph results
        for (NReMorph::TMatchResults::const_iterator iRes = results.begin(); iRes != results.end(); ++iRes) {
            TSyntaxChunks resChunks = Build(**iRes, inputSource);
            const size_t offset = chunks.size();
            chunks.resize(offset + resChunks.size());
            for (size_t i = 0; i < resChunks.size(); ++i) {
                chunks[i + offset].Swap(resChunks[i]);
            }
        }

        if (chunks.empty())
            return false;

        TSyntaxChunks conflicted;
        NSolveAmbig::SolveAmbiguity(chunks, conflicted, RankMethod);
        ::StableSort(chunks.begin(), chunks.end());

        // Try to add excluded chunks as children
        ::StableSort(conflicted.begin(), conflicted.end(), CompareChunkLength);
        for (size_t i = 0; i < conflicted.size(); ++i) {
            if (!TSyntaxChunk::AddChunk(chunks, conflicted[i])) {
                excluded.push_back(std::move(conflicted[i]));
            }
        }

        return !chunks.empty();
    }
};

} // NReMorph
