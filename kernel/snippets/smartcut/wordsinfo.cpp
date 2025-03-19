#include "wordsinfo.h"
#include "char_class.h"
#include "snip_length.h"

#include <library/cpp/token/token_structure.h>
#include <library/cpp/tokenizer/tokenizer.h>

#include <util/charset/unidata.h>
#include <util/system/yassert.h>

namespace NSnippets
{
    namespace
    {
        class TTokenHandler : public ITokenHandler {
        private:
            TVector<TWordsInfo::TWordNode>& Words;
            TUtf16String& Text;
            bool NewSent = true;
            size_t NewSentOffset = 0;
            const wchar16* const HilightMark = nullptr;

        public:
            TTokenHandler(TWordsInfo& wordsInfo, const wchar16* const hilightMark)
                : Words(wordsInfo.Words)
                , Text(wordsInfo.Text)
                , HilightMark(hilightMark)
            {
            }

            void OnSentBreak(TWtringBuf token) {
                Text.append(token);
                if (!NewSent) {
                    NewSent = true;
                    NewSentOffset = Text.length();
                    if (Words) {
                        TWordsInfo::TWordNode& word = Words.back();
                        word.LastInSent = true;
                        word.TextBufEnd = Text.length();
                        if (Text) {
                            while (word.TextBufEnd != word.TextBufBegin + 1
                                   && (IsSpace(Text[word.TextBufEnd - 1]) || (HilightMark && Text[word.TextBufEnd - 1]== *HilightMark)))
                            {
                                --word.TextBufEnd;
                            }
                        }
                    }
                }
            }

            void OnWord(TWtringBuf token) {
                Words.emplace_back();
                TWordsInfo::TWordNode& word = Words.back();
                word.WordBegin = Text.length();
                Text.append(token);
                word.WordEnd = Text.length();
                word.FirstInSent = NewSent;

                size_t beg = NewSent ? NewSentOffset : word.WordBegin;
                if (!NewSent && beg > 0 && IsLeftQuoteOrBracket(Text[beg - 1])) {
                    --beg;
                    if (beg > 0 && HilightMark && Text[beg - 1] == *HilightMark) {
                        --beg;
                    }
                }

                word.TextBufBegin = beg;
                word.TextBufEnd = Text.length();

                if (!NewSent && Words.size() >= 2) {
                    TWordsInfo::TWordNode& prevWord = Words[Words.size() - 2];
                    size_t end = prevWord.WordEnd;

                    if (end + 1 < Text.size() && HilightMark && Text[end] == *HilightMark) {
                        if (end + 2 < Text.size() && Text[end + 1] == ']')
                            end += 2;
                    } else {
                        if (end + 1 < Text.size() && IsRightQuoteOrBracket(Text[end])) {
                            ++end;
                        }
                    }

                    prevWord.TextBufEnd = end;
                }

                NewSent = false;
            }

            void OnMiscText(TWtringBuf token) {
                Text.append(token);
            }

            void OnToken(const TWideToken& token, size_t /*origleng*/, NLP_TYPE type) override {
                if (type == NLP_SENTBREAK || type == NLP_PARABREAK || type == NLP_END) {
                    OnSentBreak(token.Text());
                } else if (type == NLP_WORD || type == NLP_MARK || type == NLP_INTEGER || type == NLP_FLOAT) {
                    OnWord(token.Text());
                } else {
                    OnMiscText(token.Text());
                }
            }
        };
    } // anonymouos namespace

    TWordsInfo::TWordsInfo(const TUtf16String& source, const wchar16* const hilightMark) {
        TTokenHandler tokenHandler(*this, hilightMark);
        TNlpTokenizer tokenizer(tokenHandler, false);
        tokenizer.Tokenize(source.data(), source.size());
        tokenHandler.OnSentBreak(TWtringBuf());
    }

    int TWordsInfo::WordCount() const {
        return Words.ysize();
    }

    TWtringBuf TWordsInfo::GetWordBuf(int wordId) const {
        return TWtringBuf(Text.data() + Words[wordId].WordBegin, Text.data() + Words[wordId].WordEnd);
    }

    TWtringBuf TWordsInfo::GetPunctAfter(int wordId) const {
        Y_ASSERT(wordId + 1 < Words.ysize());
        return TWtringBuf(Text.data() + Words[wordId].WordEnd, Text.data() + Words[wordId + 1].WordBegin);
    }

    TWtringBuf TWordsInfo::GetWordWithPunctAfter(int wordId) const {
        Y_ASSERT(wordId + 1 < Words.ysize());
        return TWtringBuf(Text.data() + Words[wordId].WordBegin, Text.data() + Words[wordId + 1].WordBegin);
    }

    TWtringBuf TWordsInfo::GetTextBuf(int firstWordId, int lastWordId) const {
        return TWtringBuf(Text.data() + Words[firstWordId].TextBufBegin, Text.data() + Words[lastWordId].TextBufEnd);
    }

    TTextFragment TWordsInfo::GetTextFragment(int firstWordId, int lastWordId) const {
        TTextFragment fragment;
        fragment.TextBeginOfs = Words[firstWordId].TextBufBegin;
        fragment.PrependEllipsis = !Words[firstWordId].FirstInSent;
        fragment.TextEndOfs = Words[lastWordId].TextBufEnd;
        fragment.AppendEllipsis = !Words[lastWordId].LastInSent;
        return fragment;
    }

} // namespace NSnippets
