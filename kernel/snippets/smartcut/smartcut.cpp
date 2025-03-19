#include "smartcut.h"
#include "char_class.h"
#include "consts.h"
#include "cutparam.h"
#include "hilited_length.h"
#include "pixel_length.h"
#include "snip_length.h"
#include "wordsinfo.h"
#include "wordspan_length.h"

#include <kernel/snippets/strhl/goodwrds.h>
#include <kernel/snippets/strhl/zonedstring.h>
#include <kernel/snippets/strhl/hlmarks.h>
#include <library/cpp/stopwords/stopwords.h>

#include <util/charset/unidata.h>
#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/utility.h>
#include <util/system/yassert.h>

namespace NSnippets
{
    namespace
    {
        const TUtf16String GOOD_BREAK_PUNCT = u";()";
        const TUtf16String HTTP = u"http://";
        const TUtf16String HTTPS = u"https://";

        enum EBreakType {
            SENT_BREAK,
            GOOD_BREAK,
            WEAK_BREAK,
            NO_BREAK,
            BREAK_TYPE_COUNT,
        };

        class TBreakTypeClassifier {
        public:
            TBreakTypeClassifier(const TWordsInfo& wordsInfo, int spanFirstWord, const TWordFilter* stopwordsFilter)
                : WordsInfo(wordsInfo)
                , SpanFirstWord(spanFirstWord)
                , StopwordsFilter(stopwordsFilter)
            {
            }

            EBreakType ClassifyBreakType(int word) const {
                Y_ASSERT(word >= SpanFirstWord);
                if (WordsInfo.Words[word].LastInSent) {
                    return SENT_BREAK;
                }
                Y_ASSERT(word + 1 < WordsInfo.WordCount());
                if (word > SpanFirstWord && WordsInfo.Words[word].FirstInSent) {
                    return NO_BREAK;
                }
                if (IsPartOfMultiToken(word)) {
                    return NO_BREAK;
                }
                TWtringBuf punctAfter = WordsInfo.GetPunctAfter(word);
                if (IsGoodSentBreak(punctAfter)) {
                    return GOOD_BREAK;
                }
                if (IsStopword(word, punctAfter)) {
                    return NO_BREAK;
                }
                if (IsGoodSentBreakWithoutStopword(punctAfter)) {
                    return GOOD_BREAK;
                }
                return WEAK_BREAK;
            }

        private:
            inline bool IsPartOfMultiToken(int word) const {
                if (word + 1 == WordsInfo.WordCount()) {
                    return false;
                }
                TWtringBuf wordWithPunct = WordsInfo.GetWordWithPunctAfter(word);
                return wordWithPunct == HTTP || wordWithPunct == HTTPS;
            }

            inline bool IsGoodSentBreak(const TWtringBuf& punctAfter) const {
                return punctAfter.find_first_of(GOOD_BREAK_PUNCT) != punctAfter.npos;
            }

            inline bool IsStopword(const TWtringBuf& wordBuf) const {
                if (!StopwordsFilter) {
                    return wordBuf.size() <= 1;
                }
                EStickySide side = STICK_RIGHT;
                return StopwordsFilter->IsStopWord(wordBuf.data(), wordBuf.size(), TLangMask(), &side);
            }

            inline bool IsStopword(int word, const TWtringBuf& punctAfter) const {
                if (!IsStopword(WordsInfo.GetWordBuf(word))) {
                    return false;
                }
                for (wchar16 c : punctAfter) {
                    if (IsRightQuoteOrBracket(c)) {
                        // The stopword is not "connected" with the next word
                        return false;
                    }
                }
                return true;
            }

            inline bool IsGoodSentBreakWithoutStopword(const TWtringBuf& punctAfter) const {
                return punctAfter.Contains(',');
            }

            const TWordsInfo& WordsInfo;
            int SpanFirstWord = 0;
            const TWordFilter* StopwordsFilter = nullptr;
        };

        class TBracketChecker {
        public:
            TBracketChecker(const TWordsInfo& wordsInfo, int w0, int w1)
                : WordsInfo(wordsInfo)
            {
                Init(w0, w1);
            }

            bool IsAfterBracket(int word) const {
                Y_ASSERT(word < WordsInfo.WordCount());
                size_t wordEndOfs = WordsInfo.Words[word].TextBufEnd;
                Y_ASSERT(wordEndOfs >= BeginOfs && wordEndOfs - BeginOfs < AfterBracket.size());
                return AfterBracket[wordEndOfs - BeginOfs];
            }

        private:
            void Init(int w0, int w1) {
                BeginOfs = WordsInfo.Words[w0].WordEnd; // ignore brackets before first word
                size_t endOfs = WordsInfo.Words[w1].TextBufEnd;
                size_t lastBracketOfs = 0; // assumes that BeginOfs > 0
                AfterBracket.resize(endOfs - BeginOfs + 1);
                for (size_t ofs = BeginOfs; ofs <= endOfs; ++ofs) {
                    if (WordsInfo.Text[ofs] == '(') {
                        lastBracketOfs = ofs;
                    } else if (WordsInfo.Text[ofs] == ')') {
                        lastBracketOfs = 0;
                    }
                    AfterBracket[ofs - BeginOfs] = lastBracketOfs > 0 &&
                        ofs - lastBracketOfs < MIN_AFTER_BRACKET;
                }
            }

            const TWordsInfo& WordsInfo;
            TVector<bool> AfterBracket;
            size_t BeginOfs = 0;
            static const int MIN_AFTER_BRACKET = 40;
        };

        int SmartCutImpl(int w0, int w1, const TWordsInfo& wordsInfo,
            float maxLen,
            const TWordSpanLengthCalculator& calculator,
            const TTextCuttingOptions& options)
        {
            int lastFit = calculator.FindFirstWordLonger(w0, w1, maxLen) - 1;
            if (lastFit < w0) {
                return -1;
            }

            float minLen;
            int firstFit;
            if (options.MaximizeLen) {
                minLen = maxLen;
                firstFit = lastFit;
            } else {
                minLen = maxLen * options.Threshold;
                firstFit = calculator.FindFirstWordLonger(w0, lastFit, minLen);
            }

            TBreakTypeClassifier classifier(wordsInfo, w0, options.StopWordsFilter);
            if (firstFit < lastFit) {
                int bestBreak[BREAK_TYPE_COUNT];
                for (int i = 0; i < BREAK_TYPE_COUNT; ++i) {
                    bestBreak[i] = -1;
                }
                TBracketChecker bracketChecker(wordsInfo, w0, lastFit);
                for (int word = lastFit; word >= firstFit; --word) {
                    if (bracketChecker.IsAfterBracket(word)) {
                        continue;
                    }
                    EBreakType breakType = classifier.ClassifyBreakType(word);
                    if (breakType == SENT_BREAK) {
                        return word;
                    }
                    if (breakType < NO_BREAK && bestBreak[breakType] == -1) {
                        bestBreak[breakType] = word;
                    }
                }
                for (int i = 0; i < BREAK_TYPE_COUNT; ++i) {
                    if (bestBreak[i] != -1) {
                        return bestBreak[i];
                    }
                }
            }
            for (int word = lastFit; word >= w0; --word) {
                EBreakType breakType = classifier.ClassifyBreakType(word);
                if (breakType != NO_BREAK) {
                    return word;
                }
            }
            return lastFit;
        }

        void HiliteText(const TInlineHighlighter& highlighter, const TUtf16String& text, TVector<TBoldSpan>& boldSpans) {
            TZonedString zoned = text;
            TPaintingOptions options = TPaintingOptions::DefaultSnippetOptions();
            options.SrcOutput = true;
            highlighter.PaintPassages(zoned, options);
            FillBoldSpans(zoned, boldSpans);
        }

        TPixelLengthCalculator* GetPixelLengthCalculator(const TUtf16String& source, const TInlineHighlighter& highlighter) {
            TVector<TBoldSpan> boldSpans;
            HiliteText(highlighter, source, boldSpans);
            return new TPixelLengthCalculator(source, boldSpans);
        }

        const TUtf16String TRASH_WORD = u" Qqqqqqqqqqqqqqqq";

        TMultiCutResult SmartCutCommon(
            const TUtf16String& source,
            const float maxLengthStd,
            const float maxLengthExt,
            const TCutParams& cutParams,
            THolder<TPixelLengthCalculator> pixelLengthCalculator,
            const TTextCuttingOptions& options)
        {
            TMultiCutResult result;
            TUtf16String actualSource = source;
            if (options.AddEllipsisToShortText) {
                actualSource += TRASH_WORD;
            }
            TWordsInfo wordsInfo(actualSource, options.HilightMark);

            int w1 = wordsInfo.WordCount() - 1;
            if (options.AddEllipsisToShortText) {
                --w1;
            }
            if (options.CutLastWord) {
                --w1;
            }
            if (w1 < 0) {
                return result;
            }
            TMakeFragment makeFragment = [&](int firstWord, int lastWord) {
                TTextFragment fragment = wordsInfo.GetTextFragment(firstWord, lastWord);
                fragment.Calculator = pixelLengthCalculator.Get();
                return fragment;
            };
            TSnipLengthCalculator snipLengthCalculator(cutParams);
            TWordSpanLengthCalculator wordSpanLengthCalculator(makeFragment, snipLengthCalculator);

            struct TCutResult {
                TUtf16String Passage;
                int NumWords = 0;
                int NumChars = 0;
            };

            auto cutter = [&](int wordMax, float maxLength) -> TCutResult {
                TCutResult result;
                if (maxLength > 0) {
                    int w1Cut = SmartCutImpl(0, wordMax, wordsInfo, maxLength, wordSpanLengthCalculator, options);
                    if (w1Cut >= 0) {
                        result.NumWords = w1Cut + 1;
                        result.Passage = ToWtring(wordsInfo.GetTextBuf(0, w1Cut));
                        result.NumChars = (int)result.Passage.size();
                        if (!wordsInfo.Words[w1Cut].LastInSent && options.AddBoundaryEllipsis) {
                            result.Passage += BOUNDARY_ELLIPSIS;
                        }
                    }
                }
                return result;
            };

            TCutResult cutShort = cutter(w1, maxLengthStd);
            TCutResult cutLong = cutter(w1, maxLengthExt);

            result.Short = cutShort.Passage;
            result.Long = cutLong.Passage;

            result.CharCountDifference = ClampVal(cutLong.NumChars - cutShort.NumChars, 0, Max<int>());
            if (cutLong.NumChars >= cutShort.NumChars) {
                result.CommonPrefixLen = cutShort.NumChars;
            }
            result.WordCountDifference = ClampVal(cutLong.NumWords - cutShort.NumWords, 0, Max<int>());
            return result;
        }
    } // anonymouos namespace

    TUtf16String SmartCutSymbol(
        const TUtf16String& source,
        size_t maxSymbols,
        const TTextCuttingOptions& options)
    {
        const float maxLength = static_cast<float>(maxSymbols);
        const TCutParams cutParams = TCutParams::Symbol();
        return SmartCutCommon(source, maxLength, 0.0, cutParams, nullptr, options).Short;
    }

    TUtf16String SmartCutPixelNoQuery(
        const TUtf16String& source,
        float maxRows,
        int pixelsInLine,
        float fontSize,
        const TTextCuttingOptions& options,
        const TVector<TBoldSpan>& boldSpans)
    {
        const TCutParams cutParams = TCutParams::Pixel(pixelsInLine, fontSize);
        return SmartCutCommon(source, maxRows, 0.0, cutParams, MakeHolder<TPixelLengthCalculator>(source, boldSpans), options).Short;
    }

    TUtf16String SmartCutPixelWithQuery(
        const TUtf16String& source,
        const TInlineHighlighter& highlighter,
        float maxRows,
        int pixelsInLine,
        float fontSize,
        const TTextCuttingOptions& options)
    {
        const TCutParams cutParams = TCutParams::Pixel(pixelsInLine, fontSize);
        return SmartCutCommon(source, maxRows, 0.0, cutParams, THolder<TPixelLengthCalculator>(GetPixelLengthCalculator(source, highlighter)), options).Short;
    }

    TUtf16String SmartCutPixelCustomWithQuery(
        const TUtf16String& source,
        const TPixelLengthCalculator& calc,
        float maxRows,
        int pixelsInLine,
        float fontSize,
        const TTextCuttingOptions& options)
    {
        const TCutParams cutParams = TCutParams::Pixel(pixelsInLine, fontSize);
        return SmartCutCommon(source, maxRows, 0.0, cutParams, MakeHolder<TPixelLengthCalculator>(calc), options).Short;
    }

    TUtf16String SmartCutWithQuery(
        const TUtf16String& source,
        const TInlineHighlighter& highlighter,
        float maxLength,
        const TCutParams& cutParams,
        const TTextCuttingOptions& options)
    {
        return SmartCutCommon(source, maxLength, 0.0, cutParams, cutParams.IsPixel ? THolder<TPixelLengthCalculator>(GetPixelLengthCalculator(source, highlighter)) : nullptr, options).Short;
    }

    TMultiCutResult SmartCutExtSnipWithQuery(
        const TUtf16String& source,
        const TInlineHighlighter& highlighter,
        float maxLengthShort,
        float maxLengthLong,
        const TCutParams& cutParams,
        const TTextCuttingOptions& options)
    {
        return SmartCutCommon(source, maxLengthShort, maxLengthLong, cutParams, cutParams.IsPixel ? THolder<TPixelLengthCalculator>(GetPixelLengthCalculator(source, highlighter)) : nullptr, options);
    }

} // namespace NSnippets
