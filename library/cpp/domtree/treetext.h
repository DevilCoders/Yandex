#pragma once

#include "iterators.h"

#include <library/cpp/token/token_structure.h>
#include <util/generic/strbuf.h>

namespace NDomTree {
    template <class T>
    struct TChunkBase {
        TWtringBuf Text;

        TChunkBase(const TWtringBuf& buf = TWtringBuf())
            : Text(buf)
        {
        }

        bool operator==(const TChunkBase& other) const {
            if (this == &other) {
                return true;
            }
            return Text == other.Text;
        }
    };

    struct TTextChunk: public TChunkBase<TTextChunk> {
        bool Noindex;

        TTextChunk(const TWtringBuf& buf = TWtringBuf(), bool noindex = false)
            : TChunkBase(buf)
            , Noindex(noindex)
        {
        }

        bool operator==(const TTextChunk& other) const {
            return TChunkBase::operator==(other) && Noindex == other.Noindex;
        }
    };

    struct TSentChunk: public TChunkBase<TSentChunk> {
        bool ParaBreak;

        TSentChunk(const TWtringBuf& buf = TWtringBuf(), bool parabreak = false)
            : TChunkBase(buf)
            , ParaBreak(parabreak)
        {
        }

        bool operator==(const TSentChunk& other) const {
            return TChunkBase::operator==(other) && ParaBreak == other.ParaBreak;
        }
    };

    struct TTokenChunk: public TChunkBase<TTokenChunk> {
        bool SentBreak = false;
        bool ParaBreak = false;
        bool Empty = false;
        ETokenType TokenType = TOKEN_MIXED;
        bool MultiTokenStart = false;
        bool MultiTokenBody = false;
        bool Noindex = false;
        const IDomNode* Parent = nullptr;
        TPosting Pos = 0;

        TTokenChunk(const TWtringBuf& buf = TWtringBuf())
            : TChunkBase(buf)
        {
        }

        bool operator==(const TTokenChunk& other) const {
            return TChunkBase::operator==(other) && SentBreak == other.SentBreak && ParaBreak == other.ParaBreak && Empty == other.Empty && TokenType == other.TokenType && MultiTokenStart == other.MultiTokenStart && MultiTokenBody == other.MultiTokenBody && Noindex == other.Noindex && Parent == other.Parent;
        }
    };

    using ITextChunkTraverser = IAbstractTraverser<NStlIterator::TProxy<TTextChunk>>;
    using TTextChunkIterator = TDomIterator<ITextChunkTraverser>;

    using ISentTextTraverser = IAbstractTraverser<NStlIterator::TProxy<TSentChunk>>;
    using TSentTextIterator = TDomIterator<ISentTextTraverser>;

    using ITokenTraverser = IAbstractTraverser<NStlIterator::TProxy<TTokenChunk>>;
    using TTokenIterator = TDomIterator<ITokenTraverser>;

    struct INodeText {
        virtual TWtringBuf RawTextNormal() const = 0;
        virtual TTextChunkIterator RawTextAll() const = 0;
        // next method temporarily not working
        virtual TSentTextIterator Sents() const = 0;
        virtual TTokenIterator Tokens(bool noindex = false) const = 0;
        virtual TTokenIterator RTokens(bool noindex) const = 0;
        virtual ~INodeText() = default;
    };

}
