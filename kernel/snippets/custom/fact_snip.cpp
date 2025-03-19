#include "fact_snip.h"
#include "extended.h"
#include "extended_length.h"

#include <kernel/snippets/calc_dssm/calc_dssm.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/explain/finaldumpbin.h>
#include <kernel/snippets/explain/top_candidates.h>
#include <kernel/snippets/sent_info/sentword.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/single_snip.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/snip_builder/snip_builder.h>
#include <kernel/snippets/smartcut/cutparam.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/scheme/scheme.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>



namespace NSnippets {

    const size_t FACT_SNIP_VERSION = 3;
    const float MAX_FACT_SNIP_LEN = 500.0;
    const wchar16 QUESTION_MARK = wchar16('?');
    const int LONG_SENT_LENGTH_IN_WORDS = 60;
    const float MIN_EXTEND_RATIO = 1.3;

    static bool HasOnlyDigits(const TWtringBuf& text) {
        for (auto c : text) {
            if (!IsDigit(c)) {
                return false;
            }
        }
        return true;
    }

    static bool IsFootnote(const TWtringBuf& text) {
        static const THashSet<TUtf16String> footnotes = {
            u"править",
            u"править | править вики-текст",
            u"неавторитетный источник?",
            u"что?",
            u"уточнить",
            u"где?",
            u"en"
        };
        static const TVector<TUtf16String> footnotePrefixes = {
            u"источник не указан "
        };
        if (HasOnlyDigits(text)) {
            return true;
        }
        if (footnotes.find(text) != footnotes.end())
            return true;
        for (const auto& pref : footnotePrefixes) {
            if (text.StartsWith(pref))
                return true;
        }
        return false;
    }

    static void CutFootnotes(TUtf16String& text) {
        TUtf16String result;
        size_t start = 0;
        size_t pos = 0;
        TWtringBuf buf(text);
        while ((pos = buf.find('[', pos)) != buf.npos) {
            size_t to = buf.find(']', pos);
            if (to == text.npos) {
                to = text.size();
            }
            TWtringBuf footnoteText = buf.substr(pos + 1, to - pos - 1);
            if (IsFootnote(footnoteText)) {
                if (pos > 0 && IsSpace(buf[pos - 1]) && to + 1 < buf.size() && IsPunct(buf[to + 1])) {
                    // Cut space in front of footnote in cases like this: "some text [123], other text" -> "some text, other text"
                    --pos;
                }
                if (start < pos) {
                    result += buf.substr(start, pos - start);
                }
                start = to + 1;
            }
            pos = to + 1;
        }
        if (start) {
            if (start < text.size()) {
                result += text.substr(start);
            }
            text = result;
        }
    }

    static TSingleSnip CutQuestions(const TSingleSnip& snip, const TSentsInfo& si) {
        int firstSent = snip.GetFirstSent();
        while (firstSent < snip.GetLastSent()) {
            TWtringBuf sentEnd = si.GetSentEndBlanks(firstSent);
            if (sentEnd.find(QUESTION_MARK) != sentEnd.npos) {
                ++firstSent;
            } else {
                break;
            }
        }
        if (firstSent == snip.GetFirstSent()) {
            return snip;
        }
        int firstWord = si.FirstWordIdInSent(firstSent);
        return TSingleSnip(firstWord, snip.GetLastWord(), *snip.GetSentsMatchInfo());
    }

    static bool IsSpaces(const TWtringBuf& s) {
        for (wchar16 c : s) {
            if (!IsSpace(c))
                return false;
        }
        return true;
    }

    static TUtf16String CanonizeWord(const TWtringBuf& wordBuf) {
        TUtf16String word(wordBuf);
        word.to_lower();
        return word;
    }

    static TSingleSnip CutDuplicatedWords(const TSingleSnip& snip, const TSentsInfo& si) {
        int firstWordId = snip.GetFirstWord();
        while (firstWordId + 1 < snip.GetLastWord()) {
            TWtringBuf firstWord = si.GetWordBuf(firstWordId);
            TWtringBuf blanks = si.GetBlanksAfter(firstWordId);
            TWtringBuf secondWord = si.GetWordBuf(firstWordId + 1);
            if (IsSpaces(blanks) && CanonizeWord(firstWord) == CanonizeWord(secondWord)) {
                ++firstWordId;
            } else {
                break;
            }
        }
        return TSingleSnip(firstWordId, snip.GetLastWord(), *snip.GetSentsMatchInfo());
    }

    static TSnip ExtendSnipToFullSent(const TSnip& snip, const TReplaceContext& repCtx) {
        TWordSpanLen symbolSpanLen(TCutParams::Symbol());
        TSnipBuilder b(repCtx.SentsMInfo, symbolSpanLen, MAX_FACT_SNIP_LEN, MAX_FACT_SNIP_LEN);
        const TSentsInfo& info = repCtx.SentsMInfo.SentsInfo;
        for (const auto& singleSnip : snip.Snips) {
            TSentWord start = info.WordId2SentWord(singleSnip.GetFirstWord());
            TSentWord end = info.WordId2SentWord(singleSnip.GetLastWord());
            if (!b.Add(start, end)) {
                return snip;
            }
        }
        b.GrowLeftToSent();
        b.GrowRightToSent();
        return b.Get(InvalidWeight);
    }

    static TUtf16String GetPrettifiedSnipText(const TSnip& snip, const TReplaceContext& repCtx) {
        TUtf16String snipText;
        const TSentsInfo& sentsInfo = repCtx.SentsMInfo.SentsInfo;
        for (const auto& singleSnip : snip.Snips) {
            TSingleSnip cuttedSnip = CutQuestions(singleSnip, sentsInfo);
            bool needPrefix = !sentsInfo.IsWordIdFirstInSent(cuttedSnip.GetFirstWord());
            cuttedSnip = CutDuplicatedWords(cuttedSnip, sentsInfo);
            TWtringBuf prefix, text, suffix;
            sentsInfo.GetTextWithEllipsis(cuttedSnip.GetFirstWord(), cuttedSnip.GetLastWord(), prefix, text, suffix);
            if (!needPrefix) {
                prefix.Clear();
            }
            snipText += prefix;
            snipText += text;
            snipText += suffix;

        }
        CutFootnotes(snipText);
        return snipText;
    }

    static bool IsSentenceCutted(const TSnip& snip, const TReplaceContext& repCtx) {
        if (snip.Snips.empty())
            return false;

        const TSentsInfo& sentsInfo = repCtx.SentsMInfo.SentsInfo;
        const auto& lastSnip = snip.Snips.back();
        const int lastSentId = lastSnip.GetLastSent();
        const int nextSentId = lastSentId + 1;
        if (nextSentId >= sentsInfo.SentencesCount() ||
            !sentsInfo.IsWordIdLastInSent(lastSnip.GetLastWord()) ||
            sentsInfo.GetSentLengthInWords(lastSentId) < LONG_SENT_LENGTH_IN_WORDS) {
            return false;
        }

        TWtringBuf sentEndBlanks = sentsInfo.GetSentEndBlanks(lastSentId);
        const TArchiveSent& acrSent = sentsInfo.GetArchiveSent(lastSentId);
        if (!acrSent.Sent.EndsWith(wchar16('.')) && sentEndBlanks.EndsWith(TUtf16String(u". "))) {
            int firstWordInNextSentId = sentsInfo.FirstWordIdInSent(nextSentId);
            TWtringBuf firstWordInNextSent = sentsInfo.GetWordBuf(firstWordInNextSentId);
            if (!IsTitleWord(firstWordInNextSent)) {
                return true;
            }
        }
        return false;
    }

    TFactSnipReplacer::TFactSnipReplacer()
        : IReplacer("fact_snip")
    {
    }



    void TFactSnipReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();
        if (!ctx.Cfg.ExpFlagOn("use_factsnip")) {
            return;
        }
        if (ctx.OneFragmentSnip.Snips.size() != 1) {
            return;
        }
        NJson::TJsonValue data(NJson::JSON_MAP);
        TSnip factSnip = ExtendSnipToFullSent(ctx.OneFragmentSnip, ctx);
        TUtf16String snipText = GetPrettifiedSnipText(factSnip, ctx);
        data["text"] = WideToUTF8(snipText);
        if (!ctx.Cfg.ExpFlagOff("factsnip_read_more")) {
            TSnip readMoreSnip = GetExtendedIfLongEnough(ctx, GetExtSnipLen(ctx.Cfg, ctx.LenCfg), factSnip, factSnip, false);
            if (factSnip.Snips) {
                TUtf16String readMoreSnipText = GetPrettifiedSnipText(readMoreSnip, ctx);
                if (readMoreSnipText.StartsWith(snipText) && readMoreSnipText.length() > MIN_EXTEND_RATIO * snipText.length()) {
                    data["extended_text"] = WideToUTF8(readMoreSnipText);
                }
            }
        }
        TString factorsDump = TFinalFactorsDumpBinary::DumpFactors(ctx.OneFragmentSnip, ctx.Cfg.FactorsToDump());
        if (factorsDump) {
            data["factors"] = factorsDump;
        }
        data["version"] = FACT_SNIP_VERSION;
        if (ctx.Cfg.ExpFlagOn("fact_snip_candidates") && !!(manager->GetFactSnippetTopCandidatesCallback())) {
            data["fact_snip_candidates"] =  NJson::TJsonValue(NJson::JSON_ARRAY);

            const bool ignoreMetaDescr = ctx.Cfg.ExpFlagOn("fact_snip_ignore_meta_descr");
            const bool ignoreCutted = ctx.Cfg.ExpFlagOn("fact_snip_ignore_cutted");
            TVector<TSnip>& candidates = manager->GetFactSnippetTopCandidatesCallback()->GetBestCandidates();
            for (auto& snip: candidates)  {
                NJson::TJsonValue candidateElement(NJson::JSON_MAP);
                TSnip factSnipCandidate = ExtendSnipToFullSent(snip, ctx);
                TUtf16String factSnipCandidateText = GetPrettifiedSnipText(factSnipCandidate, ctx);
                const bool isCutted = ignoreCutted && IsSentenceCutted(factSnipCandidate, ctx);
                const bool hasMetaDescr = ignoreMetaDescr && snip.ContainsMetaDescr();
                if (isCutted || hasMetaDescr) {
                    continue;
                }
                candidateElement["text"] = WideToUTF8(factSnipCandidateText);

                if (!ctx.Cfg.ExpFlagOff("factsnip_read_more")) {
                    TSnip readMoreSnipCandidate = GetExtendedIfLongEnough(ctx, GetExtSnipLen(ctx.Cfg, ctx.LenCfg), factSnipCandidate, factSnipCandidate, false);
                    if (factSnipCandidate.Snips) {
                        TUtf16String readMoreSnipCandidateText = GetPrettifiedSnipText(readMoreSnipCandidate, ctx);
                        if (readMoreSnipCandidateText.StartsWith(snipText) && readMoreSnipCandidateText.length() > MIN_EXTEND_RATIO * factSnipCandidateText.length()) {
                            candidateElement["text_extended"] = WideToUTF8(readMoreSnipCandidateText);
                        }
                    }
                }
                if (factorsDump) {
                    if (ctx.Cfg.ExpFlagOn("factsnip_dssm") && !ctx.Cfg.ExpFlagOff("get_query")) {
                        AdjustCandidatesDssm(snip, ctx.Cfg, ctx.SentsMInfo, manager->GetCallback());
                    }
                    TString candidateFactorsDump = TFinalFactorsDumpBinary::DumpFactors(snip, ctx.Cfg.FactorsToDump());
                    candidateElement["factors"] = candidateFactorsDump;
                }
                data["fact_snip_candidates"].AppendValue(candidateElement);
            }
        }
        manager->GetExtraSnipAttrs().AddClickLikeSnipJson("fact_snip", NJson::WriteJson(data));

    }

} // namespace NSnippets
