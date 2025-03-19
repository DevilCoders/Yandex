#pragma once

#include <kernel/qtree/request/req_node.h>
#include <kernel/qtree/richrequest/wordnode.h> // TWordJoin

namespace NRichTreeBuilder {
    class TSubWord {
    private:
        const wchar16* Word;
        const TTokenStructure& Subtokens;
        size_t FirstSubTokenNum;
        size_t LastSubTokenNum;
        TUtf16String TextBeforeWhole;
        TUtf16String TextAfterWhole;
    public:
        TSubWord(const TRequestNode& node, size_t firstSubToken, size_t lastSubToken)
            : Word (node.GetText().c_str())
            , Subtokens(node.GetSubTokens())
            , FirstSubTokenNum(firstSubToken)
            , LastSubTokenNum(lastSubToken)
            , TextBeforeWhole(node.GetTextBefore())
            , TextAfterWhole(node.GetTextAfter())
        {
            Y_ASSERT(node.GetMultitoken().Token == Word);
        }

        const TCharSpan& FirstToken() const {
            return Subtokens[FirstSubTokenNum];
        }

        const TCharSpan& LastToken() const {
            return Subtokens[LastSubTokenNum];
        }

        bool IsClosing() const {
            return LastSubTokenNum == Subtokens.size() - 1;
        }

        bool IsOpening() const {
            return FirstSubTokenNum == 0;
        }

        TUtf16String GetTextBefore() const {
            if (IsOpening()) {
                return TextBeforeWhole;
            }
            const TCharSpan& prevToken = Subtokens[FirstSubTokenNum - 1];
        //    if (PlusSuffix(prevToken))
        //      return TUtf16String();

            const size_t pos = prevToken.EndPos() + prevToken.SuffixLen;
            const size_t len = FirstToken().Pos - pos;
            return TUtf16String(Word + pos, len);
        }

        TUtf16String GetTextAfter() const {
            if (IsClosing())
                return TextAfterWhole;
            if (AppendTextAfterToText())
                return TUtf16String();
            return GetTextAfter_();
        }

        TUtf16String GetText() const {
            TUtf16String text(Word + FirstToken().Pos, GetLengthWithSuffix());
            if (AppendTextAfterToText())
                text += GetTextAfter_();
            return text;
        }

        size_t GetLengthWithSuffix() const {
            return GetLength() + LastToken().SuffixLen;
        }

        size_t GetLength() const {
            return LastToken().EndPos() - FirstToken().Pos;
        }

        // can have text: a-b-c, a1b2c3, 1.2.3
        TPhraseType GetPhraseType() const {
            if (FirstToken().TokenDelim == TOKDELIM_NULL)
                return PHRASE_MARKSEQ;
            if (FirstToken().TokenDelim == TOKDELIM_DOT)
                return PHRASE_NUMBERSEQ;
            return PHRASE_MULTIWORD;
        }

        TWordJoin GetLeftJoin() const { // 1.2d3 -> 1.2 + d3
            if (IsOpening() || Subtokens[FirstSubTokenNum - 1].TokenDelim != TOKDELIM_NULL) {
                Y_ASSERT(IsOpening() || GetTextBefore().length() != 0);
                return WORDJOIN_DEFAULT;
            }

            Y_ASSERT(GetTextBefore().length() == 0);
            return WORDJOIN_NODELIM;
        }

        TWordJoin GetRightJoin() const {  // it can be: 1.2d -> 1.2 + d
            if (IsClosing() || Subtokens[LastSubTokenNum].TokenDelim != TOKDELIM_NULL) {
                Y_ASSERT(IsClosing() || GetTextAfter().length() != 0);
                return WORDJOIN_DEFAULT;
            }

            Y_ASSERT(GetTextAfter().length() == 0);
            return WORDJOIN_NODELIM;
        }

        TTokenStructure ObtainSubTokens() const {
            const size_t base = FirstToken().Pos;
            TTokenStructure ret;
            for (size_t i = FirstSubTokenNum; i <= LastSubTokenNum; ++i) {
                ret.push_back(Subtokens[i]);
                ret.back().Pos -= base;
            }
            ret[0].PrefixLen = 0;
            return ret;
        }

        bool IsSingleToken() const {
            return FirstSubTokenNum == LastSubTokenNum;
        }

        ETokenType GetTokenType() const {
            if (IsSingleToken())
                return Subtokens[FirstSubTokenNum].Type;
            return TOKEN_MARK;
        }

    private:
        TUtf16String GetTextAfter_() const {
            if (IsClosing())
                return TUtf16String();
            const TCharSpan& nextToken = Subtokens[LastSubTokenNum + 1];

            const size_t pos = LastToken().EndPos() + LastToken().SuffixLen;
            const size_t len = nextToken.Pos - pos;
            return TUtf16String(Word + pos, len);
        }
        bool AppendTextAfterToText() const {
            // return PlusSuffix(Subtokens[LastSubTokenNum]);
            return false;
        }

        bool PlusSuffix(const TCharSpan& s) const {
            return s.TokenDelim == TOKDELIM_PLUS && s.SuffixLen == 0;
        }
    };
}
