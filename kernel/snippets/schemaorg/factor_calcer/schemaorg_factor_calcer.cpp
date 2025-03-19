#include "schemaorg_factor_calcer.h"

#include <kernel/snippets/custom/schemaorg_viewer/schemaorg_viewer.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/sent_match.h>

#include <library/cpp/tokenizer/split.h>

#include <util/generic/hash.h>

namespace NSnippets {
    TAnswerInfo::TAnswerInfo(const NSchemaOrg::TAnswer* answer)
        : Answer(answer)
    {
        const TVector<TUtf16String> words = SplitIntoTokens(Answer->Text);
        if (words.empty()) {
            return;
        }
        WordSpan = TSpan(0, words.size() - 1);
        for (size_t i = 0; i < words.size(); i++) {
            WordHash2Pos[ComputeHash(words[i])].push_back(i);
        }
    }

    static float SafeDiv(int a, int b) {
        return b ? float(a) / float(b) : 0.0;
    }

    TAnswerMatchFactors TAnswerInfo::CalcMatchFactors(const TSentsMatchInfo& snip, const TSpans& wSpans) const {
        TAnswerMatchFactors res;
        TAnswerMatchValues match = CalcMatchValues(snip, wSpans);
        res.WordCount = WordSpan.Len();
        res.UpvoteCount = Answer->UpvoteCount.GetOrElse(-1);
        res.MaxSpanMatchRatioInSnip = SafeDiv(match.MaxCommonSubstrWordCount, match.SpanLen);
        res.MaxSpanMatchRatioInAnswer = SafeDiv(match.MaxCommonSubstrWordCount, WordSpan.Len());
        res.FirstMatchPosRatioInSnip = match.FirstWordOfMaxCSOfsInSnip >= 0
            ? 1.0 - SafeDiv(match.FirstWordOfMaxCSOfsInSnip, match.SpanLen)
            : -1.0;
        res.FirstMatchPosRatioInAnswer = match.FirstWordOfMaxCSOfsInAnswer >= 0
            ? 1.0 - SafeDiv(match.FirstWordOfMaxCSOfsInAnswer, WordSpan.Len())
            : -1.0;
        return res;
    }

    TAnswerMatchValues TAnswerInfo::CalcMatchValues(const TSentsMatchInfo& snip, const TSpans& wSpans) const {
        // find longest common substring in word count (dynamic programming)
        if (wSpans.size() != 1) {
            return TAnswerMatchValues();
        }
        const TSpan& ws = *wSpans.begin();
        TAnswerMatchValues res;
        res.SpanLen = ws.Len();
        // maxMatchLen[i][j] is max word count of common substr that ends on pos i in snip and on pos j in answer
        TVector<TVector<size_t>> maxMatchLen(res.SpanLen, TVector<size_t>(WordSpan.Len(), 0));
        for (int i = 0; i < res.SpanLen; i++) {
            const size_t snipWordHash = snip.SentsInfo.WordVal[ws.First + i].Word.Hash;
            auto iter = WordHash2Pos.find(snipWordHash);
            if (iter != WordHash2Pos.end()) {
                for (int j : iter->second) {
                    int currMatchLen = i > 0 && j > 0
                                       ? maxMatchLen[i - 1][j - 1] + 1
                                       : 1;
                    maxMatchLen[i][j] = currMatchLen;
                    if (res.MaxCommonSubstrWordCount < currMatchLen) {
                        res.MaxCommonSubstrWordCount = currMatchLen;
                        res.FirstWordOfMaxCSOfsInSnip = i - currMatchLen + 1;
                        res.FirstWordOfMaxCSOfsInAnswer = j - currMatchLen + 1;
                    }
                }
            }
        }
        return res;
    }

    TSchemaOrgQuestionInfo::TSchemaOrgQuestionInfo(const TSchemaOrgArchiveViewer* schemaOrgViewer)
        : SchemaOrgViewer(schemaOrgViewer)
    {
        if (!schemaOrgViewer) {
            return;
        }
        Question = SchemaOrgViewer->GetQuestion();
        if (!Question) {
            return;
        }
        TVector<const NSchemaOrg::TAnswer*> answers;
        Question->GetAllNotEmptyAnswers(answers);
        if (answers.empty()) {
            return;
        }
        BestAnswerData = Question->GetBestAnswer();
        BestAnswer.Reset(new TAnswerInfo(&BestAnswerData));
        for (const NSchemaOrg::TAnswer* a : answers) {
            Answers.emplace_back(TAnswerInfo(a));
        }
    }
}

