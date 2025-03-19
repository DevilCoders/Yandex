#include "is_definition.h"
#include "sent_match.h"
#include "retained_info.h"

#include <kernel/snippets/sent_info/sent_info.h>

#include <util/charset/unidata.h>
#include <util/charset/wide.h>

namespace {

    bool IsSkipablePunct(wchar32 ch) {
        if (ch == '(' || ch == ')' || IsDash(ch)) {
            return false;
        }
        return IsPunct(ch);
    }

    bool IsBeginOfDefinition(size_t ind, const TWtringBuf& text) {
        if (ind >= text.size()) {
            return false;
        }
        return IsDash(text[ind]);
    }

    class TDefinitionChecker {
    private:
        size_t CharIndex = 0;
        int WordIndex = 0;
        const NSnippets::TSentsMatchInfo& SentsMatchInfo;
        const NSnippets::TSentsInfo& SentsInfo;
        const TUtf16String& Text;

    private:
        void NextChar() {
            ++CharIndex;
            if (WordIndex + 1 < SentsInfo.WordCount() && CharIndex == SentsInfo.WordVal[WordIndex + 1].TextBufBegin) {
                ++WordIndex;
            }
        }

        bool SkipBrackets(size_t sentEndCharIndex) {
            if (Text[CharIndex] != '(') {
                return false;
            }
            size_t bracketBalance = 1;
            NextChar();
            while (CharIndex < sentEndCharIndex && WordIndex < SentsInfo.WordCount()) {
                if (bracketBalance == 0) {
                    return true;
                }
                if (Text[CharIndex] == '(') {
                    ++bracketBalance;
                } else if (Text[CharIndex] == ')') {
                    --bracketBalance;
                }
                NextChar();
            }
            return true;
        }

    public:
        TDefinitionChecker(const NSnippets::TSentsMatchInfo& smi)
            : SentsMatchInfo(smi)
            , SentsInfo(smi.SentsInfo)
            , Text(smi.SentsInfo.Text)
        {}

        bool IsDefinition(int sentIndex) {
            WordIndex = SentsInfo.FirstWordIdInSent(sentIndex);
            if (!SentsMatchInfo.IsMatch(WordIndex)) {
                return false;
            }

            CharIndex = SentsInfo.WordVal[WordIndex].TextBufEnd;
            size_t sentEndCharIndex = SentsInfo.SentVal[sentIndex].Sent.EndOfs();
            while (CharIndex < sentEndCharIndex && WordIndex < SentsInfo.WordCount()) {
                if (IsWhitespace(Text[CharIndex]) || IsSkipablePunct(Text[CharIndex])) {
                    NextChar();
                    continue;
                }
                if (SkipBrackets(sentEndCharIndex)) {
                    continue;
                }
                if (!SentsInfo.IsCharIdFirstInWord(CharIndex, WordIndex)) {
                    break;
                }
                if (!SentsMatchInfo.IsMatch(WordIndex)) {
                    break;
                }
                CharIndex = SentsInfo.WordVal[WordIndex].TextBufEnd;
            }
            if (CharIndex >= sentEndCharIndex) {
                return false;
            }
            return IsBeginOfDefinition(CharIndex, Text);
        }
    };

} // end of anonymous namespace

namespace NSnippets {

    bool LooksLikeDefinition(const TSentsMatchInfo& smi, int sentId) {
        return TDefinitionChecker(smi).IsDefinition(sentId);
    }

    bool LooksLikeDefinition(const TQueryy& query, const TUtf16String& sentence, const TConfig& cfg) {
        TRetainedSentsMatchInfo rsmi;
        rsmi.SetView({sentence}, TRetainedSentsMatchInfo::TParams(cfg, query));
        if (const TSentsMatchInfo* smi = rsmi.GetSentsMatchInfo()) {
            return smi->SentLooksLikeDefinition(0);
        }
        return false;
    }

} // end of namespace NSnippets
