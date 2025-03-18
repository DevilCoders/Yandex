#pragma once

#include "treedata.h"
#include "treetext.h"

#include <library/cpp/token/nlptypes.h>
#include <util/stream/output.h>

namespace NDomTree {
    static TStringPiece Span(const TStringPiece& f, const TStringPiece& l) {
        Y_ASSERT(f.Src == l.Src);
        Y_ASSERT(f.Start <= l.Start);
        return TStringPiece(f.Src, f.Start, l.Start - f.Start + l.Len);
    }

    class TChunkTraverser: public ITextChunkTraverser {
    private:
        const ITreeDataAccessor& TreeData;
        ui32 FToken;
        const ui32 LToken;

    public:
        TChunkTraverser(const ITreeDataAccessor& treeData, const ui32 ftoken, const ui32 ltoken)
            : TreeData(treeData)
            , FToken(ftoken)
            , LToken(ltoken)
        {
            Y_ASSERT(ftoken <= ltoken);
            Y_ASSERT(ltoken <= TreeData.TokenCount());
        }

        NStlIterator::TProxy<TTextChunk> Next() override {
            if (FToken >= LToken) {
                return TTextChunk();
            }
            const TStringPiece& first = TreeData.TokenData(FToken).TokenText;
            while (FToken < LToken && first.Src == TreeData.TokenData(FToken).TokenText.Src) {
                FToken++;
            }
            const TStringPiece& last = TreeData.TokenData(FToken - 1).TokenText;
            return TTextChunk(TreeData.WText(Span(first, last)), last.Src == TStringPiece::SPS_NOINDEX);
        }
    };

    class TSentTraverser: public ISentTextTraverser {
    private:
        const ITreeDataAccessor& TreeData;
        mutable ui32 FToken;
        const ui32 LToken;

        bool Check() const {
            if (FToken >= LToken) {
                return false;
            }

            const TTokenData& td = TreeData.TokenData(FToken);
            if (td.BreakType & ST_PARABRK) {
                return false;
            }
            if (FToken < LToken) {
                const TTokenData& next = TreeData.TokenData(FToken + 1);
                if (next.TokenText.Src != TStringPiece::SPS_TEXT) {
                    return false;
                }
            }
            return true;
        }

        void Move() const {
            while (FToken <= LToken && FToken < TreeData.TokenCount() &&
                   TreeData.TokenData(FToken).TokenText.Src == TStringPiece::SPS_NOINDEX) {
                FToken++;
            }
            while (FToken <= LToken && FToken < TreeData.TokenCount() &&
                   TreeData.TokenData(FToken).TokenMode == TTokenData::TM_SPACES_EMPTY) {
                FToken++;
            }
            if (FToken < LToken && FToken + 1 == TreeData.TokenCount()) {
                FToken++;
            }
        }

    public:
        TSentTraverser(const ITreeDataAccessor& treeData, const ui32 ftoken, const ui32 ltoken)
            : TreeData(treeData)
            , FToken(ftoken)
            , LToken(ltoken)
        {
            Y_ASSERT(ftoken <= ltoken);
            Y_ASSERT(ltoken <= TreeData.TokenCount());
        }

        NStlIterator::TProxy<TSentChunk> Next() override {
            Move();
            if (FToken >= LToken) {
                return TSentChunk();
            }

            const TTokenData& first = TreeData.TokenData(FToken);
            while (Check()) {
                FToken++;
            }
            const TTokenData& last = TreeData.TokenData(FToken);
            FToken++;
            Move();
            return TSentChunk(TreeData.WText(Span(first.TokenText, last.TokenText)), last.BreakType == ST_PARABRK);
        }
    };

    template <class Iter>
    class TTokenTraverser: public ITokenTraverser {
    private:
        const ITreeDataAccessor& TreeData;
        Iter FToken;
        const Iter LToken;
        const bool UseNoindex;

        void SkipNoindex() {
            if (!UseNoindex) {
                while (FToken < LToken && FToken->TokenText.Src == TStringPiece::SPS_NOINDEX) {
                    FToken++;
                }
            }
        }

        void Move() {
            FToken++;
            SkipNoindex();
        }

    public:
        TTokenTraverser(const ITreeDataAccessor& treeData, const Iter ftoken, const Iter ltoken, const bool noindex)
            : TreeData(treeData)
            , FToken(ftoken)
            , LToken(ltoken)
            , UseNoindex(noindex)
        {
        }

        NStlIterator::TProxy<TTokenChunk> Next() override {
            SkipNoindex();
            if (FToken >= LToken) {
                return TTokenChunk();
            }
            const TTokenData& td = *FToken;
            TTokenChunk res(TreeData.WText(td.TokenText));
            res.Parent = TreeData.GetNode(td.Parent);
            if (td.BreakType == ST_SENTBRK) {
                res.SentBreak = true;
            } else if (td.BreakType == ST_PARABRK) {
                res.SentBreak = true;
                res.ParaBreak = true;
            }
            res.Empty = td.TokenMode == TTokenData::TM_SPACES_EMPTY;
            res.TokenType = static_cast<ETokenType>(td.TokenType);
            res.MultiTokenStart = td.TokenMode == TTokenData::TM_MULTI_START;
            res.MultiTokenBody = td.TokenMode == TTokenData::TM_MULTI_BODY || td.TokenMode == TTokenData::TM_MULTI_DELIM;
            res.Noindex = td.TokenMode == TTokenData::TM_UNTOK;
            res.Pos = td.Pos;
            Move();
            return res;
        }
    };

    static TTextChunkIterator ChunkIterator(const ITreeDataAccessor& treeData, const ui32 ftoken, const ui32 ltoken) {
        using TChunkTraverserPtr = TSimpleSharedPtr<ITextChunkTraverser>;
        return TTextChunkIterator(TChunkTraverserPtr(new TChunkTraverser(treeData, ftoken, ltoken)));
    }

    static TSentTextIterator SentIterator(const ITreeDataAccessor& treeData, const ui32 ftoken, const ui32 ltoken) {
        using TSentTraverserPtr = TSimpleSharedPtr<ISentTextTraverser>;
        return TSentTextIterator(TSentTraverserPtr(new TSentTraverser(treeData, ftoken, ltoken)));
    }

    template <class Iter>
    static TTokenIterator TokenIterator(const ITreeDataAccessor& treeData, const Iter ftoken, const Iter ltoken, bool noindex) {
        using TTokenTraverserPtr = TSimpleSharedPtr<ITokenTraverser>;
        return TTokenIterator(TTokenTraverserPtr(new TTokenTraverser<Iter>(treeData, ftoken, ltoken, noindex)));
    }

    static TWtringBuf RawText(const ITreeDataAccessor& treeData, ui32 ftoken, ui32 ltoken) {
        Y_ASSERT(ftoken <= ltoken);
        Y_ASSERT(ltoken <= treeData.TokenCount());

        while (ftoken < ltoken && treeData.TokenData(ftoken).TokenText.Src != TStringPiece::SPS_TEXT) {
            ftoken++;
        }
        while (ftoken < ltoken && treeData.TokenData(ltoken - 1).TokenText.Src != TStringPiece::SPS_TEXT) {
            ltoken--;
        }
        if (ftoken == ltoken) {
            return TWtringBuf();
        }

        const TTokenData& fdata = treeData.TokenData(ftoken);
        const TTokenData& ldata = treeData.TokenData(ltoken - 1);
        return treeData.WText(Span(fdata.TokenText, ldata.TokenText));
    }

}
