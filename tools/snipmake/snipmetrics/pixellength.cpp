#include "pixellength.h"
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/titles/make_title/util_title.h>
#include <util/generic/string.h>

namespace NSnippets {

    static const int YANDEX_STRING_LIMIT = 630;
    static const int GOOGLE_STRING_LIMIT = 540;

    static const float YANDEX_REGULAR_TEXT[] = {
        #include "pixtable_yandreg.inc"
    };

    static const float YANDEX_BOLD_TEXT[] = {
        #include "pixtable_yandbold.inc"
    };

    static const float GOOGLE_REGULAR_TEXT[] = {
        #include "pixtable_googreg.inc"
    };

    static const float GOOGLE_BOLD_TEXT[] = {
        #include "pixtable_googbold.inc"
    };


    inline float GetNextWordLen(const TUtf16String& text, int start, int stop, const float* textWidth)
    {
        float sum = 0;
        for (int i = start; i < stop; i++) {
            sum += textWidth[text[i]];
        }
        return sum;
    }

    PixelLenResult GetPixelLen(const TSentsMatchInfo& sentsMatchInfo, size_t snipTextOffset, const float* regularText, const float* boldText, const int stringPixelLimit)
    {
        int currentWord = 0;
        size_t currentWordStart = 0;
        size_t currentWordStop = 0;
        size_t prevWordStop = 0;
        float sum = 0;
        int linesCount = 0;

        const TSentsInfo& sInfo = sentsMatchInfo.SentsInfo;
        const TUtf16String& text = sInfo.Text;
        int wordCount = sInfo.WordCount();

        while (currentWord < wordCount && sInfo.GetWordBuf(currentWord).data() < text.data() + snipTextOffset) {
            ++currentWord;
        }

        for (size_t i = snipTextOffset; i < text.size();) {
            if (currentWord < wordCount) {
                TWtringBuf word = sInfo.GetWordBuf(currentWord);
                const size_t wordOfs = word.data() - text.data();
                if (currentWordStart <= wordOfs) {
                    currentWordStart = wordOfs;
                    currentWordStop = currentWordStart + word.size();
                }
            }
            if (i == currentWordStop && currentWord < wordCount) {
                prevWordStop = currentWordStop;
                currentWord++;
            }

            // calculate length between words
            // assumption: only words can break lines
            if (i >= prevWordStop && i < currentWordStart) {
                sum += regularText[text[i]];
                i++;
            }

            if (currentWord == wordCount) {
                sum += regularText[text[i]];
                i++;
            }

            // calculate length of words
            if (i == currentWordStart) {
                float nextWordLen = 0;
                // matched word
                if (sentsMatchInfo.IsMatch(currentWord)) {
                    nextWordLen = GetNextWordLen(text, currentWordStart, currentWordStop, boldText);
                // not matched word
                } else {
                    nextWordLen = GetNextWordLen(text, currentWordStart, currentWordStop, regularText);
                }
                // current result + next words doesn't fit string len
                if (sum + nextWordLen > stringPixelLimit) {
                    linesCount++;
                    sum = nextWordLen;
                // current result + next result fit string len
                } else {
                    sum += nextWordLen;
                }
                i += currentWordStop - currentWordStart;
            }
        }

        // strings count starts from 1 && not empty snippet
        if (sum > 0.001 || linesCount > 0) {
            linesCount += 1;
        }

        return PixelLenResult(linesCount, sum / float(stringPixelLimit));
    }

    PixelLenResult GetYandexPixelLen(const TSentsMatchInfo& sentsMatchInfo, size_t snipTextOffset)
    {
        return GetPixelLen(sentsMatchInfo, snipTextOffset, YANDEX_REGULAR_TEXT, YANDEX_BOLD_TEXT, YANDEX_STRING_LIMIT);
    }

    PixelLenResult GetGooglePixelLen(const TSentsMatchInfo& sentsMatchInfo, size_t snipTextOffset)
    {
        return GetPixelLen(sentsMatchInfo, snipTextOffset, GOOGLE_REGULAR_TEXT, GOOGLE_BOLD_TEXT, GOOGLE_STRING_LIMIT);
    }
}
