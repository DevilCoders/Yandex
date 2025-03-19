#include "maxfit.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_info/sentword.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/single_snip.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/length.h>

#include <library/cpp/stopwords/stopwords.h>
#include <library/cpp/langs/langs.h>

#include <util/generic/strbuf.h>

namespace NSnippets
{
    namespace NMaxFit
    {
        static int CorrectLastWord(int w0, int lastFit, const TSentsInfo& sentsInfo, const TConfig& cfg) {
            TSentMultiword beg(sentsInfo.WordId2SentWord(w0));
            TSentMultiword word(sentsInfo.WordId2SentWord(lastFit));
            for (; word != beg; --word) {
                if (word.LastWordId() > lastFit) {
                    continue;
                }
                if (word.IsLastInSent()) {
                    return word.LastWordId();
                }
                if (word.IsFirstInSent()) {
                    continue;
                }
                TWtringBuf wordBuf = word.GetWordBuf();
                EStickySide side = STICK_RIGHT;
                if (cfg.GetStopWordsFilter().IsStopWord(wordBuf.data(), wordBuf.size(), TLangMask(), &side)) {
                    continue;
                }
                return word.LastWordId();
            }
            return lastFit;
        }

        TSnip GetTSnippet(const TConfig& cfg, const TWordSpanLen& wordSpanLen, const TSingleSnip& fragment, float maxLength) {
            const TSentsMatchInfo& smi = *fragment.GetSentsMatchInfo();
            if (!smi.WordsCount()) {
                return TSnip();
            }
            int w0 = fragment.GetFirstWord();
            int w1 = fragment.GetLastWord();
            int lastFit = wordSpanLen.FindFirstWordLonger(smi, w0, w1, maxLength) - 1;
            if (lastFit < w0) {
                return TSnip();
            }
            w1 = CorrectLastWord(w0, lastFit, smi.SentsInfo, cfg);
            return TSnip(TSingleSnip(w0, w1, smi), InvalidWeight);
        }
    }
}
